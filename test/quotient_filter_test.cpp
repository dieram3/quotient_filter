//          Copyright Diego Ramírez June, July 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter.hpp>
#include <gtest/gtest.h>

#include <iterator>    //
#include <ostream>     // for std::ostream
#include <stdexcept>   // for std::length_error
#include <type_traits> // for concepts check section
#include <utility>     //
#include <cassert>     // for assert
#include <cstddef>     //
// See below the use of uncommented headers.

// ==========================================
// MACROS
// ==========================================

#ifdef STATIC_ASSERT
#undef STATIC_ASSERT
#endif
#define STATIC_ASSERT(x) static_assert(x, #x)

// ==========================================
// Imported names
// ==========================================

// From <quofil/quotient_filter.hpp>
using quofil::quotient_filter;
using std::initializer_list;

// From <iterator>
using std::begin;
using std::end;
using std::distance;
// Also use std::next

// From utility
using std::pair;
using std::make_pair;
// Also use std::move

// From <cstddef>
using std::ptrdiff_t;
using std::size_t;

// ==========================================
// Constants.
// ==========================================

static constexpr float default_max_load_factor = 0.75f;

// ==========================================
// Helper functions.
// ==========================================

template <class T>
static const T &as_const(T &elem) {
  return elem;
}

// ==========================================
// Hash function used for testing.
// ==========================================

namespace {
class test_hash {
  int state;

public:
  explicit test_hash(int state_value = 0) : state{state_value} {}

  // 1:1 relation
  size_t operator()(const int key) const noexcept {
    // It is necessary static cast to unsigned int and not to size_t so the
    // hash gives a value in [0, numeric_limits<unsigned>::max()]
    return static_cast<unsigned>(key);
  }
  size_t operator()(const pair<int, int> &p) {
    return (*this)(p.first) + (*this)(p.second);
  }
  friend bool operator==(const test_hash &lhs, const test_hash &rhs) noexcept {
    return lhs.state == rhs.state;
  }
  friend std::ostream &operator<<(std::ostream &os, const test_hash &hash_fn) {
    return os << hash_fn.state;
  }
};

} // End anonymous namespace

// ==========================================
// Type aliases
// ==========================================

using filter_t = quotient_filter<int, test_hash, 16>;

// ==========================================
// Concepts check
// ==========================================

// Category
STATIC_ASSERT(std::is_class<filter_t>::value);
// Properties
STATIC_ASSERT(!std::is_polymorphic<filter_t>::value);
// Basic constructors
STATIC_ASSERT(std::is_default_constructible<filter_t>::value);
STATIC_ASSERT(std::is_copy_constructible<filter_t>::value);
STATIC_ASSERT(std::is_nothrow_move_constructible<filter_t>::value);
// Assignment
STATIC_ASSERT(std::is_copy_assignable<filter_t>::value);
STATIC_ASSERT(std::is_nothrow_move_assignable<filter_t>::value);
// Destructor
STATIC_ASSERT(std::is_nothrow_destructible<filter_t>::value);
// Check that filter_t filter(5) works but filter_t filter = 5 fails.
STATIC_ASSERT((std::is_constructible<filter_t, filter_t::size_type>::value));
STATIC_ASSERT((!std::is_convertible<filter_t::size_type, filter_t>::value));

// ==========================================
// Quotient-Filter assertions
// ==========================================

namespace {
struct slots_status {
  enum status_t { at_least, exactly };

  status_t status;
  size_t slot_count;
};

static slots_status sc_at_least(size_t slot_count) {
  return {slots_status::at_least, slot_count};
}

static slots_status sc_exactly(size_t slot_count) {
  return {slots_status::exactly, slot_count};
}
} // End anonymous namespace

static void expect_properties(const filter_t &c, const slots_status sstatus,
                              const test_hash hash_fn, const float ml) {
  switch (sstatus.status) {
  case slots_status::exactly:
    EXPECT_EQ(sstatus.slot_count, c.slot_count());
    break;
  case slots_status::at_least:
    // Note: probably replace EXPECT_LE with EXPECT_EQ will be the same since
    // the current implementation always use the minimal space it can.
    EXPECT_LE(sstatus.slot_count, c.slot_count());
    break;
  }

  EXPECT_EQ(hash_fn, c.hash_function());
  EXPECT_EQ(ml, c.max_load_factor());
}

