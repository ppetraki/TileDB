/**
 * @file   write_state.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017-2018 TileDB, Inc.
 * @copyright Copyright (c) 2016 MIT and Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file implements the WriteState class.
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

#include "tiledb/sm/buffer/const_buffer.h"
#include "tiledb/sm/misc/comparators.h"
#include "tiledb/sm/misc/logger.h"
#include "tiledb/sm/misc/utils.h"
#include "tiledb/sm/query/query.h"
#include "tiledb/sm/tile/tile.h"

#include "tiledb/sm/fragment/write_state.h"

namespace tiledb {
namespace sm {

/* ****************************** */
/*   CONSTRUCTORS & DESTRUCTORS   */
/* ****************************** */

WriteState::WriteState() {
  metadata_ = nullptr;
  bounding_coords_ = nullptr;
  fragment_ = nullptr;
  mbr_ = nullptr;
  tile_coords_aux_ = nullptr;
}

WriteState::~WriteState() {
  for (auto& tile : tiles_)
    delete tile;

  for (auto& tile_var : tiles_var_)
    delete tile_var;

  for (auto& tile_io : tile_io_)
    delete tile_io;

  for (auto& tile_io_var : tile_io_var_)
    delete tile_io_var;

  if (mbr_ != nullptr)
    ::operator delete(mbr_);

  if (bounding_coords_ != nullptr)
    ::operator delete(bounding_coords_);

  if (tile_coords_aux_ != nullptr)
    ::operator delete(tile_coords_aux_);
}

/* ****************************** */
/*           MUTATORS             */
/* ****************************** */

Status WriteState::init(const Fragment* fragment) {
  fragment_ = fragment;
  metadata_ = fragment_->metadata();

  RETURN_NOT_OK(init_tiles());
  RETURN_NOT_OK(init_tile_io());

  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  auto attribute_num = array_schema->attribute_num();
  uint64_t coords_size = array_schema->coords_size();

  // Initialize the number of cells written in the current tile
  tile_cell_num_.resize(attribute_num + 1);
  for (unsigned int i = 0; i < attribute_num + 1; ++i)
    tile_cell_num_[i] = 0;

  // Initialize the current size of the variable attribute file
  buffer_var_offsets_.resize(attribute_num);
  for (unsigned int i = 0; i < attribute_num; ++i)
    buffer_var_offsets_[i] = 0;

  mbr_ = ::operator new(2 * coords_size, std::nothrow);
  if (mbr_ == nullptr)
    return LOG_STATUS(Status::WriteStateError(
        "Cannot initialize write state; MBR allocation failed"));

  bounding_coords_ = ::operator new(2 * coords_size, std::nothrow);
  if (bounding_coords_ == nullptr)
    return LOG_STATUS(
        Status::WriteStateError("Cannot initialize write state; Bounding "
                                "coordinates allocation failed"));

  tile_coords_aux_ = ::operator new(coords_size, std::nothrow);
  if (tile_coords_aux_ == nullptr)
    return LOG_STATUS(
        Status::WriteStateError("Cannot initialize write state; Auxiliary tile "
                                "coordinates allocation failed"));

  cells_written_.resize(attribute_num + 1);

  return Status::Ok();
}

Status WriteState::finalize() {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  unsigned attribute_num = array_schema->attribute_num();
  auto cell_num = metadata_->cell_num_in_domain();

  // Check if fragment exists (i.e., some cells were written)
  bool fragment_exists = false;
  for (const auto& c : cells_written_) {
    if (c > 0) {
      fragment_exists = true;
      break;
    }
  }

  // Do nothing if fragment does not exist
  if (!fragment_exists)
    return Status::Ok();

  // Write last tile (applicable only to the sparse case)
  if (!tiles_[attribute_num]->empty())
    RETURN_NOT_OK(write_last_tile());

  // Sync all attributes
  RETURN_NOT_OK(close_files());

  if (metadata_->dense()) {  // DENSE
    // Check number of cells written
    for (unsigned i = 0; i < attribute_num; ++i) {
      if (cells_written_[i] != 0 && cells_written_[i] != cell_num)
        return LOG_STATUS(Status::WriteStateError(
            std::string("Cannot finalize write state for attribute '") +
            array_schema->attribute_name(i) +
            "'; Incorrect number of cells written"));
    }
  } else {  // SPARSE
    // Number of cells written should be equal across all attributes
    if (!metadata_->dense() && fragment_exists) {
      for (unsigned i = 1; i < cells_written_.size(); ++i) {
        if (cells_written_[i] != cells_written_[i - 1])
          return LOG_STATUS(Status::WriteStateError(
              std::string("Cannot finalize write state; The number of cells "
                          "written across the attributes is not the same")));
      }
    }
  }

  // Success
  return Status::Ok();
}

