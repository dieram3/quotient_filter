#include <quofil/quotient_filter.hpp>

#include <algorithm> // for std::min
#include <vector>    // for std::vector
#include <cassert>   // for assert
#include <cstddef>   // for std::size_t
#include <cstdint>   // for std::uint64_t

using std::size_t;
using std::uint64_t;

/// Computes the ceil of a / b
template <class T>
static constexpr T ceil_div(T a, T b) noexcept {
  return a / b + T(a % b == 0 ? 0 : 1);
}

template <class T>
static constexpr T low_mask(std::size_t num_bits) noexcept {
  return ~(~static_cast<T>(0) << num_bits);
}

// template <class T>
// static constexpr T high_mask(std::size_t num_bits) noexcept {
//  return ~(~static_cast<T>(0) >> num_bits);
//}

namespace {
class bits_manager {
  size_t q_bits;
  size_t r_bits;
  size_t num_slots;
  std::vector<bool> flags;
  std::unique_ptr<uint64_t[]> data;

private:
  // Computes the number of bytes required to the data table.
  size_t required_blocks() const noexcept {
    const size_t required_bits = num_slots * r_bits;
    return ceil_div<size_t>(required_bits, 64);
  }

public:
  bits_manager(size_t q, size_t r)
      : q_bits{q}, r_bits{r}, num_slots{1ULL << q}, flags(3 * num_slots),
        data(std::make_unique<uint64_t[]>(required_blocks())) {}

  bool is_ocuppied(size_t pos) const noexcept { return flags[3 * pos + 0]; }

  bool is_continuation(size_t pos) const noexcept { return flags[3 * pos + 1]; }

  bool is_shifted(size_t pos) const noexcept { return flags[3 * pos + 2]; }

  void set_ocuppied(size_t pos, bool value) noexcept {
    flags[3 * pos + 0] = value;
  }

  void set_continuation(size_t pos, bool value) noexcept {
    flags[3 * pos + 1] = value;
  }
  void set_shifted(size_t pos, bool value) noexcept {
    flags[3 * pos + 2] = value;
  }

  size_t quotient_bits() const noexcept { return q_bits; }

  size_t remainder_bits() const noexcept { return r_bits; }

  uint64_t get_remainder(const size_t pos) const noexcept {
    const size_t num_bit = r_bits * pos;
    const size_t block = num_bit / 64;
    const size_t offset = num_bit % 64;

    size_t pending_bits = r_bits;
    size_t bits_to_read = std::min(pending_bits, 64 - offset);

    uint64_t ans = (data[block] >> offset) & low_mask<uint64_t>(bits_to_read);
    pending_bits -= bits_to_read;
    if (pending_bits) {
      uint64_t next = data[block + 1] & low_mask<uint64_t>(pending_bits);
      ans |= next << bits_to_read;
    }
    return ans;
  }

  // Requires: value < 2^r_bits
  void set_remainder(const size_t pos, const uint64_t value) noexcept {

    assert(value == (value & low_mask<uint64_t>(r_bits)));

    const size_t num_bit = r_bits * pos;
    const size_t block = num_bit / 64;
    const size_t offset = num_bit % 64;

    size_t pending_bits = r_bits;
    size_t bits_to_write = std::min(pending_bits, 64 - offset);

    data[block] &= ~(low_mask<uint64_t>(bits_to_write) << offset);
    data[block] |= value << offset;

    pending_bits -= bits_to_write;
    if (pending_bits) {
      data[block + 1] &= ~low_mask<uint64_t>(pending_bits);
      data[block + 1] |= value >> bits_to_write;
    }
  }
};

} // end anonymous namespace

using namespace quofil::detail;

//#include <random>
//#include <iostream>
// static void qf_table_test(size_t q, size_t r) {
//  std::mt19937 gen;
//  std::uniform_int_distribution<uint64_t> dist(0, (1ULL << r) - 1);
//  const size_t num_slots = 1 << q;
//
//  qf_table table(q, r);
//  std::vector<uint64_t> vec(num_slots);
//
//  for (size_t i = 0; i < num_slots; ++i) {
//    const auto rem = dist(gen);
//    table.set_remainder(i, rem);
//    vec[i] = rem;
//  }
//
//  for (size_t i = 0; i < num_slots; ++i) {
//    const auto actual = table.get_remainder(i);
//    const auto expected = vec[i];
//    if (actual != expected) {
//      std::cout << "Error at index " << i << '\n';
//      std::cout << "Expected: " << expected << '\n';
//      std::cout << "Actual: " << actual << '\n';
//      return;
//    }
//  }
//  std::cout << "Ok\n";
//}

quotient_filter_table::quotient_filter_table(size_t q, size_t r) {
  // qf_table_test(q, r);
}
