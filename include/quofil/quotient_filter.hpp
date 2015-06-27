// The MIT License (MIT)
//
// Copyright (c) <June 2015> <Diego Ramirez and Marcello Tavano>
// any issue can mail to <(diego.ramirezd and marcello.tavanolanas)@mail.udp.cl>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
/// \file
/// \brief Contains the quotient_filter template class.

#ifndef QUOFIL_QUOTIENT_FILTER_HPP
#define QUOFIL_QUOTIENT_FILTER_HPP

#include <algorithm>  // for std::min
#include <functional> // for std::hash, std::equal_to
#include <limits>     // for std::numeric_limits
#include <memory>     // for std::unique_ptr
#include <vector>     // for std::vector
#include <cassert>    // for assert
#include <cstddef>    // for std::size_t

namespace quofil {

using uchar = unsigned char;

namespace detail {

class bits_manager {

  using block_type = unsigned long long;

  static constexpr size_t bits_per_block =
      std::numeric_limits<block_type>::digits;

  size_t q_bits;
  size_t r_bits;
  size_t num_slots;
  std::vector<bool> flags;
  std::unique_ptr<uint64_t[]> data;

private:
  // Make a mask with the num_bits lsb bits set to 1.
  static constexpr block_type low_mask(std::size_t num_bits) noexcept {
    return ~(~static_cast<block_type>(0) << num_bits);
  }

  // Computes the ceil of x/y
  static constexpr block_type ceil_div(block_type x, block_type y) noexcept {
    return x / y + block_type(x % y == 0 ? 0 : 1);
  }

  // Computes the number of bytes required to the data table.
  size_t required_blocks() const noexcept {
    const size_t required_bits = num_slots * r_bits;
    return ceil_div(required_bits, bits_per_block);
  }

public:
  bits_manager(size_t q, size_t r)
      : q_bits{q}, r_bits{r}, num_slots{1ULL << q}, flags(3 * num_slots),
        data(std::make_unique<uint64_t[]>(required_blocks())) {
    assert(r_bits <= bits_per_block);
  }

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

  block_type get_remainder(const size_t pos) const noexcept {
    const size_t num_bit = r_bits * pos;
    const size_t block = num_bit / bits_per_block;
    const size_t offset = num_bit % bits_per_block;

    size_t pending_bits = r_bits;
    size_t bits_to_read = std::min(pending_bits, bits_per_block - offset);

    block_type ans = (data[block] >> offset) & low_mask(bits_to_read);
    pending_bits -= bits_to_read;
    if (pending_bits) {
      uint64_t next = data[block + 1] & low_mask(pending_bits);
      ans |= next << bits_to_read;
    }
    return ans;
  }

  // Requires: value < 2^r_bits
  void set_remainder(const size_t pos, const uint64_t value) noexcept {

    assert(value == (value & low_mask(r_bits)));

    const size_t num_bit = r_bits * pos;
    const size_t block = num_bit / bits_per_block;
    const size_t offset = num_bit % bits_per_block;

    size_t pending_bits = r_bits;
    size_t bits_to_write = std::min(pending_bits, bits_per_block - offset);

    data[block] &= ~(low_mask(bits_to_write) << offset);
    data[block] |= value << offset;

    pending_bits -= bits_to_write;
    if (pending_bits) {
      data[block + 1] &= ~low_mask(pending_bits);
      data[block + 1] |= value >> bits_to_write;
    }
  }
};

} // end namespace detail

template <class Key, class Hash = std::hash<Key>,
          class Allocator = std::allocator<Key>>
class quotient_filter {
public:
  void insert(const Key &elem);
  void erase(const Key &elem);
  bool contains(const Key &elem) const;
  double error_probability() const;
};

} // end namespace quofil

#endif // Header guard
