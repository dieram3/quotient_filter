//          Copyright Diego Ramírez June 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter.hpp>
#include <gtest/gtest.h>

#include <algorithm>     //
#include <functional>    // for std::ref
#include <ios>           //
#include <iterator>      //
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
#include <vector>        //
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

// From <iterator>
using std::back_inserter;
using std::begin;
using std::end;
// Also use advance, next

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

// From <unordered_set>
using std::unordered_set;

// From <vector>
using std::vector;
using std::initializer_list;

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

template <typename NullaryFunction>
static void repeat(size_t n, NullaryFunction f) {
  while (n--)
    f();
}

template <typename T = int>
static vector<T> make_iota_vector(size_t num_elems) {
  vector<T> vec(num_elems);
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
  std::shuffle(std::begin(range), std::end(range), mt19937(seq));
}

template <typename Range, typename UnaryFunction>
static void for_each(const Range &range, UnaryFunction f) {
  std::for_each(std::begin(range), std::end(range), f);
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
  int_generator(const std::mt19937::result_type seed) : gen(seed) {}

  template <typename Sseq>
  int_generator(Sseq &s)
      : gen(s) {}

  void set_params(const T a, const T b) {
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
    for (const auto &elem : view)
      os << ' ' << elem;
    return os << " ]";
  }
  auto it = view.begin();
  os << '[';
  repeat(5, [&] { os << ' ' << *it++; });
  os << " ... ";
  std::advance(it, ptrdiff_t(view.size()) - 10);
  repeat(5, [&] { os << *it++ << ' '; });
  assert(it == view.end());
  return os << ']';
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

template <typename SetT>
class basic_hash_set {
  SetT set;
  state_hash hash_fn{};

public:
  basic_hash_set() {}

  template <typename InputIt>
  basic_hash_set(InputIt first, InputIt last) {
    insert(first, last);
  }

  auto insert(int elem) { return set.insert(hash_fn(elem)); }

  template <typename InputIt>
  void insert(InputIt first, InputIt last) {
    for_each(first, last, [this](int elem) { insert(elem); });
  }

  auto begin() const { return set.begin(); }
  auto end() const { return set.end(); }
  auto empty() const { return set.empty(); }
  auto size() const { return set.size(); }
};

} // End anonymous namespace

// ==========================================
// Type aliases
// ==========================================

using ordered_hash_set = basic_hash_set<std::set<size_t>>;
using unordered_hash_set = basic_hash_set<unordered_set<size_t>>;
using filter_t =
    quotient_filter<int, state_hash, numeric_limits<unsigned>::digits>;

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
// Quotient-Filter extended query functions
// ==========================================

