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

#include <iterator> // for std::begin, std::end
#include <numeric>  // for std::numeric
#include <random>   // for std::mt19937
#include <vector>   // for std::vector
#include <cstddef>  // for std::size_t

using qfilter = quofil::quotient_filter_fp;

TEST(quotient_filter_fp, WorksWell) {
  const size_t q_bits = 13;
  const size_t r_bits = 5;
  const size_t fp_bits = q_bits + r_bits;
  const size_t max_fp = (size_t(1) << fp_bits) - 1;
  std::vector<unsigned> fingerprints(max_fp);

  // std::random_device rd;
  std::mt19937 gen;

  std::iota(begin(fingerprints), end(fingerprints), size_t(0));
  std::shuffle(begin(fingerprints), end(fingerprints), gen);

  qfilter filter(q_bits, r_bits);

  const size_t elements_to_insert = filter.slots() / 3;
  for (size_t i = 0; i != elements_to_insert; ++i)
    EXPECT_TRUE(filter.insert(fingerprints[i])) << " i = " << i;

  EXPECT_EQ(elements_to_insert, filter.size());

  for (size_t i = 0; i != elements_to_insert; ++i)
    EXPECT_TRUE(filter.contains(fingerprints[i])) << " i = " << i;

  for (size_t i = elements_to_insert; i != filter.slots(); ++i)
    EXPECT_FALSE(filter.contains(fingerprints[i])) << " i = " << i;
}
