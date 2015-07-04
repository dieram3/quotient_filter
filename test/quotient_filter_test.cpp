//          Copyright Diego Ramírez June 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter.hpp>
#include <gtest/gtest.h>

#include <algorithm>     //
#include <functional>    // for std::ref
#include <ios>           //
#include <iterator>      // for std::{advance, begin, end, next}
#include <limits>        //
#include <numeric>       // for std::iota
#include <ostream>       // for std::ostream
#include <random>        //
#include <set>           // for std::set
#include <sstream>       //
#include <string>        // for std::string
#include <type_traits>   //
#include <unordered_set> // for std::unordered_set
#include <utility>       // for std::{forward, move}
#include <vector>        // for std::vector
#include <cassert>       // for assert
#include <cmath>         //
#include <cstddef>       //
// See below the use of uncommented headers.

// ==========================================
// MACROS
// ==========================================

#ifdef STATIC_ASSERT
#undef STATIC_ASSERT
#endif
#define STATIC_ASSERT(x) static_assert(x, #x)

#ifdef FILTER_TEST
#undef FILTER_TEST
#endif
#define FILTER_TEST(test_name) TEST(quotient_filter, test_name)

// ==========================================
// Imported names
// ==========================================

// From <quofil/quotient_filter.hpp>
using quofil::quotient_filter;

// From <gtest/gtest.h>
using testing::AssertionResult;
using testing::AssertionSuccess;
using testing::AssertionFailure;

// From <algorithm>
using std::for_each;
using std::max;
using std::min;
using std::sort;
// Also use: equal, generate, shuffle

// From <ios>
using std::boolalpha;

// From <numeric_limits>
using std::numeric_limits;

// From <random>
using std::mt19937;
using std::seed_seq;
using std::uniform_int_distribution;

// From <sstream>
using std::ostringstream;

// From <type_traits>
using std::is_integral;
// Also use other metaprogramming functions in the concept check section.

// From <cmath>
using std::ceil;
using std::fabs;

// From <cstddef>
using std::ptrdiff_t;
using std::size_t;

// ==========================================
// Constants.
// ==========================================

static constexpr float default_max_load_factor = 0.8f;

// ==========================================
// Basic Utility functions.
// ==========================================

template <typename Function>
static void repeat(size_t n, Function f) {
  while (n--)
    f();
}

template <typename T = int>
static std::vector<T> make_iota_vector(size_t num_elems) {
  std::vector<T> vec(num_elems);
  std::iota(vec.begin(), vec.end(), T{0});
  assert(vec.size() == num_elems);
  return vec;
}

template <typename T>
static constexpr bool float_equal(const T lhs, const T rhs) noexcept {
  STATIC_ASSERT(std::is_floating_point<T>::value);
  return fabs(lhs - rhs) < numeric_limits<T>::epsilon();
}

template <typename Range>
static void shuffle(Range &range) {
  seed_seq seq = {91, 1090, 1230123, 1981231, 9819};
  mt19937 gen(seq);
  std::shuffle(std::begin(range), std::end(range), gen);
}

// ==========================================
// Integer generator
// ==========================================

template <typename T>
class int_generator {
  mt19937 gen;
  uniform_int_distribution<T> dist{numeric_limits<T>::min(),
                                   numeric_limits<T>::max()};
  STATIC_ASSERT(is_integral<T>::value);

public:
  int_generator(std::mt19937::result_type seed) : gen(seed) {}

  template <typename Sseq>
  int_generator(Sseq &s)
      : gen(s) {}

  void set_params(T a, T b) {
    using param_t = typename decltype(dist)::param_type;
    dist.param(param_t(a, b));
  }

  T operator()() { return dist(gen); }
};

// ==========================================
// Debug view for ranges.
// ==========================================

namespace {
template <typename Range>
class basic_debug_view {
public:
  basic_debug_view(const Range &range) : prange{&range} {}

