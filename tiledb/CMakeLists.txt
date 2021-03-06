#
# tiledb/CMakeLists.txt
#
#
# The MIT License
#
# Copyright (c) 2017-2018 TileDB, Inc.
# Copyright (c) 2016 MIT and Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

############################################################
# CMake policies
############################################################

# C++ library, allow the VISIBLITY_PRESET for each c++ target to work
cmake_policy(SET CMP0063 NEW)

############################################################
# Source files
############################################################

# The core header directory.
set(TILEDB_CORE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}")

# List of API headers (to be installed)
set(TILEDB_PUBLIC_HEADERS
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/c_api/tiledb.h
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/c_api/tiledb_enum.h
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/c_api/tiledb_version.h
)

if (TILEDB_CPP_API)
  list(APPEND TILEDB_PUBLIC_HEADERS
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/tiledb
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/array.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/array_schema.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/attribute.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/compressor.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/config.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/config_iter.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/context.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/core_interface.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/deleter.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/dimension.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/domain.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/exception.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/filebuf.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/group.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map_item.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map_iter.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map_proxy.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map_schema.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/object.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/object_iter.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/query.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/schema_base.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/type.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/utils.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/version.h
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/vfs.h
  )
else()
  message(STATUS "TileDB C++ API is not built.")
endif()

# List of all core source files
set(TILEDB_CORE_SOURCES
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/array_schema/array_schema.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/array_schema/attribute.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/array_schema/dimension.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/array_schema/domain.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/buffer/buffer.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/buffer/const_buffer.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/c_api/tiledb.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cache/lru_cache.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/compressors/blosc_compressor.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/compressors/bzip_compressor.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/compressors/dd_compressor.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/compressors/gzip_compressor.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/compressors/lz4_compressor.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/compressors/rle_compressor.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/compressors/zstd_compressor.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/filesystem/hdfs_filesystem.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/filesystem/posix_filesystem.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/filesystem/s3.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/filesystem/vfs.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/filesystem/win_filesystem.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/fragment/fragment.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/fragment/fragment_metadata.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/fragment/read_state.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/fragment/write_state.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/kv/kv.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/kv/kv_item.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/kv/kv_iter.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/misc/constants.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/misc/logger.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/misc/stats.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/misc/status.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/misc/thread_pool.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/misc/uri.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/misc/utils.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/misc/win_constants.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/query/array_ordered_read_state.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/query/array_ordered_write_state.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/query/array_read_state.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/query/pq_fragment_cell_range.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/query/query.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/query/smaller_pq_fragment_cell_range.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/storage_manager/config.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/storage_manager/consolidator.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/storage_manager/locked_object.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/storage_manager/open_array.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/storage_manager/storage_manager.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/tile/tile.cc
  ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/tile/tile_io.cc
)