Status WriteState::close_files() {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  auto attribute_num = array_schema->attribute_num();
  auto attribute_ids = fragment_->query()->attribute_ids();
  auto storage_manager = fragment_->query()->storage_manager();

  // Sync all attributes
  for (auto attribute_id : attribute_ids) {
    // For all attributes
    if (attribute_id == attribute_num) {
      RETURN_NOT_OK(storage_manager->close_file(fragment_->coords_uri()));
    } else {
      RETURN_NOT_OK(
          storage_manager->close_file(fragment_->attr_uri(attribute_id)));
    }

    // Only for variable-size attributes (they have an extra file)
    if (array_schema->var_size(attribute_id))
      RETURN_NOT_OK(
          storage_manager->close_file(fragment_->attr_var_uri(attribute_id)));
  }

  // Success
  return Status::Ok();
}

Status WriteState::write(void** buffers, uint64_t* buffer_sizes) {
  // Sanity check
  if (buffers == nullptr || buffer_sizes == nullptr)
    return LOG_STATUS(Status::WriteStateError(
        "Cannot write; Invalid buffers or buffer sizes"));

  // Check if buffers are empty
  auto& attribute_ids = fragment_->query()->attribute_ids();
  auto attribute_id_num = (unsigned int)attribute_ids.size();
  bool empty_buffers = true;

  for (unsigned i = 0; i < attribute_id_num; ++i) {
    if (buffer_sizes[i] > 0) {
      empty_buffers = false;
      break;
    }
  }

  // If the buffers are empty, do nothing
  if (empty_buffers)
    return Status::Ok();

  // Create fragment directory (if it does not exist)
  auto storage_manager = fragment_->query()->storage_manager();
  auto& fragment_uri = fragment_->fragment_uri();
  RETURN_NOT_OK(storage_manager->create_dir(fragment_uri));

  // Dispatch the proper write command
  Layout layout = fragment_->query()->layout();
  if (layout == Layout::GLOBAL_ORDER || layout == Layout::COL_MAJOR ||
      layout == Layout::ROW_MAJOR) {  // Ordered
    // For easy reference
    auto array_schema = fragment_->query()->array_schema();

    // Write each attribute individually
    unsigned int buffer_i = 0;
    for (unsigned int i = 0; i < attribute_id_num; ++i) {
      if (!array_schema->var_size(attribute_ids[i])) {  // FIXED CELLS
        RETURN_NOT_OK(write_attr(
            attribute_ids[i], buffers[buffer_i], buffer_sizes[buffer_i]));
        ++buffer_i;
      } else {  // VARIABLE-SIZED CELLS
        RETURN_NOT_OK(write_attr_var(
            attribute_ids[i],
            buffers[buffer_i],  // offsets
            buffer_sizes[buffer_i],
            buffers[buffer_i + 1],  // actual cell values
            buffer_sizes[buffer_i + 1]));
        buffer_i += 2;
      }
    }

    // Success
    return Status::Ok();
  }

  if (layout == Layout::UNORDERED)  // UNORDERED
    return write_sparse_unordered(buffers, buffer_sizes);

  return LOG_STATUS(
      Status::WriteStateError("Cannot write to fragment; Invalid mode"));
}

/* ****************************** */
/*         PRIVATE METHODS        */
/* ****************************** */