// Checks whether a filter is empty and checks invariants.
static void expect_empty(const filter_t &c) {
  EXPECT_TRUE(c.empty());
  EXPECT_EQ(0, c.size());
  EXPECT_EQ(0, distance(c.begin(), c.end()));
  EXPECT_FLOAT_EQ(0.0f, c.load_factor());
  EXPECT_LE(c.load_factor(), c.max_load_factor());
}

// Checks whether a non-empty filter has the given contents and checks
// invariants.
template <typename T, typename H, size_t B>
static void expect_contents(const quotient_filter<T, H, B> &c,
                            const initializer_list<size_t> hash_list) {
  ASSERT_TRUE(hash_list.size() > 0) << "For empty filters use expect_empty()";

  EXPECT_FALSE(c.empty());
  EXPECT_EQ(hash_list.size(), c.size());
  EXPECT_EQ(ptrdiff_t(hash_list.size()), distance(c.begin(), c.end()));
  EXPECT_FLOAT_EQ(float(c.size()) / c.slot_count(), c.load_factor());
  EXPECT_LE(c.load_factor(), c.max_load_factor());

  auto it = c.begin();
  for (const size_t hash : hash_list) {
    ASSERT_TRUE(it != c.end());
    EXPECT_EQ(hash, *it++);
  }
  EXPECT_TRUE(it == c.end());
}

// ==========================================
// Tests section
// ==========================================

TEST(FilterTest, ConstructDefault) {
  const filter_t c;
  expect_properties(c, sc_exactly(0), test_hash{}, default_max_load_factor);
  expect_empty(c);
}

TEST(FilterTest, ConstructSlots) {
  const filter_t c(16);
  expect_properties(c, sc_at_least(16), test_hash{}, default_max_load_factor);
  expect_empty(c);
}

TEST(FilterTest, ConstructSlotsHash) {
  const filter_t c(25, test_hash{3});
  expect_properties(c, sc_at_least(32), test_hash{3}, default_max_load_factor);
  expect_empty(c);
}

TEST(FilterTest, ConstructRange) {
  const int elems[] = {2, 3, 4, 5, 1, 2, 3};
  const filter_t c(begin(elems), end(elems));
  expect_properties(c, sc_at_least(8), test_hash{}, default_max_load_factor);
  expect_contents(c, {1, 2, 3, 4, 5});
}

TEST(FilterTest, ConstructRangeSlots) {
  const int elems[] = {2, 3, 4, 5, 1, 2, 3};
  const filter_t c(begin(elems), end(elems), 9);
  expect_properties(c, sc_at_least(16), test_hash{}, default_max_load_factor);
  expect_contents(c, {1, 2, 3, 4, 5});
}

TEST(FilterTest, ConstructRangeSlotsHash) {
  const int elems[] = {2, 3, 4, 5, 1, 2, 3};
  const filter_t c(begin(elems), end(elems), 30, test_hash{29});
  expect_properties(c, sc_at_least(32), test_hash{29}, default_max_load_factor);
  expect_contents(c, {1, 2, 3, 4, 5});
}

TEST(FilterTest, ConstructInit) {
  const filter_t c = {4, 3, 3, 4, 1, 2, 5};
  expect_properties(c, sc_at_least(8), test_hash{}, default_max_load_factor);
  expect_contents(c, {1, 2, 3, 4, 5});
}

TEST(FilterTest, ConstructInitSlots) {
  const filter_t c({4, 3, 3, 4, 1, 2, 5}, 60);
  expect_properties(c, sc_at_least(64), test_hash{}, default_max_load_factor);
  expect_contents(c, {1, 2, 3, 4, 5});
}