  auto begin() const { return prange->begin(); }
  auto end() const { return prange->end(); }
  auto empty() const { return prange->empty(); }
  auto size() const { return prange->size(); }

private:
  const Range *prange;
};

template <typename R1, typename R2>
static bool operator==(const basic_debug_view<R1> &lhs,
                       const basic_debug_view<R2> &rhs) {
  // Intentional redundancy added.
  return lhs.size() == rhs.size() &&
         std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename R1, typename R2>
static bool operator!=(const basic_debug_view<R1> &lhs,
                       const basic_debug_view<R2> &rhs) {
  return !(lhs == rhs);
}

template <typename Range>
static std::ostream &operator<<(std::ostream &os,
                                basic_debug_view<Range> view) {
  if (view.empty()) {
    return os << "[ empty ]";
  }
  if (view.size() <= 10) {
    os << '[';
    for (auto elem : view)
      os << ' ' << elem;
    return os << " ]";
  }
  auto it = view.begin();
  os << '[';
  repeat(5, [&] { os << ' ' << *it++; });
  os << " ... ";
  std::advance(it, ptrdiff_t(view.size()) - 10);
  repeat(5, [&] { os << ' ' << *it++; });
  assert(it == view.end());
  return os << " ]";
}

template <typename Range>
static auto debug_view(const Range &range) {
  return basic_debug_view<Range>(range);
}

} // End anonymous namespace

// ==========================================
// Hash function used for testing.
// ==========================================

namespace {
class state_hash {
public:
  state_hash(int state_val = 0) : state{state_val} {}

  // 1:1 relation, always gen fp_bits
  size_t operator()(const int key) const noexcept {
    // It is necessary static cast to unsigned int and not to size_t so the
    // hash gives a fingerprint fp in [0, numeric_limits<unsigned>::max()]
    return static_cast<unsigned>(key);
  }
  friend bool operator==(const state_hash &lhs,
                         const state_hash &rhs) noexcept {
    return lhs.state == rhs.state;
  }
  friend std::ostream &operator<<(std::ostream &os, const state_hash &hash_fn) {
    return os << hash_fn.state;
  }

private:
  int state;
};

static int hash_to_key(const std::size_t hash) {
  return static_cast<int>(static_cast<unsigned>(hash));
}

} // End anonymous namespace

// ==========================================
// Quotient-Filter like class.
// ==========================================

namespace {

template <class SetT>
class set_wrapper {
  SetT the_set;
  state_hash hash_fn;

public:
  set_wrapper() : hash_fn() {}

  template <class InputIt>
  set_wrapper(InputIt first, InputIt last) {
    insert(first, last);
  }

  auto insert(int elem) { return the_set.insert(hash_fn(elem)); }
  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for_each(first, last, [this](int elem) { insert(elem); });
  }

  auto begin() const { return the_set.begin(); }
  auto end() const { return the_set.end(); }
  auto empty() const { return the_set.empty(); }
  auto size() const { return the_set.size(); }
};

} // End anonymous namespace

// ==========================================
// Type aliases
// ==========================================

using set_t = set_wrapper<std::set<size_t>>;
using unordered_set_t = set_wrapper<std::unordered_set<size_t>>;
using filter_t =
    quotient_filter<int, state_hash, numeric_limits<unsigned>::digits>;

// ==========================================
// Quotient-Filter extended query functions
// ==========================================

// Redundant empty check.
template <typename Filter>
static bool is_empty(const Filter &filter) {
  return filter.empty() && filter.size() == 0 && filter.begin() == filter.end();
}

static size_t
calc_expected_slot_count(const size_t num_elems,
                         const float ml = default_max_load_factor) {
  const size_t min_valid_cap = static_cast<size_t>(ceil(num_elems / ml));
  size_t min_num_slots = 1;
  while (min_num_slots < min_valid_cap)
    min_num_slots *= 2;
  return min_num_slots;
}

static size_t calc_expected_capacity(const size_t num_elems,
                                     const float ml = default_max_load_factor) {
  return static_cast<size_t>(calc_expected_slot_count(num_elems, ml) * ml);
}

// Returns true the given filter has the minimum reserve able to hold num_elem
// elements given the current requirements.
template <typename Filter>
static bool has_precise_reserve_for(const Filter &filter,
                                    const size_t num_elems) {

  if (num_elems == 0) {
    return filter.capacity() == 0 && filter.slot_count() == 0;
  }

  const float ml = filter.max_load_factor();

  return filter.capacity() == calc_expected_capacity(num_elems, ml) &&
         filter.slot_count() == calc_expected_slot_count(num_elems, ml);
}

