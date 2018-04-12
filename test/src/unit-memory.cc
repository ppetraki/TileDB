/**
 * @file unit-memory.cc
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
 * Tests the make_unique helpers missing from C++11.
 */

#include <memory>
#include <functional>

#include <string.h>

#include <catch.hpp>
#include "tiledb/sm/misc/memory.h"

struct Tracer {
  Tracer()
      : _f([]() {} ) {
  }

  explicit Tracer(std::function<void()>&& f)
      :_f(std::move(f)) {}

  ~Tracer() {
    _f();
  }

  std::function<void()> _f;
};

using namespace std;

TEST_CASE("Memory: Test make_unique<T>", "[memory]") {
  // Show that the allocated object was destructed by scope

  int destroyed = 0;

  {
    auto ptr = make_unique<Tracer>([&]() { ++destroyed; } );
  }

  CHECK(destroyed == 1);
}

TEST_CASE("Memory: Test make_unique<T[]>", "[memory]") {
  // Show that the allocated object was destructed by scope
  int destroyed = 0;
  {
    unique_ptr<Tracer[]> tracers = make_unique<Tracer[]>(10);
    for (int i=0; i < 10; ++i) {
        tracers[i]._f = [&]() { ++destroyed; };
    }
  }

  CHECK(destroyed == 10);
}

TEST_CASE("Memory: Test make_unique<unsigned char[]> is zeroed", "[memory]") {
  char buffer[4096];
  memset(buffer, 0, 4096);

  auto p = make_unique<unsigned char[]>(4096);

  CHECK(memcmp(p.get(), buffer, 4096) == 0);

}
