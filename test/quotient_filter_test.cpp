//          Copyright Diego Ramírez July 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter.hpp>
#include <gtest/gtest.h>

#include <limits>      // for std::numeric_limits
#include <random>      // for std::mt19937
#include <type_traits> // for std::is_same
#include <utility>     // for std::move

template <class Function>
static void repeat(std::size_t n, Function f) {
  while (n--)
    f();
}

template <class T>
static auto make_int_generator() {
  static_assert(std::is_integral<T>::value, "Must be integral");
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<T> dist(std::numeric_limits<T>::min(),
                                        std::numeric_limits<T>::max());
  return [ gen = std::move(gen), dist ]() mutable { return dist(gen); };
}

template <class T>
struct identity_hash {
  static_assert(std::is_integral<T>::value, "Must be integral");

  static constexpr std::size_t bits = std::numeric_limits<T>::digits;

  std::size_t operator()(const T &value) const { return static_cast<T>(value); }
};

template <class T, class Hash = identity_hash<T>>
using qfilter = quofil::quotient_filter<T, Hash, Hash::bits>;

TEST(quotient_filter, ResizeWell) {
  qfilter<int> filter;
  filter.max_load_factor(0.5f);
  auto gen_value = make_int_generator<int>();

  while (filter.size() < 4096) {
    const auto prev_capacity = filter.capacity();
    const auto prev_size = filter.size();
    if (filter.insert(gen_value()).second) {
      if (2 * prev_size == prev_capacity) {
        EXPECT_EQ(2 * prev_capacity, filter.capacity());
      }
      EXPECT_EQ(prev_size + 1, filter.size());
    } else {
      EXPECT_EQ(prev_capacity, filter.capacity());
      EXPECT_EQ(prev_size, filter.size());
    }
  }

  EXPECT_EQ(8192, filter.capacity());
}

//#include <chrono>
//#include <unordered_set>
// TEST(quotient_filter, InsertionBenchmark) {
//  qfilter<int> filter(0);
//  std::cout << filter.capacity() << '\n';
//
//  auto gen_value = make_int_generator<int>();
//  using std::chrono::steady_clock;
//  using std::chrono::duration_cast;
//
//  const auto start = steady_clock::now();
//  repeat(100000, [&] { filter.insert(gen_value()); });
//  const auto end = steady_clock::now();
//  const auto elapsed = duration_cast<std::chrono::milliseconds>(end - start);
//  std::cout << "Elapsed: " << elapsed.count() << " ms\n";
//  std::cout << "Load factor: " << filter.load_factor() << '\n';
//  std::cout << "Max load factor: " << filter.max_load_factor() << '\n';
//}