template <class T>
void WriteState::expand_mbr(const T* coords) {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  auto attribute_num = array_schema->attribute_num();
  auto dim_num = array_schema->dim_num();
  auto mbr = static_cast<T*>(mbr_);

  // Initialize MBR
  if (tile_cell_num_[attribute_num] == 0) {
    for (unsigned int i = 0; i < dim_num; ++i) {
      mbr[2 * i] = coords[i];
      mbr[2 * i + 1] = coords[i];
    }
  } else {  // Expand MBR
    utils::expand_mbr(mbr, coords, dim_num);
  }
}

Status WriteState::init_tiles() {
  auto array_schema = fragment_->query()->array_schema();
  auto attribute_num = array_schema->attribute_num();
  auto tile = (Tile*)nullptr;

  for (unsigned int i = 0; i < attribute_num; ++i) {
    auto attr = array_schema->attribute(i);
    bool var_size = attr->var_size();

    tile = new Tile();
    RETURN_NOT_OK_ELSE(
        tile->init(
            (var_size) ? constants::cell_var_offset_type : attr->type(),
            (var_size) ? array_schema->cell_var_offsets_compression() :
                         attr->compressor(),
            (var_size) ? array_schema->cell_var_offsets_compression_level() :
                         attr->compression_level(),
            fragment_->tile_size(i),
            (var_size) ? constants::cell_var_offset_size : attr->cell_size(),
            0),
        delete tile);
    tiles_.emplace_back(tile);

    if (var_size) {
      tile = new Tile();
      RETURN_NOT_OK_ELSE(
          tile->init(
              attr->type(),
              attr->compressor(),
              attr->compression_level(),
              fragment_->tile_size(i),
              datatype_size(attr->type()),
              0),
          delete tile);
      tiles_var_.emplace_back(tile);
    } else {
      tiles_var_.emplace_back(nullptr);
    }
  }

  tile = new Tile();
  RETURN_NOT_OK_ELSE(
      tile->init(
          array_schema->coords_type(),
          array_schema->coords_compression(),
          array_schema->coords_compression_level(),
          fragment_->tile_size(array_schema->attribute_num()),
          array_schema->coords_size(),
          array_schema->domain()->dim_num()),
      delete tile);
  tiles_.emplace_back(tile);

  return Status::Ok();
}

Status WriteState::init_tile_io() {
  auto array_schema = fragment_->query()->array_schema();
  auto attribute_num = array_schema->attribute_num();
  auto query = fragment_->query();
  for (unsigned int i = 0; i < attribute_num; ++i) {
    bool var_size = array_schema->var_size(i);
    tile_io_.emplace_back(
        new TileIO(query->storage_manager(), fragment_->attr_uri(i)));
    if (var_size) {
      tile_io_var_.emplace_back(
          new TileIO(query->storage_manager(), fragment_->attr_var_uri(i)));
    } else {
      tiles_var_.emplace_back(nullptr);
      tile_io_var_.emplace_back(nullptr);
    }
  }
  tile_io_.emplace_back(
      new TileIO(query->storage_manager(), fragment_->coords_uri()));

  return Status::Ok();
}

void WriteState::sort_cell_pos(
    const void* buffer,
    uint64_t buffer_size,
    std::vector<uint64_t>* cell_pos) const {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  Datatype coords_type = array_schema->coords_type();

  // Invoke the proper templated function
  if (coords_type == Datatype::INT32)
    sort_cell_pos<int>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::INT64)
    sort_cell_pos<int64_t>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::FLOAT32)
    sort_cell_pos<float>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::FLOAT64)
    sort_cell_pos<double>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::INT8)
    sort_cell_pos<int8_t>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::UINT8)
    sort_cell_pos<uint8_t>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::INT16)
    sort_cell_pos<int16_t>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::UINT16)
    sort_cell_pos<uint16_t>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::UINT32)
    sort_cell_pos<uint32_t>(buffer, buffer_size, cell_pos);
  else if (coords_type == Datatype::UINT64)
    sort_cell_pos<uint64_t>(buffer, buffer_size, cell_pos);
}