TEST(FilterTest, ConstructInitSlotsHash) {
  const filter_t c({4, 3, 3, 4, 1, 2, 5}, 33, test_hash{39});
  expect_properties(c, sc_at_least(64), test_hash{39}, default_max_load_factor);
  expect_contents(c, {1, 2, 3, 4, 5});
}

TEST(FilterTest, ConstructCopy) {
  filter_t orig({1, 2, 3, 4, 5, 5}, 25, test_hash{13});
  orig.max_load_factor(0.5f);

  const filter_t c = as_const(orig);

  // Note: The slot count could be less than 32 if the copy was optimized.
  expect_properties(c, sc_at_least(32), test_hash{13}, 0.5f);
  expect_contents(c, {1, 2, 3, 4, 5});
}

TEST(FilterTest, AssignCopy) {
  filter_t orig({1, 2, 3, 4, 5, 5}, 25, test_hash{13});
  orig.max_load_factor(0.5f);
  const auto orig_sc = orig.slot_count();

  filter_t c({6, 7, 8}, 100, test_hash{29});
  c = as_const(orig);

  // Note: The slot count could be different if the copy was optimized.
  expect_properties(c, sc_exactly(orig_sc), test_hash{13}, 0.5f);
  expect_contents(c, {1, 2, 3, 4, 5});
}

TEST(FilterTest, ConstructorMove) {
  filter_t orig({1, 2, 3, 4, 5}, 30, test_hash{42});
  orig.max_load_factor(1.0f);
  const auto orig_sc = orig.slot_count();

  const filter_t c = std::move(orig);

  expect_properties(c, sc_exactly(orig_sc), test_hash{42}, 1.0f);
  expect_contents(c, {1, 2, 3, 4, 5});

  // orig can be reassigned.
  orig = filter_t();
  expect_properties(orig, sc_exactly(0), test_hash{}, default_max_load_factor);
  expect_empty(orig);
}

TEST(FilterTest, AssignMove) {
  filter_t orig({1, 2, 3, 4, 5}, 0, test_hash{19});
  orig.max_load_factor(0.85f);
  const auto orig_sc = orig.slot_count();

  filter_t c({5, 6, 7, 8, 9, 10}, 98, test_hash{33});
  c = std::move(orig);

  expect_properties(c, sc_exactly(orig_sc), test_hash{19}, 0.85f);
  expect_contents(c, {1, 2, 3, 4, 5});

  // orig can be reassigned.
  orig = filter_t();
  expect_properties(orig, sc_exactly(0), test_hash{}, default_max_load_factor);
  expect_empty(orig);
}

TEST(FilterTest, Iterators) {
  {
    const filter_t empty_filter;
    EXPECT_TRUE(empty_filter.begin() == empty_filter.end());
    EXPECT_FALSE(empty_filter.begin() != empty_filter.end());
  }
  {
    const filter_t c = {50, 70, 90};
    ASSERT_TRUE(c.begin() != c.end());
    ASSERT_FALSE(c.begin() == c.end());
    auto it1 = c.begin();
    ASSERT_TRUE(it1 == c.begin());
    ASSERT_FALSE(it1 != c.begin());
    EXPECT_EQ(50, *it1);

    auto it2 = ++it1;
    ASSERT_TRUE(it2 == it1);
    ASSERT_FALSE(it2 != it1);
    ASSERT_TRUE(it2 != c.begin() && it2 != c.end());
    ASSERT_FALSE(it2 == c.begin() || it2 == c.end());
    EXPECT_EQ(70, *it2);
    EXPECT_EQ(70, *it1);

    auto it3 = it2++;
    ASSERT_TRUE(it3 == it1);
    EXPECT_EQ(70, *it3);
    EXPECT_EQ(90, *it2++);

    ASSERT_TRUE(it2 == c.end());
    ASSERT_FALSE(it2 != c.end());
  }
}

