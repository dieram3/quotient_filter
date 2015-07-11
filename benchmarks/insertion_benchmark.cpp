//          Copyright Diego Ramírez July 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <quofil/quotient_filter.hpp>

#include <algorithm>     // for std::generate
#include <iostream>      // for std::cout
#include <vector>        // for std::vector
#include <random>        // for std::random_device, std::mt19937
#include <unordered_set> // for std::unordered_set
#include <climits>       // for INT_MIN, INT_MAX

struct self_hash {
  size_t operator()(int key) const { return static_cast<unsigned>(key); }
};

class steady_timer {
public:
  using clock = std::chrono::steady_clock;
  using duration = clock::duration;
  using time_point = clock::time_point;

public:
  void start() { start_time = clock::now(); }

  void finish() { end_time = clock::now(); }

  duration elapsed() const { return (end_time - start_time); }

  auto elapsed_ms() const {
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;
    return duration_cast<milliseconds>(elapsed()).count();
  }

private:
  time_point start_time{};
  time_point end_time{};
};

// Returns an unsigned integer type representing the number of milliseconds.
template <class HashSet>
static auto measure(const size_t num_elems, const float ml, bool reserve) {
  using std::chrono::steady_clock;
  using std::chrono::duration;
  using std::chrono::milliseconds;

  HashSet set;
  steady_timer timer;
  std::seed_seq seq{num_elems, num_elems * 11, num_elems % 11};
  std::mt19937 gen(seq);
  std::uniform_int_distribution<int> dist(INT_MIN, INT_MAX);

  set.max_load_factor(ml);
  if (reserve)
    set.reserve(num_elems);

  std::vector<int> vec(num_elems);
  std::generate(vec.begin(), vec.end(), [&] { return dist(gen); });

  timer.start();
  set.insert(vec.begin(), vec.end());
  timer.finish();
  return timer.elapsed_ms();
}

int main(const int argc, char *const argv[]) {
  if (argc != 1) {
    std::cerr << "Usage: " << argv[0] << '\n';
    return -1;
  }

  using std::cout;
  cout << "INSERTION BENCHMARKS\n\n";
  size_t num_benchmark = 0;

  for (const size_t num_elems :
       {1'000u, 10'000u, 100'000u, 1'000'000u, 10'000'000u}) {
    for (const float ml : {0.10f, 0.25f, 0.40f, 0.50f, 0.60f, 0.75f, 0.90f})
      for (const bool reserve : {true, false}) {
        cout << "Benchmark " << ++num_benchmark << '\n';
        cout << "================\n\n";
        cout << "Elements to insert: " << num_elems << '\n';
        cout << "Max load factor: " << 100.0f * ml << "%\n";
        cout << "Reserves storage: " << (reserve ? "Yes" : "No") << '\n';
        cout << '\n';
        {
          using filter_t = quofil::quotient_filter<int, self_hash, 32>;
          const auto elapsed = measure<filter_t>(num_elems, ml, reserve);
          cout << "Quotient filter: " << elapsed << " ms\n";
        }
        {
          using set_t = std::unordered_set<int>;
          const auto elapsed = measure<set_t>(num_elems, ml, reserve);
          cout << "Unordered set: " << elapsed << " ms\n";
        }
        cout << "\n\n";
      }
  }
}