template <class T>
void WriteState::sort_cell_pos(
    const void* buffer,
    uint64_t buffer_size,
    std::vector<uint64_t>* cell_pos) const {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  auto dim_num = array_schema->dim_num();
  uint64_t coords_size = array_schema->coords_size();
  uint64_t buffer_cell_num = buffer_size / coords_size;
  Layout cell_order = array_schema->cell_order();
  auto buffer_T = static_cast<const T*>(buffer);
  auto domain = array_schema->domain();

  // Populate cell_pos
  cell_pos->resize(buffer_cell_num);
  for (uint64_t i = 0; i < buffer_cell_num; ++i)
    (*cell_pos)[i] = i;

  // Invoke the proper sort function, based on the cell order
  if (domain->tile_extents() == nullptr) {  // NO TILE GRID
    // Sort cell positions
    switch (cell_order) {
      case Layout::ROW_MAJOR:
        std::sort(
            cell_pos->begin(),
            cell_pos->end(),
            SmallerRow<T>(buffer_T, dim_num));
        break;
      case Layout::COL_MAJOR:
        std::sort(
            cell_pos->begin(),
            cell_pos->end(),
            SmallerCol<T>(buffer_T, dim_num));
        break;
      default:  // Error
        assert(0);
    }
  } else {  // TILE GRID
    // Get tile ids
    std::vector<uint64_t> ids;
    ids.resize(buffer_cell_num);
    for (uint64_t i = 0; i < buffer_cell_num; ++i)
      ids[i] = domain->tile_id<T>(&buffer_T[i * dim_num], (T*)tile_coords_aux_);
    // Sort cell positions
    switch (cell_order) {
      case Layout::ROW_MAJOR:
        std::sort(
            cell_pos->begin(),
            cell_pos->end(),
            SmallerIdRow<T>(buffer_T, dim_num, ids));
        break;
      case Layout::COL_MAJOR:
        std::sort(
            cell_pos->begin(),
            cell_pos->end(),
            SmallerIdCol<T>(buffer_T, dim_num, ids));
        break;
      default:  // Error
        assert(0);
    }
  }
}

Status WriteState::update_metadata(const void* buffer, uint64_t buffer_size) {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  Datatype coords_type = array_schema->coords_type();

  // Invoke the proper templated function
  switch (coords_type) {
    case Datatype::INT32:
      return update_metadata<int>(buffer, buffer_size);
    case Datatype::INT64:
      return update_metadata<int64_t>(buffer, buffer_size);
    case Datatype::FLOAT32:
      return update_metadata<float>(buffer, buffer_size);
    case Datatype::FLOAT64:
      return update_metadata<double>(buffer, buffer_size);
    case Datatype::INT8:
      return update_metadata<int8_t>(buffer, buffer_size);
    case Datatype::UINT8:
      return update_metadata<uint8_t>(buffer, buffer_size);
    case Datatype::INT16:
      return update_metadata<int16_t>(buffer, buffer_size);
    case Datatype::UINT16:
      return update_metadata<uint16_t>(buffer, buffer_size);
    case Datatype::UINT32:
      return update_metadata<uint32_t>(buffer, buffer_size);
    case Datatype::UINT64:
      return update_metadata<uint64_t>(buffer, buffer_size);
    default:
      return LOG_STATUS(Status::WriteStateError(
          "Cannot update metadata; Invalid coordinates type"));
  }
}

template <class T>
Status WriteState::update_metadata(const void* buffer, uint64_t buffer_size) {
  // Trivial case
  if (buffer_size == 0)
    return Status::Ok();

  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  auto attribute_num = array_schema->attribute_num();
  auto dim_num = array_schema->dim_num();
  auto capacity = array_schema->capacity();
  auto coords_size = array_schema->coords_size();
  auto buffer_cell_num = buffer_size / coords_size;
  auto buffer_T = static_cast<const T*>(buffer);
  uint64_t& tile_cell_num = tile_cell_num_[attribute_num];

  // Update bounding coordinates and MBRs
  for (uint64_t i = 0; i < buffer_cell_num; ++i) {
    // Set first bounding coordinates
    if (tile_cell_num == 0)
      std::memcpy(bounding_coords_, &buffer_T[i * dim_num], coords_size);

    // Expand MBR
    expand_mbr(&buffer_T[i * dim_num]);

    // Advance a cell
    ++tile_cell_num;

    assert(buffer_cell_num != 0);

    // Set second bounding coordinates
    if (i == buffer_cell_num - 1 || tile_cell_num == capacity)
      std::memcpy(
          static_cast<char*>(bounding_coords_) + coords_size,
          &buffer_T[i * dim_num],
          coords_size);

    // Send MBR and bounding coordinates to metadata
    if (tile_cell_num == capacity) {
      RETURN_NOT_OK(metadata_->append_mbr<T>(mbr_));
      metadata_->append_bounding_coords(bounding_coords_);
      tile_cell_num = 0;
    }
  }

  return Status::Ok();
}