TEST(FilterTest, MaxSizeAndLimits) {
  quotient_filter<int, test_hash, 4> c;
  static_assert(decltype(c)::hash_bits == 4,
                "The filter was instantiated with hash values of four bits");

  c.max_load_factor(1.0f);

  ASSERT_EQ(8, c.max_size()); // 1 << (hash_bits - 1) where hash_bits = 4
  c.insert({1, 2, 3, 4, 5, 6, 7, 8});
  EXPECT_EQ(8, c.size());
  EXPECT_FLOAT_EQ(1.0f, c.load_factor());
  EXPECT_THROW(c.insert(9), std::length_error);
  EXPECT_THROW(c.insert({7, 8, 9}), std::length_error);
  EXPECT_NO_THROW(c.insert({6, 7, 8}));
  c.erase(8);
  ASSERT_EQ(7, c.size());
  EXPECT_NO_THROW(c.insert({6, 9, 7}));
  EXPECT_THROW(c.insert(8), std::length_error);

  EXPECT_EQ(8, c.slot_count());
  expect_contents(c, {1, 2, 3, 4, 5, 6, 7, 9});
}

TEST(FilterTest, Clear) {
  filter_t c({1, 2, 3, 4, 5}, 60, test_hash{139});
  c.max_load_factor(0.60f);
  const auto prev_sc = c.slot_count();

  c.clear();
  expect_properties(c, sc_exactly(prev_sc), test_hash{139}, 0.60f);
  expect_empty(c);
}

TEST(FilterTest, InsertElement) {
  filter_t c;
  const auto res0 = c.insert(10);
  {
    SCOPED_TRACE("First insertion");
    EXPECT_TRUE(res0.second);
    ASSERT_TRUE(res0.first != c.end());
    EXPECT_EQ(10, *res0.first);
    expect_contents(c, {10});
  }
  {
    SCOPED_TRACE("Second insertion");
    const auto res = c.insert(10);
    EXPECT_FALSE(res.second);
    ASSERT_TRUE(res.first == res0.first);
    expect_contents(c, {10});
  }
  {
    SCOPED_TRACE("Third insertion");
    auto res = c.insert(20);
    EXPECT_TRUE(res.second);
    ASSERT_TRUE(res.first != c.end());
    EXPECT_EQ(20, *res.first);
    expect_contents(c, {10, 20});
  }
}

TEST(FilterTest, InsertRange) {
  filter_t c;
  {
    SCOPED_TRACE("First insertion");
    int elems[] = {1, 2, 3, 4, 3, 3, 1};
    c.insert(begin(elems), end(elems));
    expect_contents(c, {1, 2, 3, 4});
  }
  {
    SCOPED_TRACE("Second insertion");
    int elems[] = {5, 0, 4, 3};
    c.insert(begin(elems), end(elems));
    expect_contents(c, {0, 1, 2, 3, 4, 5});
  }
  {
    SCOPED_TRACE("Third insertion");
    int elems[] = {1, 2, 3, 4};
    c.insert(begin(elems), end(elems));
    expect_contents(c, {0, 1, 2, 3, 4, 5});
  }
}

TEST(FilterTest, InsertInit) {
  filter_t c;
  {
    SCOPED_TRACE("First insertion");
    c.insert({2, 1, 2, 3, 3});
    expect_contents(c, {1, 2, 3});
  }
  {
    SCOPED_TRACE("Second insertion");
    c.insert({2, 4, 1, 1, 5, 5, 0});
    expect_contents(c, {0, 1, 2, 3, 4, 5});
  }
  {
    SCOPED_TRACE("Third insertion");
    c.insert({1, 3, 3, 3, 0, 5});
    expect_contents(c, {0, 1, 2, 3, 4, 5});
  }
}

TEST(FilterTest, Emplace) {
  quotient_filter<pair<int, int>, test_hash, 16> c;
  {
    SCOPED_TRACE("First emplacement");
    const auto ans = c.emplace(3, 4);
    EXPECT_TRUE(ans.second);
    EXPECT_TRUE(c.begin() == ans.first);
    expect_contents(c, {7});
  }
  {
    SCOPED_TRACE("Second emplacement");
    const auto ans = c.emplace(2, 5);
    EXPECT_FALSE(ans.second);
    EXPECT_TRUE(c.begin() == ans.first);
    expect_contents(c, {7});
  }
  {
    SCOPED_TRACE("Third emplacement");
    const auto ans = c.emplace(make_pair(10, 5));
    EXPECT_TRUE(ans.second);
    EXPECT_TRUE(std::next(c.begin(), 1) == ans.first);
    expect_contents(c, {7, 15});
  }
}