template <typename Filter>
static bool use_minimum_storage(const Filter &filter) {
  return has_precise_reserve_for(filter, filter.size());
}

// Asserts that f1 has same state than f2 (including the elements).
static AssertionResult assert_all_equal(const char *expr1, const char *expr2,
                                        const filter_t &f1,
                                        const filter_t &f2) {
  ostringstream oss;
  oss << boolalpha;

  auto report =
      [&oss, expr1, expr2](const char *param, auto value1, auto value2) {
        oss << param << ":\n";
        oss << " Expected: " << value1 << '\n';
        oss << " Actual:   " << value2 << '\n';
      };

  auto check_for = [report](const char *member_name, auto value1, auto value2,
                            auto equal_to) {
    if (!equal_to(value1, value2))
      report(member_name, value1, value2);
  };

  auto eq = std::equal_to<>();
  check_for("Contained elements", debug_view(f1), debug_view(f2), eq);
  check_for("Result of .empty()", f1.empty(), f2.empty(), eq);
  check_for("size", f1.size(), f2.size(), eq);
  check_for("slot_count", f1.slot_count(), f2.slot_count(), eq);
  check_for("hash_function", f1.hash_function(), f2.hash_function(), eq);
  check_for("capacity", f1.capacity(), f2.capacity(), eq);

  auto feq = [](float x, float y) { return float_equal(x, y); };
  check_for("max_load_factor", f1.max_load_factor(), f2.max_load_factor(), feq);
  check_for("load_factor", f1.load_factor(), f2.load_factor(), feq);

  const std::string the_report = oss.str();
  if (the_report.empty())
    return AssertionSuccess();

  AssertionResult result(false);
  result << "The given filters are not exactly equal.\n";
  result << "The expected filter:   " << expr1 << '\n';
  result << "And the actual filter: " << expr2 << '\n';
  result << "Differ on:\n\n";
  result << the_report;
  return result;
}

// ==========================================
// Test data generation
// ==========================================

// Creates a quotient_filter of the given size with uniformly distrubuted data
// between numeric_limits<T>::min() and numeric_limits<T>::max()
// This algorithm is deterministic when compiled under the same environment.
static filter_t make_test_filter(const size_t size) {
  seed_seq seq = {size, 10 * size, size << 3 & ~size};
  int_generator<int> gen_elem(seq);
  filter_t filter(size);
  while (filter.size() != size)
    filter.insert(gen_elem());
  return filter;
}

static std::vector<size_t> make_test_capacities() {
  return {0, 1, 2, 30, 50, 140};
}

static std::vector<state_hash> make_test_hash_functions() {
  return {0, 2, 5, -1};
}

static auto make_test_vectors() {
  std::vector<std::vector<int>> vectors;
  vectors.emplace_back();
  vectors.emplace_back(1);
  vectors.emplace_back(10);
  vectors.emplace_back(50);
  vectors.emplace_back(130);
  vectors.emplace_back(350);

  assert(vectors.front().empty());
  assert(vectors.back().size() == 350);

  using seed_type = mt19937::result_type;
  int_generator<seed_type> gen_seed(3487124);

  for (std::vector<int> &vec : vectors) {
    int_generator<int> gen_elem(gen_seed());
    std::generate(vec.begin(), vec.end(), std::ref(gen_elem));
  }

  return vectors;
}

// ==========================================
// Concepts check
// ==========================================

// Category
STATIC_ASSERT(std::is_class<filter_t>::value);
// Properties
STATIC_ASSERT(!std::is_polymorphic<filter_t>::value);
// Basic constructors
STATIC_ASSERT(std::is_nothrow_default_constructible<filter_t>::value);
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
// FILTER_TEST Section
// ==========================================

FILTER_TEST(Can_be_default_constructed) {
  EXPECT_PRED_FORMAT2(assert_all_equal, filter_t(0), filter_t());
}

FILTER_TEST(Can_be_copy_constructed) {
  const filter_t orig = make_test_filter(15);
  EXPECT_PRED_FORMAT2(assert_all_equal, orig, filter_t(orig));
}