Status WriteState::write_attr(
    unsigned int attribute_id, void* buffer, uint64_t buffer_size) {
  // Trivial case
  if (buffer_size == 0)
    return Status::Ok();

  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  auto attribute_num = array_schema->attribute_num();

  // Update metadata in the case of sparse fragment coordinates
  if (attribute_id == attribute_num)
    RETURN_NOT_OK(update_metadata(buffer, buffer_size));

  // Preparation
  auto buf = new ConstBuffer(buffer, buffer_size);
  auto tile = tiles_[attribute_id];
  auto tile_io = tile_io_[attribute_id];

  // Fill tiles and dispatch them for writing
  uint64_t bytes_written = 0;
  do {
    RETURN_NOT_OK(tile->write(buf));
    if (tile->full()) {
      RETURN_NOT_OK(tile_io->write(tile, &bytes_written));
      metadata_->append_tile_offset(attribute_id, bytes_written);
      tile->reset_offset();
      tile->set_size(0);
    }
  } while (!buf->end());

  // Clean up
  delete buf;

  cells_written_[attribute_id] +=
      buffer_size / array_schema->cell_size(attribute_id);

  return Status::Ok();
}

Status WriteState::write_attr_last(unsigned int attribute_id) {
  auto tile = tiles_[attribute_id];
  assert(!tile->empty());
  auto tile_io = tile_io_[attribute_id];

  // Fill tiles and dispatch them for writing
  uint64_t bytes_written;
  RETURN_NOT_OK(tile_io->write(tile, &bytes_written));
  metadata_->append_tile_offset(attribute_id, bytes_written);
  tile->reset_offset();

  auto array_schema = fragment_->query()->array_schema();
  cells_written_[attribute_id] +=
      tile->size() / array_schema->cell_size(attribute_id);

  return Status::Ok();
}

Status WriteState::write_attr_var(
    unsigned int attribute_id,
    void* buffer,
    uint64_t buffer_size,
    void* buffer_var,
    uint64_t buffer_var_size) {
  // Trivial case
  if (buffer_size == 0 || buffer_var_size == 0)
    return Status::Ok();
  assert(buffer != nullptr && buffer_var != nullptr);

  auto buf = new ConstBuffer(buffer, buffer_size);
  auto buf_var = new ConstBuffer(buffer_var, buffer_var_size);

  uint64_t& buffer_var_offset = buffer_var_offsets_[attribute_id];

  auto tile = tiles_[attribute_id];
  auto tile_var = tiles_var_[attribute_id];
  auto tile_io = tile_io_[attribute_id];
  auto tile_io_var = tile_io_var_[attribute_id];

  // Fill tiles and dispatch them for writing
  uint64_t bytes_written = 0;
  uint64_t bytes_written_var = 0;
  auto current_var_offset = buffer_var_offset;
  do {
    RETURN_NOT_OK(tile->write_with_shift(buf, buffer_var_offset));
    uint64_t bytes_to_write_var =
        (buf->end()) ?
            buffer_var_offset + buffer_var_size - current_var_offset :
            buffer_var_offset + buf->value<uint64_t>() - current_var_offset;

    RETURN_NOT_OK(tile_var->write(buf_var, bytes_to_write_var));

    current_var_offset += bytes_to_write_var;

    if (tile->full()) {
      RETURN_NOT_OK(tile_io->write(tile, &bytes_written));
      RETURN_NOT_OK(tile_io_var->write(tile_var, &bytes_written_var));
      metadata_->append_tile_offset(attribute_id, bytes_written);
      metadata_->append_tile_var_offset(attribute_id, bytes_written_var);
      metadata_->append_tile_var_size(attribute_id, tile_var->size());
      tile->reset_offset();
      tile->set_size(0);
      tile_var->reset_offset();
      tile_var->set_size(0);
    }
  } while (!buf->end());

  buffer_var_offset += buffer_var_size;

  // Clean up
  delete buf;
  delete buf_var;

  cells_written_[attribute_id] += buffer_size / constants::cell_var_offset_size;

  return Status::Ok();
}