TEST(FilterTest, EraseIterator) {
  using const_iter = filter_t::const_iterator;
  filter_t c = {1, 2, 3, 4, 5};

  const const_iter it = c.find(3);
  ASSERT_TRUE(it != c.end());
  c.erase(it);
  expect_contents(c, {1, 2, 4, 5});
  EXPECT_TRUE(c.find(3) == c.end());
}

TEST(FilterTest, EraseKey) {
  filter_t c = {1, 2, 3, 4, 5};
  {
    SCOPED_TRACE("Erase existing key");
    EXPECT_EQ(1, c.erase(3));
    EXPECT_EQ(0, c.count(3));
    expect_contents(c, {1, 2, 4, 5});
  }
  {
    SCOPED_TRACE("Erase non existing key");
    EXPECT_EQ(0, c.erase(3));
    EXPECT_EQ(0, c.count(3));
    expect_contents(c, {1, 2, 4, 5});
  }
}

TEST(FilterTest, Find) {
  const filter_t c = {10, 20, 30, 40, 50};
  {
    SCOPED_TRACE("Find existing key");
    const auto it = c.find(30);
    ASSERT_TRUE(it != c.end());
    EXPECT_EQ(30, *it);
    EXPECT_TRUE(it == std::next(c.begin(), 2));
  }
  {
    SCOPED_TRACE("Find non existing key");
    const auto it = c.find(35);
    EXPECT_TRUE(it == c.end());
  }
}

TEST(FilterTest, Count) {
  const filter_t c = {10, 20, 30, 40, 50, 50, 50, 40, 40};
  ASSERT_EQ(5, c.size());

  EXPECT_EQ(0, c.count(0));
  EXPECT_EQ(1, c.count(10));
  EXPECT_EQ(1, c.count(20));
  EXPECT_EQ(1, c.count(30));
  EXPECT_EQ(1, c.count(40));
  EXPECT_EQ(1, c.count(50));
  EXPECT_EQ(0, c.count(60));
}

TEST(FilterTest, LoadFactor) {
  filter_t c;
  c.insert(10);
  ASSERT_EQ(1, c.size());
  EXPECT_FLOAT_EQ(1.0f / c.slot_count(), c.load_factor());

  c.insert({20, 30});
  ASSERT_EQ(3, c.size());
  EXPECT_FLOAT_EQ(3.0f / c.slot_count(), c.load_factor());
}

TEST(FilterTest, MaxLoadFactor) {
  filter_t c(16);
  auto new_elem = [&c] { return 10 * static_cast<int>(c.size()); };

  c.max_load_factor(0.5f);
  EXPECT_FLOAT_EQ(0.5f, c.max_load_factor());
  auto slot_count = c.slot_count();
  while (2 * c.size() < slot_count) {
    c.insert(new_elem());
    ASSERT_EQ(slot_count, c.slot_count());
  }
  EXPECT_FLOAT_EQ(0.5f, c.load_factor());

  c.insert(new_elem());
  EXPECT_LT(slot_count, c.slot_count());
  slot_count = c.slot_count();
  c.max_load_factor(1.0f);
  EXPECT_FLOAT_EQ(1.0f, c.max_load_factor());
  EXPECT_EQ(slot_count, c.slot_count())
      << "slot_count() should not change if max load factor becomes greater";

  while (c.size() < slot_count) {
    c.insert(new_elem());
    ASSERT_EQ(slot_count, c.slot_count());
  }
  EXPECT_FLOAT_EQ(1.0f, c.load_factor());

  c.max_load_factor(0.5f);
  EXPECT_LT(slot_count, c.slot_count())
      << "slot_count() should grow to satisfy the requirements of "
         "the new max_load_factor()";
  EXPECT_LE(c.load_factor(), 0.5f);
}

