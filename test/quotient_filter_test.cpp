// The MIT License (MIT)
//
// Copyright (c) <June 2015> <Diego Ramirez and Marcello Tavano>
// any issue can mail to <(diego.ramirezd and marcello.tavanolanas)@mail.udp.cl>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <quofil/quotient_filter.hpp>
#include <gtest/gtest.h>

#include <random>  // for std::mt19937
#include <vector>  // for std::vector
#include <cstddef> // for std::size_t

TEST(bits_manager, WorksWell) {

  const size_t q_bits = 17;
  const size_t r_bits = 32 - q_bits;
  const size_t num_slots = 1 << q_bits;
  const size_t max_remainder = (1 << r_bits) - 1;

  std::mt19937 gen;
  std::uniform_int_distribution<size_t> dist(0, max_remainder);

  quofil::detail::bits_manager bm(q_bits, r_bits);
  std::vector<uint64_t> vec(num_slots);

  for (size_t i = 0; i < num_slots; ++i) {
    const auto remainder = dist(gen);
    bm.set_remainder(i, remainder);
    vec[i] = remainder;
  }

  for (size_t i = 0; i < num_slots; ++i)
    EXPECT_EQ(vec[i], bm.get_remainder(i)) << " i = " << i;
}
