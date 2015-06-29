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

#include <algorithm> // for std::equal
#include <iterator>  // for std::begin, std::end
#include <numeric>   // for std::numeric
#include <random>    // for std::mt19937
#include <tuple>
#include <vector>  // for std::vector
#include <cstddef> // for std::size_t

#include <unordered_set>
using qfilter = quofil::quotient_filter_fp;
using ulong = unsigned long;

template <class Function>
static void repeat(std::size_t n, Function f) {
  while (n--)
    f();
}

// Returns the max value representable with the given bits.
static ulong max_value(std::size_t bits) { return (1UL << bits) - 1; }

TEST(quotient_filter_fp, WorksWell) {
  const size_t q_bits = 13;
  const size_t r_bits = 5;
  const size_t fp_bits = q_bits + r_bits;
  const ulong max_fp = max_value(fp_bits);

  qfilter filter(q_bits, r_bits);
  std::set<ulong> set;

  const size_t max_elements_to_insert = filter.capacity() / 2;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<ulong> dist(0, max_fp);

  for (size_t i = 0; i != max_elements_to_insert; ++i) {
    const auto fp = dist(gen);
    const auto pfilter = filter.insert(fp);
    const auto pset = set.insert(fp);
    EXPECT_EQ(pfilter.second, pset.second);
    EXPECT_EQ(*pfilter.first, fp);
  }

  EXPECT_EQ(filter.size(), set.size());

  for (ulong value : set)
    EXPECT_TRUE(filter.count(value));

  repeat(10000, [&] {
    const auto fp = dist(gen);
    EXPECT_EQ(set.count(fp), filter.count(fp));
  });
}

TEST(quotient_filter_fp, InsertionDeletionQueryTest) {
  const size_t q_bits = 13;
  const size_t r_bits = 2;
  const size_t fp_bits = q_bits + r_bits;
  const ulong max_fp = max_value(fp_bits);

  qfilter filter(q_bits, r_bits);
  std::set<ulong> set;

  std::random_device rd;
  std::mt19937 fp_gen(rd());
  std::default_random_engine bern_gen(rd());

  std::bernoulli_distribution insert_dist;
  using param_t = std::bernoulli_distribution::param_type;
  std::uniform_int_distribution<ulong> fp_dist(0, max_fp);

  repeat(3 * filter.capacity(), [&] {
    const double load_factor = double(filter.size()) / filter.capacity();
    const bool do_insertion =
        filter.empty()
            ? true
            : filter.full() ? false
                            : insert_dist(bern_gen, param_t(1.0 - load_factor));

    const auto fp = fp_dist(fp_gen);

    if (do_insertion) {
      const auto pfilter = filter.insert(fp);
      const auto pset = set.insert(fp);
      EXPECT_EQ(pset.second, pfilter.second);
      EXPECT_EQ(fp, *pfilter.first);
    } else {
      const auto ans_filter = filter.erase(fp);
      const auto ans_set = set.erase(fp);
      EXPECT_EQ(ans_set, ans_filter);
    }
    EXPECT_EQ(set.size(), filter.size());
  });
}

TEST(quotient_filter_fp, WorksWellWhenFull) {
  const size_t q_bits = 10;
  const size_t r_bits = 8;
  const size_t fp_bits = q_bits + r_bits;
  const ulong max_fp = max_value(fp_bits);

  qfilter filter(q_bits, r_bits);
  std::set<ulong> set;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<ulong> dist(0, max_fp);

  while (!filter.full()) {
    const auto fp = dist(gen);
    if (set.insert(fp).second)
      filter.insert(fp);
  }

  EXPECT_EQ(filter.capacity(), filter.size());
  EXPECT_THROW(filter.insert(*set.begin()), quofil::filter_is_full);

  for (auto fp : set)
    EXPECT_TRUE(filter.erase(fp));

  EXPECT_TRUE(filter.empty());
  EXPECT_EQ(0, filter.size());
}

TEST(iterator, WorksWell) {
  const size_t q_bits = 11;
  const size_t r_bits = 6;
  const size_t fp_bits = q_bits + r_bits;
  const ulong max_fp = max_value(fp_bits);

  qfilter filter(q_bits, r_bits);
  std::set<ulong> set;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<ulong> dist(0, max_fp);

  repeat(filter.capacity(), [&] {
    const auto new_value = dist(gen);
    filter.insert(new_value);
    set.insert(new_value);
  });

  EXPECT_EQ(*filter.begin(), *set.begin());
  EXPECT_TRUE(std::equal(filter.begin(), filter.end(), set.begin(), set.end()));

  auto it = std::next(set.begin(), set.size() / 2);
  EXPECT_TRUE(std::equal(filter.find(*it), filter.end(), it, set.end()));
}
