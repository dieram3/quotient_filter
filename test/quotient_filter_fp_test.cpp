//          Copyright Diego Ramírez July 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter_fp.hpp>
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
using value_t = qfilter::value_type;

template <class Function>
static void repeat(std::size_t n, Function f) {
  while (n--)
    f();
}

static auto make_fp_generator(const qfilter &filter) {
  const size_t fp_bits = filter.quotient_bits() + filter.remainder_bits();
  const value_t max_fp = (value_t{1} << fp_bits) - 1;

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<value_t> dist(0, max_fp);

  return [ gen = std::move(gen), dist ]() mutable { return dist(gen); };
}

static auto make_insertion_decision_generator(const qfilter &filter) {
  using param_t = std::bernoulli_distribution::param_type;
  std::bernoulli_distribution dist;
  std::default_random_engine gen;

  return [ gen = std::move(gen), dist, &filter ]() mutable {
    if (filter.empty())
      return true;
    if (filter.full())
      return false;
    const double load_factor = double(filter.size()) / filter.capacity();
    return dist(gen, param_t(1.0 - load_factor));
  };
}

TEST(quotient_filter_fp, WorksWell) {
  qfilter filter(13, 5); // q_bits and r_bits
  std::set<value_t> set;

  auto gen_fp = make_fp_generator(filter);

  repeat(filter.capacity() / 2, [&] {
    const auto fp = gen_fp();
    const auto pfilter = filter.insert(fp);
    const auto pset = set.insert(fp);
    EXPECT_EQ(pfilter.second, pset.second);
    EXPECT_EQ(*pfilter.first, fp);
  });

  EXPECT_EQ(filter.size(), set.size());

  for (value_t value : set)
    EXPECT_TRUE(filter.count(value));

  repeat(10000, [&] {
    const auto fp = gen_fp();
    EXPECT_EQ(set.count(fp), filter.count(fp));
  });
}

TEST(quotient_filter_fp, InsertionDeletionQueryTest) {
  qfilter filter(13, 2); // q_bits and r_bits
  std::set<value_t> set;

  auto gen_fp = make_fp_generator(filter);
  auto do_insertion = make_insertion_decision_generator(filter);

  repeat(3 * filter.capacity(), [&] {
    const auto fp = gen_fp();

    if (do_insertion()) {
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

  qfilter filter(10, 8); // q_bits and r_bits
  std::set<value_t> set;

  auto gen_fp = make_fp_generator(filter);

  while (!filter.full()) {
    const auto fp = gen_fp();
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

TEST(quotient_filter_fp, ClearWorksWell) {
  qfilter filter(9, 6); // q_bits and r_bits
  auto gen_fp = make_fp_generator(filter);
  repeat(filter.capacity(), [&] { filter.insert(gen_fp()); });

  const auto prev_q = filter.quotient_bits();
  const auto prev_r = filter.remainder_bits();
  const auto prev_cap = filter.capacity();
  filter.clear();

  EXPECT_EQ(prev_cap, filter.capacity());
  EXPECT_EQ(prev_q, filter.quotient_bits());
  EXPECT_EQ(prev_r, filter.remainder_bits());
  EXPECT_TRUE(filter.empty());
  EXPECT_FALSE(filter.full());
  EXPECT_TRUE(filter.begin() == filter.end());

  filter.insert(5);
  auto first = filter.begin();
  EXPECT_NE(first++, filter.end());
  EXPECT_EQ(first, filter.end());
  EXPECT_TRUE(filter.erase(5));

  std::set<value_t> set;
  while (!filter.full()) {
    const auto fp = gen_fp();
    if (set.insert(fp).second)
      EXPECT_TRUE(filter.insert(fp).second);
  }

  for (value_t fp : set)
    EXPECT_TRUE(filter.erase(fp));

  EXPECT_TRUE(filter.empty());
}

TEST(iterator, WorksWell) {
  qfilter filter(11, 6); // q_bits and r_bits
  std::set<value_t> set;

  auto gen_fp = make_fp_generator(filter);

  repeat(filter.capacity(), [&] {
    const auto fp = gen_fp();
    filter.insert(fp);
    set.insert(fp);
  });

  EXPECT_EQ(*filter.begin(), *set.begin());
  EXPECT_TRUE(std::equal(filter.begin(), filter.end(), set.begin(), set.end()));

  auto it = std::next(set.begin(), set.size() / 2);
  EXPECT_TRUE(std::equal(filter.find(*it), filter.end(), it, set.end()));
}

TEST(iterator, ContructorWorksWell) {
  qfilter filter(4, 4);
  const unsigned value1 = 3;
  filter.insert(value1); // Should be in the first slot.
  EXPECT_EQ(value1, *filter.begin());
  EXPECT_TRUE(filter.erase(value1));
  const unsigned value2 = 0b11'1111;
  filter.insert(value2);
  EXPECT_EQ(value2, *filter.begin());
}