FILTER_TEST(Can_be_copy_assigned) {
  const filter_t orig = make_test_filter(15);
  filter_t new_filter;
  EXPECT_PRED_FORMAT2(assert_all_equal, orig, new_filter = orig);
}

FILTER_TEST(Can_be_move_constructed) {
  filter_t orig = make_test_filter(15);
  const filter_t backup = orig;
  EXPECT_PRED_FORMAT2(assert_all_equal, backup, filter_t(std::move(orig)));
  // The moved filter can be remade
  orig = backup;
  EXPECT_PRED_FORMAT2(assert_all_equal, backup, orig);
}

FILTER_TEST(Can_be_move_assigned) {
  filter_t orig = make_test_filter(15);
  const filter_t backup = orig;
  filter_t new_filter;
  EXPECT_PRED_FORMAT2(assert_all_equal, backup, new_filter = std::move(orig));
  // The moved filter can be remade
  orig = backup;
  EXPECT_PRED_FORMAT2(assert_all_equal, backup, orig);
}

FILTER_TEST(Can_be_constructed_without_ranges) {
  for (const size_t cap : make_test_capacities()) {
    for (const state_hash hash_fn : make_test_hash_functions()) {
      const filter_t filter(cap, hash_fn);
      EXPECT_FLOAT_EQ(default_max_load_factor, filter.max_load_factor());
      EXPECT_TRUE(is_empty(filter));
      EXPECT_TRUE(has_precise_reserve_for(filter, cap));
      EXPECT_EQ(hash_fn, filter.hash_function());
    }
    const filter_t filter(cap);
    EXPECT_PRED_FORMAT2(assert_all_equal, filter_t(cap, state_hash()), filter);
  }
}

FILTER_TEST(Can_be_constructed_with_ranges) {
  for (const auto &vec : make_test_vectors()) {
    for (const size_t cap : make_test_capacities()) {
      for (const state_hash hash_fn : make_test_hash_functions()) {
        filter_t expected_filter(cap, hash_fn);
        expected_filter.insert(vec.begin(), vec.end());
        EXPECT_PRED_FORMAT2(assert_all_equal, expected_filter,
                            filter_t(vec.begin(), vec.end(), cap, hash_fn));
      }
      EXPECT_PRED_FORMAT2(assert_all_equal,
                          filter_t(vec.begin(), vec.end(), cap, state_hash()),
                          filter_t(vec.begin(), vec.end(), cap));
    }
    EXPECT_PRED_FORMAT2(assert_all_equal, filter_t(vec.begin(), vec.end(), 0),
                        filter_t(vec.begin(), vec.end()));
  }
}

FILTER_TEST(Can_be_constructed_with_initializer_lists) {
  for (const size_t cap : make_test_capacities()) {
    for (const state_hash hash_fn : make_test_hash_functions()) {
      filter_t expected_filter(cap, hash_fn);
      expected_filter.insert({1, 1, 2, 3, 5, 8});
      EXPECT_PRED_FORMAT2(assert_all_equal, expected_filter,
                          filter_t({1, 1, 2, 3, 5, 8}, cap, hash_fn));
    }
    EXPECT_PRED_FORMAT2(assert_all_equal,
                        filter_t({1, 1, 2, 3, 5, 8}, cap, state_hash()),
                        filter_t({1, 1, 2, 3, 5, 8}, cap));
  }

  EXPECT_PRED_FORMAT2(assert_all_equal, filter_t({1, 1, 2, 3, 5, 8}, 0),
                      filter_t({1, 1, 2, 3, 5, 8}));
}

FILTER_TEST(Can_be_iterated) {
  for (const auto &vec : make_test_vectors()) {
    const set_t set(vec.begin(), vec.end());
    const filter_t filter(vec.begin(), vec.end(), set.size());

    EXPECT_TRUE(
        std::equal(set.begin(), set.end(), filter.begin(), filter.end()));
  }

  const filter_t filter;
  EXPECT_TRUE(filter.begin() == filter.end());
}