TEST(FilterTest, Regenerate) {
  filter_t c;
  c.max_load_factor(0.5f);
  c.regenerate(100);
  expect_properties(c, sc_at_least(128), test_hash{}, 0.5f);

  auto slot_count = c.slot_count();
  c.insert({1, 2, 3, 4});
  EXPECT_EQ(slot_count, c.slot_count());
  expect_contents(c, {1, 2, 3, 4});

  c.regenerate(0);
  expect_properties(c, sc_at_least(8), test_hash{}, 0.5f);
  expect_contents(c, {1, 2, 3, 4});
}

TEST(FilterTest, Reserve) {
  filter_t c;
  c.max_load_factor(0.5f);
  c.reserve(100);
  expect_properties(c, sc_at_least(256), test_hash{}, 0.5f);
  expect_empty(c);

  c.insert({1, 2, 3, 3});
  c.reserve(30);
  expect_properties(c, sc_at_least(64), test_hash{}, 0.5f);
  expect_contents(c, {1, 2, 3});

  c.reserve(0);
  expect_properties(c, sc_at_least(8), test_hash{}, 0.5f);
  expect_contents(c, {1, 2, 3});
}

TEST(FilterTest, EqualityOperators) {
  {
    filter_t orig(0, test_hash{123});
    orig.max_load_factor(1.0f);
    orig.insert({1, 2, 3, 4, 5});
    ASSERT_LT(7, orig.slot_count());

    filter_t other(0, test_hash{321});
    other.max_load_factor(0.5f);
    other.insert({1, 2, 3, 4, 5});
    ASSERT_LT(15, other.slot_count());

    // The have different hash functions, max load factors and possibly
    // different slot counts but they are still equal.
    EXPECT_TRUE(as_const(orig) == as_const(other));
    EXPECT_FALSE(as_const(orig) != as_const(other));
  }

  {
    const filter_t orig = {1, 2, 3, 4, 5};
    const filter_t other = {1, 2, 3, 4, 5, 6};

    // Range comparison could be equal if size is not taken into account.
    EXPECT_FALSE(orig == other);
    EXPECT_FALSE(other == orig);
    EXPECT_TRUE(orig != other);
    EXPECT_TRUE(other != orig);
  }
  {
    const filter_t empty1;
    const filter_t empty2(10, test_hash{321});
    EXPECT_TRUE(empty1 == empty2);
    EXPECT_FALSE(empty1 != empty2);
  }
}

TEST(FilterTest, SwapMember) {
  filter_t c1({1, 2, 3, 4, 5}, 250, test_hash{23});
  c1.max_load_factor(0.3f);
  const auto c1_sc = c1.slot_count();

  filter_t c2({5, 6, 7, 8, 9, 9, 7}, 0, test_hash{47});
  c2.max_load_factor(0.8f);
  const auto c2_sc = c2.slot_count();

  c1.swap(c2);

  expect_properties(c1, sc_exactly(c2_sc), test_hash{47}, 0.8f);
  expect_contents(c1, {5, 6, 7, 8, 9});

  expect_properties(c2, sc_exactly(c1_sc), test_hash{23}, 0.3f);
  expect_contents(c2, {1, 2, 3, 4, 5});
}

TEST(FilterTest, SwapNonMember) {
  filter_t c1({1, 2, 3, 4, 5}, 250, test_hash{23});
  c1.max_load_factor(0.3f);
  const auto c1_sc = c1.slot_count();

  filter_t c2({5, 6, 7, 8, 9, 9, 7}, 0, test_hash{47});
  c2.max_load_factor(0.8f);
  const auto c2_sc = c2.slot_count();

  swap(c1, c2);

  expect_properties(c1, sc_exactly(c2_sc), test_hash{47}, 0.8f);
  expect_contents(c1, {5, 6, 7, 8, 9});

  expect_properties(c2, sc_exactly(c1_sc), test_hash{23}, 0.3f);
  expect_contents(c2, {1, 2, 3, 4, 5});
}
