/**
 * @file   tile.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 TileDB, Inc.
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
 * This file defines class Tile.
 */

#ifndef TILEDB_TILE_H
#define TILEDB_TILE_H

#include "attribute.h"
#include "buffer.h"
#include "const_buffer.h"
#include "status.h"

#include <cinttypes>

namespace tiledb {

/**
 * Handles tile information. A tile can be in main memory if it has been
 * fetched from the disk or has been mmap-ed from a file. However, a tile
 * can be solely on the disk, in which case the Tile object stores the
 * offset in the file where the tile data begin, plus the tile size.
 */
class Tile {
 public:
  /* ********************************* */
  /*     CONSTRUCTORS & DESTRUCTORS    */
  /* ********************************* */

  /** Constructor. */
  Tile();

  /**
   * Constructor.
   *
   * @param type The type of the data to be stored.
   * @param compression The compression type.
   * @param compression_level The compression level.
   * @param tile_size The tile size.
   * @param cell_size The cell size.
   */
  Tile(
      Datatype type,
      Compressor compression,
      int compression_level,
      uint64_t tile_size,
      uint64_t cell_size);

  /**
   * Constructor.
   *
   * @param type The type of the data to be stored.
   * @param compression The compression type.
   * @param cell_size The cell size.
   */
  Tile(Datatype type, Compressor compression, uint64_t cell_size);

  /** Destructor. */
  ~Tile();

  /* ********************************* */
  /*                API                */
  /* ********************************* */

  /** Allocates memory of the input size. */
  Status alloc(uint64_t size);

  /** Returns the internal buffer. */
  Buffer* buffer() const;

  /** Returns the cell size. */
  uint64_t cell_size() const;

  /** Returns the tile compressor. */
  Compressor compressor() const;

  /** Returns the tile compression level. */
  int compression_level() const;

  /** Returns the tile data. */
  void* data() const;

  /** Checks if the tile is empty. */
  bool empty() const;

  /** Checks if the tile is full. */
  bool full() const;

  /** The current offset in the tile. */
  uint64_t offset() const;

  /** Reads from the tile into the input buffer *nbytes*. */
  Status read(void* buffer, uint64_t nbytes);

  /** Resets the tile offset. */
  void reset_offset();

  /** Sets the tile offset. */
  void set_offset(uint64_t offset);

  /** Returns the tile size. */
  uint64_t size() const;

  /** Returns the tile data type. */
  Datatype type() const;

  /** Returns the value of type T in the tile at the input offset. */
  template <class T>
  inline T value(uint64_t offset) const {
    return buffer_->value<T>(offset);
  }

  /** Returns the value of type T in the tile at the current offset. */
  template <class T>
  inline T value() const {
    return buffer_->value<T>();
  }

  /** Writes as much data as possibly can be read from the input buffer. */
  Status write(ConstBuffer* buf);

  /**
   * Writes exactly *nbytes* from the input buffer to the local buffer.
   * The local buffer can be potentially expanded to fit these bytes.
   */
  Status write(ConstBuffer* buf, uint64_t nbytes);

  /**
   * Writes as much data as possibly can be read from the input buffer.
   * Note that this is a special function where each read value (of type
   * uint64_t) is added to the input offset prior to being written to
   * the tile local buffer.
   */
  Status write_with_shift(ConstBuffer* buf, uint64_t offset);

 private:
  /* ********************************* */
  /*         PRIVATE ATTRIBUTES        */
  /* ********************************* */

  /** Local buffer that stores the tile data. */
  Buffer* buffer_;

  /** The cell size. */
  uint64_t cell_size_;

  /** The compression type. */
  Compressor compressor_;

  /** The compression level. */
  int compression_level_;

  /** The current offset in the tile. */
  uint64_t offset_;

  /** The tile size. */
  uint64_t tile_size_;

  /** The tile data type. */
  Datatype type_;

  /* ********************************* */
  /*          PRIVATE METHODS          */
  /* ********************************* */
};

}  // namespace tiledb

#endif  // TILEDB_TILE_H