FILTER_TEST(Has_max_size_according_to_fp_bits_and_works_well_at_limits) {
  // fp_bits == 10, r_bits must be at least 1 so max q_bits is 9
  quotient_filter<int, state_hash, 10> filter;
  filter.max_load_factor(1.0f);
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(use_minimum_storage(filter));

  STATIC_ASSERT(decltype(filter)::fp_bits == 10);
  constexpr size_t max_size = size_t{1} << 9;
  EXPECT_EQ(max_size, filter.max_size());

  auto vec = make_iota_vector<int>(2 * max_size);
  assert(size_t(vec.back() + 1) == size_t{1} << 10);
  shuffle(vec);
  const int elem_out = vec.back();
  vec.erase(vec.begin() + vec.size() / 2, vec.end());

  for (int elem : vec) {
    EXPECT_TRUE(filter.insert(elem).second);
    EXPECT_TRUE(use_minimum_storage(filter));
  }

  EXPECT_NO_THROW(filter.insert(vec.back()));
  EXPECT_THROW(filter.insert(elem_out), std::length_error);
  sort(vec.begin(), vec.end());
  EXPECT_EQ(debug_view(vec), debug_view(filter));
}

FILTER_TEST(Can_be_cleared) {
  const size_t num_elems = 1000;
  const auto backup_filter = make_test_filter(num_elems);
  auto filter = backup_filter;

  ASSERT_EQ(num_elems, filter.size());
  ASSERT_PRED_FORMAT2(assert_all_equal, filter, backup_filter);
  filter.clear();
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(has_precise_reserve_for(filter, num_elems));
  filter.insert(backup_filter.begin(), backup_filter.end());
  EXPECT_PRED_FORMAT2(assert_all_equal, backup_filter, filter);

  filter.clear();
  filter.insert({1, 2, 3, 4, 5});
  if (!use_minimum_storage(filter)) {
    filter.reserve(0);
    ASSERT_EQ(5, filter.size());
    EXPECT_TRUE(use_minimum_storage(filter));
  }

  EXPECT_PRED_FORMAT2(assert_all_equal, filter_t({1, 2, 3, 4, 5}), filter);
}

FILTER_TEST(Can_insert_single_values) {
  for (const auto &vec : make_test_vectors()) {
    filter_t filter;
    set_t set;
    for (const int elem : vec) {
      const auto filter_ans = filter.insert(elem);
      const auto set_ans = set.insert(elem);
      EXPECT_EQ(set_ans.second, filter_ans.second);
      EXPECT_EQ(filter.find(elem), filter_ans.first);
      EXPECT_TRUE(use_minimum_storage(filter));
    }
    EXPECT_EQ(debug_view(set), debug_view(filter));
  }
}

FILTER_TEST(Can_insert_ranges) {
  for (const auto &vec : make_test_vectors()) {
    const set_t set(vec.begin(), vec.end());
    filter_t filter;

    const auto middle = vec.begin() + vec.size() / 2;

    filter.insert(vec.begin(), middle);
    EXPECT_TRUE(use_minimum_storage(filter));

    filter.insert(middle, vec.end());
    EXPECT_TRUE(use_minimum_storage(filter));

    EXPECT_EQ(debug_view(set), debug_view(filter));
  }
}

FILTER_TEST(Can_insert_initializer_lists) {
  filter_t filter;
  filter.insert({0, 0, 1, 1, 9, 8, 7, 6, 5, 4, 3, 2});
  EXPECT_TRUE(use_minimum_storage(filter));

  EXPECT_EQ(debug_view(make_iota_vector(10)), debug_view(filter));

  filter.insert({14, 0, 9, 13, 12, 10, 11});
  EXPECT_EQ(debug_view(make_iota_vector(15)), debug_view(filter));
}

FILTER_TEST(Can_emplace_elements) {
  quotient_filter<std::string> filter;
  std::string str(10, 'a');
  EXPECT_FALSE(filter.count(str));
  filter.emplace(10u, 'a');
  EXPECT_TRUE(filter.count(str));
  EXPECT_EQ(1, filter.size());
}

FILTER_TEST(Can_erase_using_iterators) {
  for (const auto &vec : make_test_vectors()) {
    filter_t filter(vec.begin(), vec.end());
    unordered_set_t uset(vec.begin(), vec.end());

    for (const std::size_t hash : uset) {
      const int key = hash_to_key(hash);
      const auto it = filter.find(key);
      ASSERT_NE(filter.end(), it);
      ASSERT_EQ(hash, *it);
      filter.erase(it);
      EXPECT_EQ(filter.end(), filter.find(key));
    }

    EXPECT_TRUE(is_empty(filter));
    EXPECT_TRUE(has_precise_reserve_for(filter, uset.size()));
  }
}

