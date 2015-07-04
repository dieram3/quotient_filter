//          Copyright Diego Ramírez June 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter_fp.hpp>
#include <gtest/gtest.h>

#include <algorithm> // for std::equal
#include <iterator>  // for std::{begin, end, next}
#include <random>    // imported names declared below.
#include <utility>   // for std::move
#include <vector>    // for std::vector
#include <cstddef>   // imported names declared below.
#include <cstdint>   // for SIZE_MAX

// ==========================================
// Macros
// ==========================================

#ifdef FILTER_TEST
#undef FILTER_TEST
#endif
#define FILTER_TEST(test_name) TEST(quotient_filter_fp, test_name)

#ifdef ITERATOR_TEST
#undef ITERATOR_TEST
#endif
#define ITERATOR_TEST(test_name) TEST(quotient_filter_fp_iterator, test_name)

// ==========================================
// Imported names
// ==========================================

// From <random>
using std::bernoulli_distribution;
using std::mt19937;
using std::minstd_rand;
using std::uniform_int_distribution;

// From <cstddef>
using std::size_t;

// ==========================================
// Type aliases
// ==========================================

using filter_t = quofil::quotient_filter_fp;
using value_t = filter_t::value_type;
using set_t = std::set<value_t>;

// ==========================================
// Utilities for tests.
// ==========================================

template <typename Function>
static void repeat(size_t n, Function f) {
  while (n--)
    f();
}

template <typename Range1, typename Range2>
static bool equal(const Range1 &lhs, const Range2 &rhs) {
  return std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs),
                    std::end(rhs));
}

// Checks whether lhs and rhs are clones.
static bool totally_equal(const filter_t &lhs, const filter_t &rhs) noexcept {
  return lhs.size() == rhs.size() && lhs.capacity() == rhs.capacity() &&
         lhs.quotient_bits() == rhs.quotient_bits() &&
         lhs.remainder_bits() == rhs.remainder_bits() && equal(lhs, rhs);
}

static auto make_fp_generator(const filter_t &filter) {
  const size_t fp_bits = filter.quotient_bits() + filter.remainder_bits();
  const value_t max_fp = (value_t{1} << fp_bits) - 1;

  mt19937 gen(823076453);
  uniform_int_distribution<value_t> dist(0, max_fp);

  return [ gen = std::move(gen), dist ]() mutable { return dist(gen); };
}

static auto make_insertion_decision_generator(const filter_t &filter) {
  using param_t = std::bernoulli_distribution::param_type;
  bernoulli_distribution dist;
  minstd_rand gen(3782348);

  return [&filter, gen = std::move(gen), dist ]() mutable {
    if (filter.empty())
      return true;
    if (filter.full())
      return false;
    const double load_factor = double(filter.size()) / filter.capacity();
    return dist(gen, param_t(1.0 - load_factor));
  };
}

// Populates a filter with random values.
// Note: A filter with a small r_bits is difficult to fill.
static void populate(filter_t &filter, size_t insertion_tries = SIZE_MAX) {
  auto gen_fp = make_fp_generator(filter);
  while (!filter.full() && insertion_tries) {
    filter.insert(gen_fp());
    --insertion_tries;
  }
}

// ==========================================
// FILTER_TEST Section
// ==========================================

FILTER_TEST(Can_mix_insertions_deletions_and_queries) {
  // small r is required so random fingerprints have high probability to exist.
  filter_t filter(13, 1);
  set_t set;

  auto gen_fp = make_fp_generator(filter);
  auto do_insertion = make_insertion_decision_generator(filter);

  repeat(3 * filter.capacity(), [&] {
    const auto fp = gen_fp();

    if (do_insertion()) {
      const auto pfilter = filter.insert(fp);
      const auto pset = set.insert(fp);
      EXPECT_EQ(pset.second, pfilter.second);
      EXPECT_EQ(fp, *pfilter.first);
      EXPECT_EQ(pfilter.first, filter.find(fp));
    } else {
      const auto ans_filter = filter.erase(fp);
      const auto ans_set = set.erase(fp);
      EXPECT_EQ(ans_set, ans_filter);
      EXPECT_FALSE(filter.count(fp));
    }
    EXPECT_EQ(set.size(), filter.size());
  });

  for (value_t value : set)
    EXPECT_TRUE(filter.count(value));

  repeat(10000, [&] {
    const auto fp = gen_fp();
    EXPECT_EQ(set.count(fp), filter.count(fp));
  });
}

FILTER_TEST(Can_be_empty_and_full) {

  filter_t filter(10, 8); // q_bits, r_bits
  const auto capacity = filter.capacity();

  populate(filter);
  set_t set(filter.begin(), filter.end());

  EXPECT_TRUE(filter.full());
  EXPECT_EQ(capacity, filter.capacity());
  EXPECT_EQ(capacity, filter.size());

  EXPECT_THROW(filter.insert(*set.begin()), quofil::filter_is_full);
  EXPECT_TRUE(filter.full());

  for (const value_t fp : set) {
    EXPECT_FALSE(filter.empty());
    EXPECT_TRUE(filter.erase(fp));
    EXPECT_FALSE(filter.full());
  }

  EXPECT_TRUE(filter.empty());
}

