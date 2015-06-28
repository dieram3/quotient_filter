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

#include <functional> // for std::hash, std::equal_to
#include <memory>     // for std::unique_ptr
#include <vector>     // for std::vector
#include <cstddef>    // for std::size_t

namespace quofil {

// fp stands for fingerprint
class quotient_filter_fp {
public:
  using value_type = unsigned long long;

public:
  quotient_filter_fp(std::size_t q, std::size_t r);
  bool contains(std::size_t fp) const noexcept;
  bool insert(std::size_t fp);
  std::size_t size() const noexcept { return num_elements; }

private:
  value_type get_remainder(std::size_t pos) const noexcept;
  void set_remainder(std::size_t pos, value_type value) noexcept;

  std::size_t next_pos(std::size_t pos) const noexcept;
  std::size_t prev_pos(std::size_t pos) const noexcept;

  value_type quotient_part(value_type fp) const noexcept;
  value_type remainder_part(value_type fp) const noexcept;

  std::size_t find_run(value_type fp) const noexcept;

private:
  std::size_t q_bits;
  std::size_t r_bits;
  std::size_t num_slots;
  std::size_t num_elements;
  value_type q_mask;
  value_type r_mask;
  std::vector<bool> flags;
  std::unique_ptr<value_type[]> data;
};

template <class Key, class Hash = std::hash<Key>>
class quotient_filter {
public:
  void insert(const Key &elem);
  void erase(const Key &elem);
  bool contains(const Key &elem) const;
  double error_probability() const;
};

} // end namespace quofil

#endif // Header guard