FILTER_TEST(Can_erase_using_keys) {
  for (const auto &vec : make_test_vectors()) {
    filter_t filter(vec.begin(), vec.end());
    unordered_set_t uset(vec.begin(), vec.end());

    for (const std::size_t hash : uset) {
      const int key = hash_to_key(hash);
      ASSERT_EQ(1, filter.count(key));
      EXPECT_EQ(1, filter.erase(key));
      EXPECT_EQ(0, filter.count(key));
      EXPECT_EQ(0, filter.erase(key));
    }

    EXPECT_TRUE(is_empty(filter));
    EXPECT_TRUE(has_precise_reserve_for(filter, uset.size()));
  }
}

FILTER_TEST(Has_member_swap) {
  const auto backup1 = make_test_filter(10);
  const auto backup2 = make_test_filter(1000);
  auto filter1 = backup1;
  auto filter2 = backup2;

  filter1.swap(filter2);

  EXPECT_PRED_FORMAT2(assert_all_equal, backup2, filter1);
  EXPECT_PRED_FORMAT2(assert_all_equal, backup1, filter2);
}

FILTER_TEST(Can_find_and_count_elements) {

  for (const auto &vec : make_test_vectors()) {
    const auto middle = vec.begin() + vec.size() / 2;
    filter_t filter(vec.begin(), middle, vec.size() / 2);

    for_each(vec.begin(), middle, [&filter](int key) {
      auto it = filter.find(key);
      const auto hash_fn = filter.hash_function();

      ASSERT_NE(filter.end(), it);
      EXPECT_EQ(hash_fn(key), *it);
      EXPECT_EQ(1, filter.count(key));
    });

    for_each(middle, vec.end(), [&filter](int key) {
      EXPECT_EQ(filter.end(), filter.find(key));
      EXPECT_EQ(0, filter.count(key));
    });
  }
}

FILTER_TEST(Can_query_load_factor) {

  for (const auto &vec : make_test_vectors()) {
    filter_t filter;
    EXPECT_FLOAT_EQ(1.0f, filter.load_factor());
    for (const int elem : vec) {
      filter.insert(elem);
      EXPECT_FLOAT_EQ(float(filter.size()) / filter.slot_count(),
                      filter.load_factor());
    }
  }
}

FILTER_TEST(Can_change_max_load_factor_and_grow_if_necessary) {
  std::vector<float> factors;
  for (float ml = 0.10f; ml <= 1.00f; ml += 0.01f)
    factors.push_back(ml);
  shuffle(factors);

  const auto backup = make_test_filter(1000);
  auto filter = backup;

  for (const float ml : factors) {
    assert(ml <= 1.0f);
    assert(ml >= 0.10f);

    const size_t slot_count = calc_expected_slot_count(filter.size(), ml);
    const size_t prev_slot_count = filter.slot_count();

    filter.max_load_factor(ml);
    ASSERT_EQ(ml, filter.max_load_factor());

    if (slot_count <= prev_slot_count) {
      EXPECT_EQ(prev_slot_count, filter.slot_count());
    } else {
      EXPECT_EQ(slot_count, filter.slot_count());
    }
  }

  filter.max_load_factor(default_max_load_factor);
  filter.reserve(0);
  EXPECT_PRED_FORMAT2(assert_all_equal, backup, filter);
}

FILTER_TEST(Can_be_adjusted_with_reserve) {
  const auto backup = make_test_filter(15);
  auto filter = backup;

  std::vector<size_t> capacities;
  for (size_t num_elems = 0; num_elems < 1000; num_elems += 5)
    capacities.push_back(num_elems);
  shuffle(capacities);

  for (const size_t num_elems : capacities) {
    filter.reserve(num_elems);
    EXPECT_TRUE(has_precise_reserve_for(filter, max(filter.size(), num_elems)));
  }

  filter.reserve(0);
  EXPECT_PRED_FORMAT2(assert_all_equal, backup, filter);
}

