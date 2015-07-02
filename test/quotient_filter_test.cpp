//          Copyright Diego Ramírez June 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter.hpp>
#include <gtest/gtest.h>

#include <algorithm>   // for std::equal, std::shuffle
#include <iterator>    // for std::next, std::begin, std::end
#include <limits>      // for std::numeric_limits
#include <numeric>     // for std::iota
#include <random>      // for std::mt19937, std::random_device
#include <set>         // for std::set
#include <string>      // for std::string
#include <type_traits> // for std::is_integral
#include <utility>     // for std::move
#include <vector>      // for std::vector
#include <cmath>       // for std::ceil, std::isnan, std::fabs

// ==========================================
// General declarations
// ==========================================

#define QFILTER_TEST(test_name) TEST(quotient_filter, test_name)

#ifdef STATIC_ASSERT
#undef STATIC_ASSERT
#endif

#define STATIC_ASSERT(x) static_assert(x, #x)

using std::size_t;

static constexpr float default_max_load_factor = 0.8f;

// ==========================================
// Utilities for tests.
// ==========================================

template <typename Function>
static void repeat(size_t n, Function f) {
  while (n--)
    f();
}

// Redundant equal
template <typename Range1, typename Range2>
static bool equal(const Range1 &lhs, const Range2 &rhs) {
  return lhs.empty() == rhs.empty() && lhs.size() == rhs.size() &&
         std::equal(std::begin(lhs), std::end(lhs), std::begin(rhs),
                    std::end(rhs));
}

// Redundant equal
template <typename Range1, typename T>
static bool equal(const Range1 &lhs, std::initializer_list<T> rhs) {
  return lhs.size() == rhs.size() && std::equal(std::begin(lhs), std::end(lhs),
                                                std::begin(rhs), std::end(rhs));
}

template <typename T>
static constexpr bool float_equal(const T lhs, const T rhs) noexcept {
  STATIC_ASSERT(std::is_floating_point<T>::value);
  return std::fabs(lhs - rhs) < std::numeric_limits<T>::epsilon();
}

template <typename Filter>
static bool totally_equal(const Filter &lhs, const Filter &rhs) {
  return lhs.capacity() == rhs.capacity() &&
         lhs.slot_count() == rhs.slot_count() &&
         float_equal(lhs.max_load_factor(), rhs.max_load_factor()) &&
         float_equal(lhs.load_factor(), rhs.load_factor()) && lhs == rhs &&
         equal(lhs, rhs); // Intentional redudancy applied.
}

template <typename T = int>
static std::vector<T> make_test_vector(size_t num_elems) {
  std::vector<T> vec(num_elems);
  std::iota(vec.begin(), vec.end(), T{0});
  assert(vec.size() == num_elems);
  return vec;
}

// Redundant empty check.
template <typename Filter>
static bool is_empty(const Filter &filter) {
  return filter.empty() && filter.size() == 0 && filter.begin() == filter.end();
}

static size_t
calc_expected_slot_count(const size_t num_elems,
                         const float ml = default_max_load_factor) {
  const size_t min_valid_cap = static_cast<size_t>(std::ceil(num_elems / ml));
  size_t min_num_slots = 1;
  while (min_num_slots < min_valid_cap)
    min_num_slots *= 2;
  return min_num_slots;
}

static size_t calc_expected_capacity(const size_t num_elems,
                                     const float ml = default_max_load_factor) {
  return static_cast<size_t>(calc_expected_slot_count(num_elems, ml) * ml);
}