// Redundant empty check.
template <typename T, typename H, size_t B>
static bool is_empty(const quotient_filter<T, H, B> &filter) {
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
template <typename T, typename H, size_t B>
static bool has_precise_reserve_for(const quotient_filter<T, H, B> &filter,
                                    const size_t num_elems) {

  if (num_elems == 0) {
    return filter.capacity() == 0 && filter.slot_count() == 0;
  }

  const float ml = filter.max_load_factor();

  return filter.capacity() == calc_expected_capacity(num_elems, ml) &&
         filter.slot_count() == calc_expected_slot_count(num_elems, ml);
}

template <typename T, typename H, size_t B>
static bool use_minimum_storage(const quotient_filter<T, H, B> &filter) {
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
  seed_seq seq = {size, size * 11, size / 11, size % 11};
  int_generator<int> gen_elem(seq);
  filter_t filter(size);
  while (filter.size() != size)
    filter.insert(gen_elem());
  return filter;
}

// ==========================================
// FilterTest class
// ==========================================

namespace {

class FilterTest : public ::testing::Test {
protected:
  void SetUp() override;

protected:
  vector<vector<int>> test_vectors;
  vector<size_t> test_capacities;
  vector<state_hash> test_hash_functions;

private:
  static vector<int> generate_vector(const size_t size, mt19937 &engine);
  static vector<vector<int>> make_test_vectors(initializer_list<size_t>);
};

void FilterTest::SetUp() {
  test_vectors = make_test_vectors({0, 1, 10, 50, 130, 350});
  test_capacities = {0, 1, 2, 30, 50, 140};
  test_hash_functions = {0, 2, 5, -1};
}

// Generates a vector uniformly distributed in the range of 'int' using the
// given random number engine. The size will be size + size / 10. At least
// size/10 elements will be repeated.
vector<int> FilterTest::generate_vector(const size_t size, mt19937 &engine) {
  uniform_int_distribution<int> dist(numeric_limits<int>::min(),
                                     numeric_limits<int>::max());
  vector<int> vec(size + size / 10);
  generate(begin(vec), end(vec), [&] { return dist(engine); });
  copy(end(vec) - size / 10, end(vec), begin(vec));
  shuffle(begin(vec), end(vec), engine);
  return vec;
}

// Generates a sequence of vectors according the given sizes.
// Each generated vector will be uniformly distributed in the range of 'int'.
vector<vector<int>>
FilterTest::make_test_vectors(const initializer_list<size_t> sizes) {

  // Use the given sizes for getting randomness.
  seed_seq seq(sizes);
  mt19937 engine(seq);

  vector<vector<int>> vectors;
  vectors.reserve(sizes.size());
  for (const size_t size : sizes)
    vectors.push_back(generate_vector(size, engine));

  return vectors;
}

} // End anonymous namespace

// ==========================================
// Tests section
// ==========================================

TEST_F(FilterTest, DefaultConstructor) {
  EXPECT_PRED_FORMAT2(assert_all_equal, filter_t(0), filter_t());
}

TEST_F(FilterTest, CopyConstructor) {
  for_each(test_vectors, [](const vector<int> &data) {
    const filter_t orig(begin(data), end(data));
    EXPECT_PRED_FORMAT2(assert_all_equal, orig, filter_t(orig));
  });
}

TEST_F(FilterTest, CopyAssignment) {
  for_each(test_vectors, [](const vector<int> &data) {
    const filter_t orig(begin(data), end(data));
    filter_t new_filter;
    EXPECT_PRED_FORMAT2(assert_all_equal, orig, new_filter = orig);
  });
}

TEST_F(FilterTest, MoveConstructor) {
  for_each(test_vectors, [](const vector<int> &data) {
    filter_t orig(begin(data), end(data));
    const filter_t backup = orig;
    EXPECT_PRED_FORMAT2(assert_all_equal, backup, filter_t(std::move(orig)));
    // The moved filter can be remade
    orig = backup;
    EXPECT_PRED_FORMAT2(assert_all_equal, backup, orig);
  });
}

TEST_F(FilterTest, MoveAssignment) {
  for_each(test_vectors, [](const vector<int> &data) {
    filter_t orig(begin(data), end(data));
    const filter_t backup = orig;
    filter_t new_filter;
    EXPECT_PRED_FORMAT2(assert_all_equal, backup, new_filter = std::move(orig));
    // The moved filter can be remade
    orig = backup;
    EXPECT_PRED_FORMAT2(assert_all_equal, backup, orig);
  });
}

TEST_F(FilterTest, CapacityConstructor) {
  for (const size_t cap : test_capacities) {
    for (const state_hash hash_fn : test_hash_functions) {
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

TEST_F(FilterTest, RangeConstructor) {
  for (const vector<int> &data : test_vectors) {
    for (const size_t cap : test_capacities) {
      for (const state_hash hash_fn : test_hash_functions) {
        filter_t expected_filter(cap, hash_fn);
        expected_filter.insert(begin(data), end(data));
        EXPECT_PRED_FORMAT2(assert_all_equal, expected_filter,
                            filter_t(begin(data), end(data), cap, hash_fn));
      }
      EXPECT_PRED_FORMAT2(assert_all_equal,
                          filter_t(begin(data), end(data), cap, state_hash()),
                          filter_t(begin(data), end(data), cap));
    }
    EXPECT_PRED_FORMAT2(assert_all_equal, filter_t(begin(data), end(data), 0),
                        filter_t(begin(data), end(data)));
  }
}

TEST_F(FilterTest, InitializerListConstructor) {
  for (const size_t cap : test_capacities) {
    for (const state_hash hash_fn : test_hash_functions) {
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

TEST_F(FilterTest, Iterators) {
  for_each(test_vectors, [](const vector<int> &data) {
    const ordered_hash_set hash_set(begin(data), end(data));
    const filter_t filter(begin(data), end(data), hash_set.size());
    EXPECT_TRUE(
        std::equal(begin(hash_set), end(hash_set), begin(filter), end(filter)));
  });

  const filter_t filter;
  EXPECT_TRUE(begin(filter) == end(filter));
}

TEST_F(FilterTest, MaxSizeAndLimits) {
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

TEST_F(FilterTest, Clear) {
  const size_t num_elems = 1000;
  const auto backup = make_test_filter(num_elems);
  auto filter = backup;

  ASSERT_EQ(num_elems, filter.size());
  filter.clear();
  EXPECT_TRUE(is_empty(filter));
  EXPECT_TRUE(has_precise_reserve_for(filter, num_elems));
  filter.insert(backup.begin(), backup.end());
  EXPECT_PRED_FORMAT2(assert_all_equal, backup, filter);

  filter.clear();
  filter.insert({1, 2, 3, 4, 5});
  if (!use_minimum_storage(filter)) {
    filter.reserve(0);
    ASSERT_EQ(5, filter.size());
    EXPECT_TRUE(use_minimum_storage(filter));
  }

  EXPECT_PRED_FORMAT2(assert_all_equal, filter_t({1, 2, 3, 4, 5}), filter);
}

TEST_F(FilterTest, SingleInsert) {
  for_each(test_vectors, [](const vector<int> &data) {
    filter_t filter;
    ordered_hash_set hash_set;
    for (const int elem : data) {
      const auto filter_ans = filter.insert(elem);
      const auto set_ans = hash_set.insert(elem);
      EXPECT_EQ(set_ans.second, filter_ans.second);
      ASSERT_TRUE(filter_ans.first != end(filter));
      EXPECT_TRUE(filter_ans.first == filter.find(elem));
      EXPECT_TRUE(use_minimum_storage(filter));
    }
    EXPECT_EQ(debug_view(hash_set), debug_view(filter));
  });
}

TEST_F(FilterTest, RangeInsert) {
  for_each(test_vectors, [](const vector<int> &data) {
    const ordered_hash_set hash_set(begin(data), end(data));
    filter_t filter;

    const auto middle = begin(data) + data.size() / 2;

    filter.insert(begin(data), middle);
    EXPECT_TRUE(use_minimum_storage(filter));

    filter.insert(middle, end(data));
    EXPECT_TRUE(use_minimum_storage(filter));

    EXPECT_EQ(debug_view(hash_set), debug_view(filter));
  });
}

TEST_F(FilterTest, InitializerListInsert) {
  filter_t filter;

  filter.insert({0, 0, 1, 1, 9, 8, 7, 6, 5, 4, 3, 2});
  EXPECT_TRUE(use_minimum_storage(filter));
  EXPECT_EQ(debug_view(make_iota_vector(10)), debug_view(filter));

  filter.insert({14, 0, 9, 13, 12, 10, 11});
  EXPECT_TRUE(use_minimum_storage(filter));
  EXPECT_EQ(debug_view(make_iota_vector(15)), debug_view(filter));
}

TEST_F(FilterTest, Emplace) {
  quotient_filter<std::string> filter;
  std::string str(10, 'a');
  EXPECT_FALSE(filter.count(str));
  filter.emplace(10u, 'a');
  EXPECT_TRUE(filter.count(str));
  EXPECT_EQ(1, filter.size());
}

TEST_F(FilterTest, IteratorErase) {
  for_each(test_vectors, [](const vector<int> &data) {
    filter_t filter(begin(data), end(data));
    unordered_hash_set hash_set(begin(data), end(data));

    for (const size_t hash : hash_set) {
      const int key = hash_to_key(hash);
      const auto it = filter.find(key);
      ASSERT_NE(end(filter), it);
      ASSERT_EQ(hash, *it);
      filter.erase(it);
      EXPECT_EQ(end(filter), filter.find(key));
    }

    EXPECT_TRUE(is_empty(filter));
    EXPECT_TRUE(has_precise_reserve_for(filter, hash_set.size()));
  });
}

TEST_F(FilterTest, KeyErase) {
  for_each(test_vectors, [](const vector<int> &data) {
    filter_t filter(begin(data), end(data));
    unordered_hash_set hash_set(begin(data), end(data));

    for (const size_t hash : hash_set) {
      const int key = hash_to_key(hash);
      ASSERT_EQ(1, filter.count(key));
      EXPECT_EQ(1, filter.erase(key));
      EXPECT_EQ(0, filter.count(key));
      EXPECT_EQ(0, filter.erase(key));
    }

    EXPECT_TRUE(is_empty(filter));
    EXPECT_TRUE(has_precise_reserve_for(filter, hash_set.size()));
  });
}

TEST_F(FilterTest, MemberSwap) {
  const auto backup1 = make_test_filter(10);
  const auto backup2 = make_test_filter(1000);
  auto filter1 = backup1;
  auto filter2 = backup2;

  filter1.swap(filter2);

  EXPECT_PRED_FORMAT2(assert_all_equal, backup2, filter1);
  EXPECT_PRED_FORMAT2(assert_all_equal, backup1, filter2);
}

TEST_F(FilterTest, Find) {

  for_each(test_vectors, [](const vector<int> &the_data) {
    const unordered_set<int> dataset(begin(the_data), end(the_data));

    const auto middle = next(begin(dataset), dataset.size() / 2);
    const filter_t filter(begin(dataset), middle, dataset.size() / 2);

    for_each(begin(dataset), middle, [&filter](int key) {
      auto it = filter.find(key);
      const auto hash_fn = filter.hash_function();

      ASSERT_NE(end(filter), it);
      EXPECT_EQ(hash_fn(key), *it);
    });

    for_each(middle, end(dataset),
             [&filter](int key) { EXPECT_EQ(end(filter), filter.find(key)); });
  });
}

TEST_F(FilterTest, Count) {
  for_each(test_vectors, [](const vector<int> &the_data) {
    const unordered_set<int> dataset(begin(the_data), end(the_data));
    const auto middle = next(begin(dataset), dataset.size() / 2);
    const filter_t filter(begin(dataset), middle, dataset.size() / 2);

    for_each(begin(dataset), middle,
             [&filter](int key) { EXPECT_EQ(1, filter.count(key)); });

    for_each(middle, end(dataset),
             [&filter](int key) { EXPECT_EQ(0, filter.count(key)); });
  });
}

TEST_F(FilterTest, LoadFactor) {

  for_each(test_vectors, [](const vector<int> &data) {
    filter_t filter;
    EXPECT_FLOAT_EQ(1.0f, filter.load_factor());
    for (const int elem : data) {
      filter.insert(elem);
      EXPECT_FLOAT_EQ(float(filter.size()) / filter.slot_count(),
                      filter.load_factor());
    }
  });
}

TEST_F(FilterTest, MaxLoadFactor) {
  vector<float> factors;
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

TEST_F(FilterTest, Reserve) {
  const auto backup = make_test_filter(15);
  auto filter = backup;

  vector<size_t> capacities;
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

TEST_F(FilterTest, EqualComparison) {
  const filter_t orig = {1, 2, 3, 4, 5};

  vector<filter_t> cheap_copies;

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

TEST_F(FilterTest, NonMemberSwap) {
  const auto backup1 = make_test_filter(10);
  const auto backup2 = make_test_filter(1000);
  auto filter1 = backup1;
  auto filter2 = backup2;

  swap(filter1, filter2);

  EXPECT_PRED_FORMAT2(assert_all_equal, backup2, filter1);
  EXPECT_PRED_FORMAT2(assert_all_equal, backup1, filter2);
}

TEST_F(FilterTest, InsertionQueryAndDeletionMix) {
  quotient_filter<int, state_hash, 10> filter;
  filter.max_load_factor(1.0f);

  int_generator<int> gen_elem(83792341);
  gen_elem.set_params(0, (1 << filter.fp_bits) - 1);

  repeat(10, [&filter, &gen_elem] {
    unordered_hash_set hash_set;

    ASSERT_EQ(512, filter.max_size());

    while (filter.size() != filter.max_size()) {
      const int elem = gen_elem();
      const auto pfilter = filter.insert(elem);
      const auto pset = hash_set.insert(elem);
      EXPECT_EQ(elem, *pfilter.first);
      EXPECT_EQ(pset.second, pfilter.second);
    }

    for (size_t hash : hash_set) {
      const int key = hash_to_key(hash);
      EXPECT_EQ(1, filter.count(key));
      EXPECT_EQ(1, filter.erase(key));
      EXPECT_EQ(0, filter.count(key));
    }

    EXPECT_TRUE(is_empty(filter));
    EXPECT_TRUE(has_precise_reserve_for(filter, hash_set.size()));
  });
}