FILTER_TEST(Can_be_equally_compared) {
  const filter_t orig = {1, 2, 3, 4, 5};

  std::vector<filter_t> cheap_copies;

  cheap_copies.emplace_back(orig.begin(), orig.end(), 4 * orig.capacity());
  ASSERT_NE(orig.capacity(), cheap_copies.back().capacity());

  cheap_copies.emplace_back(orig.begin(), orig.end(), orig.capacity(),
                            state_hash{2});
  {
    filter_t temp(orig.begin(), orig.end());
    temp.max_load_factor(default_max_load_factor / 4);
    ASSERT_FALSE(float_equal(orig.max_load_factor(), temp.max_load_factor()));
    cheap_copies.push_back(std::move(temp));
  }

  for (const filter_t &cheap_copy : cheap_copies) {
    ASSERT_TRUE(debug_view(cheap_copy) == debug_view(orig));
    EXPECT_TRUE(cheap_copy == orig);
    ASSERT_FALSE(debug_view(cheap_copy) != debug_view(orig));
    EXPECT_FALSE(cheap_copy != orig);
  }

  const filter_t similar_filter = {6, 7, 8, 9, 10};

  ASSERT_TRUE(debug_view(similar_filter) != debug_view(orig));
  EXPECT_TRUE(similar_filter != orig);
}

FILTER_TEST(Has_non_member_swap) {
  const auto backup_filter1 = make_test_filter(10);
  const auto backup_filter2 = make_test_filter(1000);
  auto filter1 = backup_filter1;
  auto filter2 = backup_filter2;

  swap(filter1, filter2);

  EXPECT_PRED_FORMAT2(assert_all_equal, backup_filter2, filter1);
  EXPECT_PRED_FORMAT2(assert_all_equal, backup_filter1, filter2);
}

FILTER_TEST(Can_insert_and_reallocates_when_needed) {
  filter_t filter;

  filter.max_load_factor(0.5f);
  int_generator<int> gen_value(78340652);

  filter.insert(gen_value());
  EXPECT_EQ(1, filter.capacity());
  EXPECT_EQ(2, filter.slot_count());

  while (filter.size() < 4096) {
    const auto prev_size = filter.size();
    const bool did_insertion = filter.insert(gen_value()).second;

    if (did_insertion)
      EXPECT_EQ(prev_size + 1, filter.size());
    else
      EXPECT_EQ(prev_size, filter.size());

    EXPECT_TRUE(use_minimum_storage(filter));
  }

  EXPECT_EQ(4096, filter.capacity());
  EXPECT_EQ(8192, filter.slot_count());
}

FILTER_TEST(Works_on_arbitrary_types) {
  using std::string;
  quotient_filter<string> filter;
  const string random_key = "abcdef";
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(filter.insert(random_key).second);
  EXPECT_TRUE(filter.count(random_key));
  if (filter.emplace(10u, 'a').second) {
    EXPECT_EQ(2, filter.size());
    string temp(10, 'a');
    filter.erase(filter.find(temp));
    EXPECT_EQ(1, filter.size());
  }

  EXPECT_TRUE(filter.erase(random_key));
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(!use_minimum_storage(filter));
}

FILTER_TEST(Can_mix_insertions_deletions_and_queries) {
  quotient_filter<int, state_hash, 10> filter;
  filter.max_load_factor(1.0f);

  int_generator<int> gen_elem(83792341);
  gen_elem.set_params(0, (1 << filter.fp_bits) - 1);

  repeat(10, [&filter, &gen_elem] {
    set_t set;

    ASSERT_EQ(512, filter.max_size());

    while (filter.size() != filter.max_size()) {
      const int elem = gen_elem();
      const auto pfilter = filter.insert(elem);
      const auto pset = set.insert(elem);
      EXPECT_EQ(elem, *pfilter.first);
      EXPECT_EQ(pset.second, pfilter.second);
    }

    for (std::size_t hash : set) {
      const int key = hash_to_key(hash);
      EXPECT_TRUE(filter.count(key));
      EXPECT_TRUE(filter.erase(key));
      EXPECT_FALSE(filter.count(key));
    }

    EXPECT_TRUE(is_empty(filter));
    EXPECT_TRUE(has_precise_reserve_for(filter, set.size()));
  });
}