FILTER_TEST(Can_be_cleared) {
  filter_t filter(9, 6); // q_bits, r_bits
  populate(filter, filter.capacity());

  const auto prev_q = filter.quotient_bits();
  const auto prev_r = filter.remainder_bits();
  const auto prev_cap = filter.capacity();
  filter.clear();

  EXPECT_TRUE(filter.empty());
  EXPECT_EQ(prev_cap, filter.capacity());
  EXPECT_EQ(prev_q, filter.quotient_bits());
  EXPECT_EQ(prev_r, filter.remainder_bits());
  EXPECT_EQ(filter.begin(), filter.end());

  filter.insert(5);
  auto first = filter.begin();
  EXPECT_NE(first++, filter.end());
  EXPECT_EQ(first, filter.end());
  EXPECT_TRUE(filter.erase(5));
  EXPECT_TRUE(filter.empty());

  auto gen_fp = make_fp_generator(filter);
  set_t set;
  while (!filter.full()) {
    const auto fp = gen_fp();
    if (set.insert(fp).second)
      EXPECT_TRUE(filter.insert(fp).second);
  }

  for (value_t fp : set)
    EXPECT_TRUE(filter.erase(fp));

  EXPECT_TRUE(filter.empty());
}

FILTER_TEST(Can_be_default_constructed) {
  filter_t filter;
  EXPECT_EQ(0, filter.capacity());
  EXPECT_TRUE(filter.empty());
  EXPECT_TRUE(filter.full());
  EXPECT_EQ(0, filter.quotient_bits());
  EXPECT_EQ(0, filter.quotient_bits());
  EXPECT_EQ(filter.begin(), filter.end());
}

FILTER_TEST(Can_be_constructed_with_required_q_and_r) {
  const size_t q_bits = 4;
  const size_t r_bits = 7;
  filter_t filter(q_bits, r_bits);

  EXPECT_EQ(q_bits, filter.quotient_bits());
  EXPECT_EQ(r_bits, filter.remainder_bits());
  EXPECT_EQ(0, filter.size());
  EXPECT_EQ(1UL << q_bits, filter.capacity());
  EXPECT_EQ(filter.begin(), filter.end());
}

FILTER_TEST(Can_be_copied) {
  filter_t filter(5, 3); // q_bits, r_bits
  populate(filter, filter.capacity() / 2);
  set_t set(filter.begin(), filter.end());

  filter_t new_filter = filter;

  ASSERT_TRUE(equal(set, new_filter));
  EXPECT_TRUE(totally_equal(filter, new_filter));
}

FILTER_TEST(Can_be_moved) {
  filter_t filter(5, 3);
  populate(filter, filter.capacity() / 2);

  const filter_t backup_filter = filter;
  const filter_t new_filter = std::move(filter);

  // The new filter is OK.
  EXPECT_TRUE(totally_equal(backup_filter, new_filter));

  // The old filter has invalid state but can be reassigned.
  filter = filter_t();
  EXPECT_TRUE(totally_equal(filter, filter_t()));
}

FILTER_TEST(Can_be_operated_normally_if_default_constructed) {
  filter_t filter;
  constexpr value_t random_fp = 1234;
  EXPECT_FALSE(filter.count(random_fp));
  EXPECT_EQ(filter.end(), filter.find(random_fp));
  EXPECT_THROW(filter.insert(random_fp), quofil::filter_is_full);
  EXPECT_FALSE(filter.erase(random_fp));
  filter.clear();
  EXPECT_TRUE(totally_equal(filter_t(), filter));
}

// ==========================================
// ITERATOR_TEST Section
// ==========================================

ITERATOR_TEST(Can_iterate) {
  filter_t filter(11, 6); // q_bits, r_bits
  populate(filter, filter.capacity());
  set_t set(filter.begin(), filter.end());

  ASSERT_EQ(*filter.begin(), *set.begin());
  ASSERT_TRUE(equal(filter, set));

  auto it = std::next(set.begin(), set.size() / 2);
  ASSERT_TRUE(std::equal(filter.find(*it), filter.end(), it, set.end()));
}

ITERATOR_TEST(Works_as_expected_when_constructed) {
  filter_t filter(4, 4); // q_bits, r_bits
  const unsigned value1 = 3;
  filter.insert(value1); // Should be in the first slot.
  EXPECT_EQ(value1, *filter.begin());
  EXPECT_TRUE(filter.erase(value1));
  const unsigned value2 = 0b11'1111;
  filter.insert(value2);
  EXPECT_EQ(value2, *filter.begin());
}