if (TILEDB_CPP_API)
  list(APPEND TILEDB_CORE_SOURCES
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/array.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/array_schema.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/attribute.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/config.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/config_iter.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/context.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/deleter.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/dimension.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/domain.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/filebuf.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/group.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map_item.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map_iter.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map_proxy.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/map_schema.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/object.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/object_iter.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/query.cc
    ${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api/vfs.cc
  )
endif()

# 'External' source files included in the source tree.
set(TILEDB_EXTERNALS_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/include")
set(TILEDB_EXTERNALS_SOURCES
  ${CMAKE_SOURCE_DIR}/external/src/md5/md5.cc
)

############################################################
# Build core objects as a reusable object library
############################################################

add_library(TILEDB_CORE_OBJECTS OBJECT 
    ${TILEDB_CORE_SOURCES}
    ${TILEDB_EXTERNALS_SOURCES}
)

# Compile all core sources with PIC
set_property(TARGET TILEDB_CORE_OBJECTS PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(TILEDB_CORE_OBJECTS
  PRIVATE
    "${TILEDB_CORE_INCLUDE_DIR}"
    "${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/c_api"
    "${TILEDB_EXTERNALS_INCLUDE_DIR}"
)

if (TILEDB_CPP_API)
  target_include_directories(TILEDB_CORE_OBJECTS
    PRIVATE
      "${TILEDB_CORE_INCLUDE_DIR}/tiledb/sm/cpp_api"
  )
endif()

############################################################
# Compile options/definitions
############################################################

if (SANITIZER)
  if (NOT CMAKE_BUILD_TYPE MATCHES "Debug")
    message(FATAL_ERROR "Sanitizers only enabled for Debug build")
  endif()
  # Enable sanitizers only on clang for now
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Sanitizers only enabled for the clang compiler")
  endif()
  string(TOLOWER ${SANITIZER} SANITIZER)
  if (NOT SANITIZER MATCHES "^(address|memory|thread|undefined)$")
    message(FATAL_ERROR "Unknown clang sanitizer: ${SANITIZER})")
  else()
    message(STATUS "The TileDB library is compiled with sanitizer ${SANITIZER} enabled")
  endif()
  target_compile_options(TILEDB_CORE_OBJECTS
    PRIVATE
      -g -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize=${SANITIZER}
  )
endif()

if (TILEDB_VERBOSE)
  add_definitions(-DTILEDB_VERBOSE)
  message(STATUS "The TileDB library is compiled with verbosity.")
endif()

############################################################
# Dependencies: set up includes/linking
############################################################

# Unfortunately, with CMake < 3.12, you can't use target_link_libraries()
# on TILEDB_CORE_OBJECTS. This workaround uses an interface library
# so we can use the targets created by the calls to find_package().
add_library(TILEDB_CORE_OBJECTS_ILIB INTERFACE)

# S3 dependencies
if (TILEDB_S3)
  message(STATUS "The TileDB library is compiled with S3 support.")
  find_package(Curl_EP REQUIRED)
  find_package(OpenSSL_EP REQUIRED)
  find_package(AWSSDK_EP REQUIRED)
  target_link_libraries(TILEDB_CORE_OBJECTS_ILIB
    INTERFACE
      AWSSDK::aws-cpp-sdk-s3
      AWSSDK::aws-cpp-sdk-core
  )
  if (NOT WIN32)
    # No Curl or OpenSSL required on Windows.
    target_link_libraries(TILEDB_CORE_OBJECTS_ILIB
      INTERFACE
        Curl::Curl
        OpenSSL::SSL
        OpenSSL::Crypto
    )
  endif()
  add_definitions(-DHAVE_S3)
endif()

# HDFS dependencies
if (TILEDB_HDFS)
  if(WIN32)
    message(FATAL_ERROR "TileDB HDFS backend is not supported for Windows builds currently")
  else()
    message(STATUS "The TileDB library is compiled with HDFS support.")
    add_definitions(-DHAVE_HDFS)
  endif()
endif()

# Sanitizer linker flags
if (SANITIZER)
  target_link_libraries(TILEDB_CORE_OBJECTS_ILIB
    INTERFACE
      -fsanitize=${SANITIZER}
  )
endif()

# Coverage linker flags
if (CMAKE_BUILD_TYPE MATCHES "Coverage")
  target_link_libraries(TILEDB_CORE_OBJECTS_ILIB
    INTERFACE
      --coverage
  )
endif()

if (NOT WIN32)
  # On Linux, must explicitly link -lpthread -ldl in order for static linking
  # to libzstd or libcurl to work.
  target_link_libraries(TILEDB_CORE_OBJECTS_ILIB INTERFACE pthread dl)
endif()

# Required dependencies. These come last as any prior dependencies relying on
# these must be listed first for Linux. E.g. AWSSDK and Curl depend on Zlib,
# which is installed here.
find_package(Blosc_EP REQUIRED)
find_package(Bzip2_EP REQUIRED)
find_package(LZ4_EP REQUIRED)
find_package(Spdlog_EP REQUIRED)
find_package(Zlib_EP REQUIRED)
find_package(Zstd_EP REQUIRED)
target_link_libraries(TILEDB_CORE_OBJECTS_ILIB
  INTERFACE
    Blosc::Blosc
    Bzip2::Bzip2
    LZ4::LZ4
    Spdlog::Spdlog
    Zlib::Zlib
    Zstd::Zstd
)

# Copy over dependency info (e.g. include directories) to the core objects.
target_compile_definitions(TILEDB_CORE_OBJECTS
  PRIVATE
    $<TARGET_PROPERTY:TILEDB_CORE_OBJECTS_ILIB,INTERFACE_COMPILE_DEFINITIONS>
)
target_include_directories(TILEDB_CORE_OBJECTS
  PRIVATE
    $<TARGET_PROPERTY:TILEDB_CORE_OBJECTS_ILIB,INTERFACE_INCLUDE_DIRECTORIES>
)

############################################################
# TileDB static and shared library targets
############################################################

add_library(tiledb_static STATIC $<TARGET_OBJECTS:TILEDB_CORE_OBJECTS>)
add_library(tiledb_shared SHARED $<TARGET_OBJECTS:TILEDB_CORE_OBJECTS>)

# Target properties
set_target_properties(tiledb_shared
  PROPERTIES
    OUTPUT_NAME "tiledb"
)
set_target_properties(tiledb_static
  PROPERTIES
    OUTPUT_NAME "tiledb"
)

# Link the dependencies specified earlier
target_link_libraries(tiledb_shared
  PUBLIC
    $<TARGET_PROPERTY:TILEDB_CORE_OBJECTS_ILIB,INTERFACE_LINK_LIBRARIES>
)

# Windows linker configuration
if (WIN32)
  set(WIN32_LIBS shlwapi)

  if (TILEDB_S3)
    list(APPEND WIN32_LIBS bcrypt winhttp wininet userenv version)
  endif()

  foreach (LIB ${WIN32_LIBS})
    find_library(LIB_${LIB} ${LIB})
    message(STATUS "Found Win32 lib ${LIB}: ${LIB_${LIB}}")
    target_link_libraries(tiledb_static PUBLIC ${LIB_${LIB}})
    target_link_libraries(tiledb_shared PUBLIC ${LIB_${LIB}})
  endforeach()
  # On Windows we must name the static library something else to avoid
  # name clash with the DLL's "import library" .lib file.
  set_target_properties(tiledb_static PROPERTIES OUTPUT_NAME "tiledbstatic")
endif()

############################################################
# API symbol exports (and public headers for install)
############################################################

include(GenerateExportHeader)

# Generates the file 'tiledb_export.h' suitable for the current compiler.
generate_export_header(TILEDB_CORE_OBJECTS
  BASE_NAME tiledb
)

# Set variables in the parent scope so the tests and examples can reference it.
set(TILEDB_EXPORT_HEADER_NAME "tiledb_export.h" PARENT_SCOPE)
set(TILEDB_EXPORT_HEADER_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)
set(TILEDB_EXPORT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/tiledb_export.h")
set(TILEDB_EXPORT_HEADER "${TILEDB_EXPORT_HEADER}" PARENT_SCOPE)

# Set related compiler settings
target_compile_definitions(TILEDB_CORE_OBJECTS PRIVATE -DTILEDB_CORE_OBJECTS_EXPORTS)
target_include_directories(TILEDB_CORE_OBJECTS PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# Add the generated header to the public headers list
list(APPEND TILEDB_PUBLIC_HEADERS
  ${TILEDB_EXPORT_HEADER}
)

# Set the public headers, which are the ones that get installed.
set_target_properties(tiledb_shared
  PROPERTIES
    PUBLIC_HEADER "${TILEDB_PUBLIC_HEADERS}"
)
set_target_properties(tiledb_static
  PROPERTIES
    PUBLIC_HEADER "${TILEDB_PUBLIC_HEADERS}"
)

############################################################
# Installation
############################################################

# Install libraries.
# Note on Windows, the DLL counts as "runtime" and should go into bin.
install(
    TARGETS tiledb_static tiledb_shared
    PUBLIC_HEADER DESTINATION include/tiledb
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)