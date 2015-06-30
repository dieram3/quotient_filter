//          Copyright Diego Ramírez July 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter_fp.hpp>
#include <gtest/gtest.h>

#include <algorithm> // for std::equal
#include <iterator>  // for std::begin, std::end, std::next
#include <numeric>   // for std::numeric_limits
#include <random>    // for std::mt19937
#include <utility>   // for std::move
#include <vector>    // for std::vector
#include <cstddef>   // for std::size_t

#define QFILTER_TEST(test_name) TEST(quotient_filter_fp, test_name)

#define QF_ITERATOR_TEST(test_name) TEST(quotient_filter_fp_iterator, test_name)

using qfilter = quofil::quotient_filter_fp;
using value_t = qfilter::value_type;
using std::size_t;
using set_t = std::set<value_t>;

template <class Function>
static void repeat(size_t n, Function f) {
  while (n--)
    f();
}

template <class Range1, class Range2>
static bool equal(const Range1 &lhs, const Range2 &rhs) {
  return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs),
                    std::end(rhs));
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

// A filter with a small r_bits is difficult to fill.
static void populate(qfilter &filter, size_t insertion_tries = -1) {
  auto gen_fp = make_fp_generator(filter);
  while (!filter.full() && insertion_tries) {
    filter.insert(gen_fp());
    --insertion_tries;
  }
}

QFILTER_TEST(Can_mix_insertion_and_queries) {
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

QFILTER_TEST(Can_mix_insertion_deletion_and_queries) {
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
      EXPECT_TRUE(filter.find(fp) == pfilter.first);
    } else {
      const auto ans_filter = filter.erase(fp);
      const auto ans_set = set.erase(fp);
      EXPECT_EQ(ans_set, ans_filter);
      EXPECT_FALSE(filter.count(fp));
    }
    EXPECT_EQ(set.size(), filter.size());
  });
}

QFILTER_TEST(Works_as_expected_if_full) {

  qfilter filter(10, 8); // q_bits, r_bits
  populate(filter);
  set_t set(filter.begin(), filter.end());

  ASSERT_TRUE(filter.full());
  EXPECT_THROW(filter.insert(*set.begin()), quofil::filter_is_full);

  for (const value_t fp : set)
    ASSERT_TRUE(filter.erase(fp));

  EXPECT_TRUE(filter.empty());
}

QFILTER_TEST(Can_be_cleared) {
  qfilter filter(9, 6); // q_bits, r_bits
  populate(filter, filter.capacity());

  const auto prev_q = filter.quotient_bits();
  const auto prev_r = filter.remainder_bits();
  const auto prev_cap = filter.capacity();
  filter.clear();

  EXPECT_TRUE(filter.empty());
  EXPECT_EQ(prev_cap, filter.capacity());
  EXPECT_EQ(prev_q, filter.quotient_bits());
  EXPECT_EQ(prev_r, filter.remainder_bits());

  EXPECT_TRUE(filter.begin() == filter.end());

  filter.insert(5);
  auto first = filter.begin();
  EXPECT_NE(first++, filter.end());
  EXPECT_EQ(first, filter.end());
  EXPECT_TRUE(filter.erase(5));
  EXPECT_TRUE(filter.empty());

  auto gen_fp = make_fp_generator(filter);
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

QFILTER_TEST(Can_be_default_constructed) {
  qfilter filter;
  EXPECT_EQ(0, filter.capacity());
  EXPECT_TRUE(filter.empty());
  EXPECT_TRUE(filter.full());
  EXPECT_EQ(0, filter.quotient_bits());
  EXPECT_EQ(0, filter.quotient_bits());
  EXPECT_TRUE(filter.begin() == filter.end());
}

QFILTER_TEST(Can_be_moved) {
  const size_t q_bits = 5;
  const size_t r_bits = 3;

  qfilter filter(q_bits, r_bits);
  populate(filter, filter.capacity() / 2);
  set_t set(filter.begin(), filter.end());

  const qfilter new_filter = std::move(filter);

  // The new filter is OK.
  EXPECT_EQ(set.size(), new_filter.size());
  EXPECT_EQ(size_t{1} << q_bits, new_filter.capacity());
  EXPECT_EQ(q_bits, new_filter.quotient_bits());
  EXPECT_EQ(r_bits, new_filter.remainder_bits());
  EXPECT_TRUE(equal(set, new_filter));

  // The old filter has invalid state but can be reassigned.
  filter = qfilter();
  EXPECT_EQ(0, filter.size());
  EXPECT_EQ(0, filter.capacity());
  EXPECT_EQ(0, filter.quotient_bits());
  EXPECT_EQ(0, filter.remainder_bits());
  EXPECT_TRUE(filter.begin() == filter.end());
}

QFILTER_TEST(Can_be_copied) {
  qfilter filter(5, 3); // q_bits, r_bits
  populate(filter, filter.capacity() / 2);
  set_t set(filter.begin(), filter.end());

  qfilter new_filter = filter;

  EXPECT_EQ(filter.size(), new_filter.size());
  EXPECT_EQ(filter.capacity(), new_filter.capacity());
  EXPECT_EQ(filter.quotient_bits(), new_filter.quotient_bits());
  EXPECT_EQ(filter.remainder_bits(), new_filter.remainder_bits());

  EXPECT_TRUE(equal(filter, new_filter));
  EXPECT_TRUE(equal(set, new_filter));
}

QF_ITERATOR_TEST(Can_iterate) {
  qfilter filter(11, 6); // q_bits, r_bits
  populate(filter, filter.capacity());
  std::set<value_t> set(filter.begin(), filter.end());

  ASSERT_EQ(*filter.begin(), *set.begin());
  ASSERT_TRUE(equal(filter, set));

  auto it = std::next(set.begin(), set.size() / 2);
  ASSERT_TRUE(std::equal(filter.find(*it), filter.end(), it, set.end()));
}

QF_ITERATOR_TEST(Works_as_expected_when_constructed) {
  qfilter filter(4, 4); // q_bits, r_bits
  const unsigned value1 = 3;
  filter.insert(value1); // Should be in the first slot.
  EXPECT_EQ(value1, *filter.begin());
  EXPECT_TRUE(filter.erase(value1));
  const unsigned value2 = 0b11'1111;
  filter.insert(value2);
  EXPECT_EQ(value2, *filter.begin());
}