Status WriteState::write_attr_var_last(unsigned int attribute_id) {
  auto tile = tiles_[attribute_id];
  auto tile_var = tiles_var_[attribute_id];
  auto tile_io = tile_io_[attribute_id];
  auto tile_io_var = tile_io_var_[attribute_id];

  // Fill tiles and dispatch them for writing
  uint64_t bytes_written, bytes_written_var;
  RETURN_NOT_OK(tile_io->write(tile, &bytes_written));
  RETURN_NOT_OK(tile_io_var->write(tile_var, &bytes_written_var));
  metadata_->append_tile_offset(attribute_id, bytes_written);
  metadata_->append_tile_var_offset(attribute_id, bytes_written_var);
  metadata_->append_tile_var_size(attribute_id, tile_var->size());
  tile->reset_offset();
  tile_var->reset_offset();

  cells_written_[attribute_id] +=
      tile->size() / constants::cell_var_offset_size;

  return Status::Ok();
}

Status WriteState::write_last_tile() {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  auto attribute_num = array_schema->attribute_num();

  // Send last MBR, bounding coordinates and tile cell number to metadata
  metadata_->append_mbr(mbr_);
  metadata_->append_bounding_coords(bounding_coords_);
  metadata_->set_last_tile_cell_num(tile_cell_num_[attribute_num]);

  // Flush the last tile for each compressed attribute (it is still in main
  // memory
  for (unsigned int i = 0; i < attribute_num + 1; ++i) {
    RETURN_NOT_OK(write_attr_last(i));
    if (array_schema->var_size(i))
      RETURN_NOT_OK(write_attr_var_last(i));
  }

  // Success
  return Status::Ok();
}

Status WriteState::write_sparse_unordered(
    void** buffers, uint64_t* buffer_sizes) {
  // For easy reference
  auto query = fragment_->query();
  auto array_schema = query->array_schema();
  auto& attribute_ids = query->attribute_ids();
  auto attribute_id_num = (int)attribute_ids.size();

  // Find the coordinates buffer
  int coords_buffer_i = -1;
  RETURN_NOT_OK(query->coords_buffer_i(&coords_buffer_i));

  // Sort cell positions
  std::vector<uint64_t> cell_pos;
  sort_cell_pos(
      buffers[coords_buffer_i], buffer_sizes[coords_buffer_i], &cell_pos);

  // Write each attribute individually
  int buffer_i = 0;
  for (int i = 0; i < attribute_id_num; ++i) {
    if (!array_schema->var_size(attribute_ids[i])) {  // FIXED CELLS
      RETURN_NOT_OK(write_sparse_unordered_attr(
          attribute_ids[i],
          buffers[buffer_i],
          buffer_sizes[buffer_i],
          cell_pos));
      ++buffer_i;
    } else {  // VARIABLE-SIZED CELLS
      RETURN_NOT_OK(write_sparse_unordered_attr_var(
          attribute_ids[i],
          buffers[buffer_i],  // offsets
          buffer_sizes[buffer_i],
          buffers[buffer_i + 1],  // actual values
          buffer_sizes[buffer_i + 1],
          cell_pos));
      buffer_i += 2;
    }
  }

  // Success
  return Status::Ok();
}