template <typename Range>
static void shuffle(Range &range) {
  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(std::begin(range), std::end(range), gen);
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

template <typename T>
static auto make_int_generator(const T a = std::numeric_limits<T>::min(),
                               const T b = std::numeric_limits<T>::max()) {
  STATIC_ASSERT(std::is_integral<T>::value);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<T> dist(a, b);
  return [ gen = std::move(gen), dist ]() mutable { return dist(gen); };
}

namespace {
template <typename T>
struct identity_hash {
  STATIC_ASSERT(std::is_integral<T>::value);

  size_t operator()(const T &value) const noexcept {
    return static_cast<size_t>(value);
  }
};
} // End anonymous namespace

template <typename T,
          size_t B = std::numeric_limits<std::make_unsigned_t<T>>::digits>
using qfilter = quofil::quotient_filter<T, identity_hash<T>, B>;

// ==========================================
// QFILTER_TEST Section
// ==========================================

QFILTER_TEST(Can_be_default_constructed) {
  qfilter<int> filter;
  EXPECT_EQ(default_max_load_factor, filter.max_load_factor());
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(use_minimum_storage(filter));
}

QFILTER_TEST(Can_be_constructed_with_required_capacity) {
  const size_t required_capacity = 140;
  qfilter<int> filter(required_capacity);
  EXPECT_EQ(default_max_load_factor, filter.max_load_factor());
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(has_precise_reserve_for(filter, required_capacity));
}

QFILTER_TEST(Can_be_constructed_with_a_range) {
  for (size_t num_elems = 0; num_elems != 5; ++num_elems) {
    const auto vec = make_test_vector(num_elems);
    qfilter<int> filter(vec.begin(), vec.end());
    EXPECT_EQ(default_max_load_factor, filter.max_load_factor());
    EXPECT_TRUE(use_minimum_storage(filter));
    EXPECT_TRUE(equal(filter, vec));
  }
}

QFILTER_TEST(Can_be_constructed_with_a_range_and_required_capacity) {
  for (size_t num_elems = 0; num_elems != 5; ++num_elems) {
    const auto vec = make_test_vector(num_elems);
    const size_t cap =
        2 * std::max(calc_expected_capacity(vec.size()), size_t{1});
    qfilter<int> filter(vec.begin(), vec.end(), cap);
    EXPECT_EQ(default_max_load_factor, filter.max_load_factor());
    EXPECT_FALSE(use_minimum_storage(filter));
    EXPECT_TRUE(has_precise_reserve_for(filter, cap));
    EXPECT_TRUE(equal(filter, vec));
  }
}

QFILTER_TEST(Can_be_constructed_with_initializer_list) {
  qfilter<int> filter = {1, 2, 3, 4, 5};
  EXPECT_EQ(default_max_load_factor, filter.max_load_factor());
  EXPECT_TRUE(use_minimum_storage(filter));
  EXPECT_TRUE(equal(filter, {1, 2, 3, 4, 5}));
}

QFILTER_TEST(Can_be_constructed_with_initializer_list_and_required_capacity) {
  const size_t num_elems = 2 * calc_expected_capacity(5);
  qfilter<int> filter({1, 2, 3, 4, 5}, num_elems);
  EXPECT_EQ(default_max_load_factor, filter.max_load_factor());
  EXPECT_FALSE(use_minimum_storage(filter));
  EXPECT_TRUE(has_precise_reserve_for(filter, num_elems));
  EXPECT_TRUE(equal(filter, {1, 2, 3, 4, 5}));
}

QFILTER_TEST(Can_be_iterated) {
  qfilter<int> filter;
  EXPECT_EQ(filter.begin(), filter.end());
  filter.insert(1);
  auto it = filter.begin();
  EXPECT_EQ(1, *it);
  EXPECT_EQ(1, *it++);
  EXPECT_EQ(filter.end(), it);

  filter.insert({2, 3, 4});
  it = filter.begin();

  EXPECT_EQ(1, *it++);
  EXPECT_EQ(2, *it);
  ++it;
  EXPECT_EQ(3, *it);
  it++;
  EXPECT_NE(filter.begin(), filter.end());
  EXPECT_EQ(4, *it);
  EXPECT_EQ(filter.end(), std::next(it));
}

QFILTER_TEST(Has_max_size_according_to_fp_bits_and_works_well_at_limits) {
  // fp_bits == 10, r_bits must be at least 1 so max q_bits is 9
  qfilter<int, 10> filter;
  filter.max_load_factor(1.0f);
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(use_minimum_storage(filter));

  STATIC_ASSERT(decltype(filter)::fp_bits == 10);
  constexpr size_t max_size = size_t{1} << 9;
  EXPECT_EQ(max_size, filter.max_size());

  std::vector<int> vec(max_size);
  std::iota(vec.begin(), vec.end(), 0);

  for (int elem : vec) {
    EXPECT_TRUE(filter.insert(elem).second);
    EXPECT_TRUE(has_precise_reserve_for(filter, size_t(elem + 1)));
  }

  EXPECT_NO_THROW(filter.insert(vec.back()));
  EXPECT_THROW(filter.insert(vec.back() + 1), std::length_error);
  EXPECT_TRUE(equal(vec, filter));
}

QFILTER_TEST(Can_be_cleared) {
  const size_t num_elems = 1000;
  const auto vec = make_test_vector(num_elems);
  qfilter<int> filter(vec.begin(), vec.end(), num_elems);
  const auto backup_filter = filter;

  ASSERT_EQ(num_elems, filter.size());
  ASSERT_TRUE(totally_equal(filter, backup_filter));
  filter.clear();
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(has_precise_reserve_for(filter, num_elems));
  filter.insert(backup_filter.begin(), backup_filter.end());
  EXPECT_TRUE(totally_equal(backup_filter, filter));

  filter.clear();
  filter.insert({1, 2, 3, 4, 5});
  if (!use_minimum_storage(filter)) {
    filter.reserve(0);
    ASSERT_EQ(5, filter.size());
    EXPECT_TRUE(use_minimum_storage(filter));
  }
  EXPECT_TRUE(totally_equal(filter, qfilter<int>({1, 2, 3, 4, 5})));
}

QFILTER_TEST(Can_insert_single_values) {
  qfilter<int> filter;
  for (int i = 0; i != 10; ++i) {
    EXPECT_TRUE(equal(filter, make_test_vector(i)));
    filter.insert(i);
    EXPECT_TRUE(use_minimum_storage(filter));
  }
  EXPECT_TRUE(equal(filter, make_test_vector(10)));
}

QFILTER_TEST(Can_insert_ranges) {
  qfilter<int> filter = {1, 2, 3};
  const auto vec = make_test_vector(1000);
  filter.insert(vec.begin(), vec.end());
  EXPECT_TRUE(use_minimum_storage(filter));
  EXPECT_TRUE(equal(filter, vec));
}

QFILTER_TEST(Can_insert_initializer_lists) {
  qfilter<int> filter;
  filter.max_load_factor(0.5f);
  filter.insert({0, 0, 1, 1, 9, 8, 7, 6, 5, 4, 3, 2});
  EXPECT_TRUE(use_minimum_storage(filter));
  EXPECT_TRUE(equal(filter, make_test_vector(10)));
}

QFILTER_TEST(Can_emplace_elements_for_getting_hash) {
  quofil::quotient_filter<std::string> filter;
  std::string str(10, 'a');
  EXPECT_FALSE(filter.count(str));
  filter.emplace(10, 'a');
  EXPECT_TRUE(filter.count(str));
}

QFILTER_TEST(Can_erase_using_iterators) {
  qfilter<int> filter;
  const int random_key = 11;
  filter.insert(random_key);
  ASSERT_TRUE(filter.count(random_key));
  auto it = filter.find(random_key);
  ASSERT_NE(it, filter.end());
  filter.erase(it);
  EXPECT_FALSE(filter.count(random_key));
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(has_precise_reserve_for(filter, 1));
}

QFILTER_TEST(Can_erase_using_keys) {
  qfilter<int> filter;
  const int random_key = 11;

  ASSERT_FALSE(filter.erase(random_key));
  filter.insert(random_key);
  EXPECT_TRUE(filter.erase(random_key));
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(has_precise_reserve_for(filter, 1));
}

QFILTER_TEST(Has_member_swap) {
  const auto vec1 = make_test_vector(10);
  const auto vec2 = make_test_vector(1000);
  qfilter<int> filter1(vec1.begin(), vec1.end());
  qfilter<int> filter2(vec2.begin(), vec2.end());

  const qfilter<int> backup_filter1 = filter1;
  const qfilter<int> backup_filter2 = filter2;

  filter1.swap(filter2);

  EXPECT_TRUE(totally_equal(filter1, backup_filter2));
  EXPECT_TRUE(totally_equal(filter2, backup_filter1));
}

QFILTER_TEST(Can_count_appearances) {
  qfilter<int> filter = {1, 2, 3, 4, 5};

  for (int elem : {1, 2, 3, 4, 5})
    EXPECT_TRUE(filter.count(elem));

  for (int elem : {6, 7, 8, 9, 10})
    EXPECT_FALSE(filter.count(elem));
}

QFILTER_TEST(Can_find_hashes) {
  qfilter<int> filter = {1, 2, 3, 4, 5};

  for (int key : {1, 2, 3, 4, 5}) {
    auto it = filter.find(key);
    ASSERT_NE(it, filter.end());
    EXPECT_EQ(key, *it);
  }

  for (int key : {6, 7, 8, 9, 10})
    EXPECT_EQ(filter.end(), filter.find(key));
}

QFILTER_TEST(Can_query_load_factor) {
  qfilter<int> filter;
  EXPECT_TRUE(std::isnan(filter.load_factor()));

  for (int elem : make_test_vector(1000)) {
    filter.insert(elem);
    const float expected_lf = float(filter.size()) / filter.slot_count();
    EXPECT_FLOAT_EQ(expected_lf, filter.load_factor());
  }
}

QFILTER_TEST(Can_change_max_load_factor_and_grow_if_necessary) {
  std::vector<float> factors;
  for (float ml = 0.1f; ml <= 1.0f; ml += 0.02f)
    factors.push_back(ml);
  shuffle(factors);

  const auto vec = make_test_vector(1000);
  qfilter<int> filter(vec.begin(), vec.end());
  ASSERT_EQ(1000, filter.size());

  for (const float ml : factors) {
    ASSERT_LE(ml, 1.0f);
    ASSERT_GE(ml, 0.1f);

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
}

QFILTER_TEST(Can_be_adjusted_with_reserve) {
  qfilter<int> filter;
  filter.max_load_factor(1.0f);

  auto vec = make_test_vector<size_t>(3000);
  shuffle(vec);

  for (const size_t num_elems : vec) {
    filter.reserve(num_elems);
    EXPECT_TRUE(has_precise_reserve_for(filter, num_elems));
    EXPECT_TRUE(is_empty(filter));
  }
}

QFILTER_TEST(Can_get_hash_function) {
  struct state_hash {
    state_hash(int val = 0) : value{val} {}
    size_t operator()(int key) const { return key * value; }
    int value;
  };

  using filter_t = quofil::quotient_filter<int, state_hash>;
  const filter_t filter1;
  EXPECT_EQ(0, filter1.hash_function().value);

  const filter_t filter2(0, state_hash(5));
  const auto hash_fn = filter2.hash_function();
  EXPECT_EQ(5, hash_fn.value);
  EXPECT_EQ(55, hash_fn(11));
}

QFILTER_TEST(Can_be_equally_compared) {
  const qfilter<int> filter1 = {1, 2, 3, 4, 5};
  const qfilter<int> filter2({1, 2, 3, 4, 5}, filter1.capacity() * 3);
  const qfilter<int> filter3 = {6, 7, 8, 9, 10};

  ASSERT_NE(filter1.capacity(), filter2.capacity());
  EXPECT_TRUE(filter1 == filter2);
  EXPECT_FALSE(filter2 == filter3);
  EXPECT_FALSE(filter1 != filter2);
  EXPECT_TRUE(filter1 != filter3);
}

QFILTER_TEST(Has_non_member_swap) {
  const auto vec1 = make_test_vector(10);
  const auto vec2 = make_test_vector(1000);
  qfilter<int> filter1(vec1.begin(), vec1.end());
  qfilter<int> filter2(vec2.begin(), vec2.end());

  const qfilter<int> backup_filter1 = filter1;
  const qfilter<int> backup_filter2 = filter2;

  swap(filter1, filter2);

  EXPECT_TRUE(totally_equal(filter1, backup_filter2));
  EXPECT_TRUE(totally_equal(filter2, backup_filter1));
}

QFILTER_TEST(Can_insert_and_reallocates_when_needed) {
  qfilter<int> filter;

  filter.max_load_factor(0.5f);
  auto gen_value = make_int_generator<int>();

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

QFILTER_TEST(WorksOnArbitraryTypes) {
  quofil::quotient_filter<std::string> filter;
  const std::string random_key = "abcdef";
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(filter.insert(random_key).second);
  EXPECT_TRUE(filter.count(random_key));
  if (filter.emplace(10, 'a').second) {
    EXPECT_EQ(2, filter.size());
    std::string temp(10, 'a');
    filter.erase(filter.find(temp));
    EXPECT_EQ(1, filter.size());
  }

  EXPECT_TRUE(filter.erase(random_key));
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(!use_minimum_storage(filter));
}

QFILTER_TEST(Can_mix_insertions_deletions_and_queries) {
  qfilter<int> filter;
  std::set<int> set;

  auto gen_elem = make_int_generator(0, 2000);

  repeat(1000, [&] {
    const int elem = gen_elem();
    const auto pfilter = filter.insert(elem);
    const auto pset = set.insert(elem);
    EXPECT_EQ(elem, *pfilter.first);
    EXPECT_EQ(pset.second, pfilter.second);
  });

  for (int elem : set) {
    EXPECT_TRUE(filter.count(elem));
    EXPECT_TRUE(filter.erase(elem));
    EXPECT_FALSE(filter.count(elem));
  }

  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(has_precise_reserve_for(filter, set.size()));
}