Status WriteState::write_sparse_unordered_attr(
    unsigned int attribute_id,
    void* buffer,
    uint64_t buffer_size,
    const std::vector<uint64_t>& cell_pos) {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  uint64_t cell_size = array_schema->cell_size(attribute_id);

  // Check number of cells in buffer
  uint64_t buffer_cell_num = buffer_size / cell_size;
  if (buffer_cell_num != uint64_t(cell_pos.size())) {
    return LOG_STATUS(Status::WriteStateError(
        std::string("Cannot write sparse unordered; Invalid number of "
                    "cells in attribute '") +
        array_schema->attribute_name(attribute_id) + "'"));
  }

  // Allocate a local buffer to hold the sorted cells
  auto sorted_buf = new Buffer();
  auto buffer_c = static_cast<const char*>(buffer);

  // Sort and write attribute values in batches
  Status st;
  for (uint64_t i = 0; i < buffer_cell_num; ++i) {
    // Write batch
    if (sorted_buf->offset() + cell_size > constants::sorted_buffer_size) {
      RETURN_NOT_OK_ELSE(
          write_attr(attribute_id, sorted_buf->data(), sorted_buf->offset()),
          delete sorted_buf);
      sorted_buf->reset_offset();
      sorted_buf->set_size(0);
    }

    // Keep on copying the cells in the sorted order in the sorted buffer
    RETURN_NOT_OK(
        sorted_buf->write(buffer_c + cell_pos[i] * cell_size, cell_size));
  }

  // Write final batch
  if (sorted_buf->offset() != 0) {
    RETURN_NOT_OK_ELSE(
        write_attr(attribute_id, sorted_buf->data(), sorted_buf->offset()),
        delete sorted_buf);
  }

  // Clean up
  delete sorted_buf;

  return Status::Ok();
}

Status WriteState::write_sparse_unordered_attr_var(
    unsigned int attribute_id,
    void* buffer,
    uint64_t buffer_size,
    void* buffer_var,
    uint64_t buffer_var_size,
    const std::vector<uint64_t>& cell_pos) {
  // For easy reference
  auto array_schema = fragment_->query()->array_schema();
  uint64_t cell_size = constants::cell_var_offset_size;
  auto buffer_s = static_cast<const uint64_t*>(buffer);
  auto buffer_var_c = static_cast<const char*>(buffer_var);

  // Check number of cells in buffer
  uint64_t buffer_cell_num = buffer_size / cell_size;
  if (buffer_cell_num != uint64_t(cell_pos.size())) {
    return LOG_STATUS(Status::WriteStateError(
        std::string("Cannot write sparse unordered variable; "
                    "Invalid number of cells in attribute '") +
        array_schema->attribute_name(attribute_id) + "'"));
  }

  auto sorted_buf = new Buffer();
  auto sorted_buf_var = new Buffer();

  // Sort and write attribute values in batches
  Status st;
  uint64_t var_offset;
  for (uint64_t i = 0; i < buffer_cell_num; ++i) {
    // Calculate variable cell size
    uint64_t cell_var_size =
        (cell_pos[i] == buffer_cell_num - 1) ?
            buffer_var_size - buffer_s[cell_pos[i]] :
            buffer_s[cell_pos[i] + 1] - buffer_s[cell_pos[i]];

    // Write batch
    if (sorted_buf->offset() + cell_size > constants::sorted_buffer_size ||
        sorted_buf_var->offset() + cell_var_size >
            constants::sorted_buffer_var_size) {
      st = write_attr_var(
          attribute_id,
          sorted_buf->data(),
          sorted_buf->offset(),
          sorted_buf_var->data(),
          sorted_buf_var->offset());

      if (!st.ok()) {
        delete sorted_buf;
        delete sorted_buf_var;
        return st;
      }

      sorted_buf->reset_offset();
      sorted_buf->set_size(0);
      sorted_buf_var->reset_offset();
      sorted_buf_var->set_size(0);
    }

    // Keep on copying the cells in sorted order in the sorted buffers
    var_offset = sorted_buf_var->offset();
    RETURN_NOT_OK(sorted_buf->write(&var_offset, cell_size));
    RETURN_NOT_OK(sorted_buf_var->write(
        buffer_var_c + buffer_s[cell_pos[i]], cell_var_size));
  }

  // Write final batch
  if (sorted_buf->offset() != 0)
    st = write_attr_var(
        attribute_id,
        sorted_buf->data(),
        sorted_buf->offset(),
        sorted_buf_var->data(),
        sorted_buf_var->offset());

  // Clean up
  delete sorted_buf;
  delete sorted_buf_var;

  // Success
  return st;
}

}  // namespace sm
}  // namespace tiledb
