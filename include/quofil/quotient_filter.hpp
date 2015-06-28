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

#include <exception>  // for std::exception
#include <functional> // for std::hash
#include <memory>     // for std::unique_ptr
#include <vector>     // for std::vector
#include <cstddef>    // for std::size_t

namespace quofil {

struct filter_is_full : public std::exception {
public:
  const char *what() const noexcept override;
};

// fp stands for fingerprint
class quotient_filter_fp {
public:
  using value_type = unsigned long long;

public:
  /// \brief Constructs a quotient filter using the given bits requirements.
  ///
  /// The constructed filter will use approximately <tt>(r + 3) * pow(2, q)</tt>
  /// bits of memory. Note that all used fingerprint will be truncated to its
  /// <tt>r + q</tt> least significant bits. If the truncation does not affect
  /// to any fingerprint the quotient filter will not give false positives.
  ///
  /// \param q The number of bits for the quotient.
  /// \param r The number of bits for the remainder.
  ///
  /// \pre \p r shall be greater than 0.
  ///
  quotient_filter_fp(std::size_t q, std::size_t r);

  /// \brief Checks if the given fingerprint is contained into the filter.
  bool contains(std::size_t fp) const noexcept;

  /// \brief Inserts the given fingerprint into the filter.
  ///
  /// \param fp The fingerprint to be inserted.
  ///
  /// \returns \c false if the fingerprint was already contained into the
  /// filter, otherwise \c true
  ///
  /// \throws filter_is_full if \c *this is full.
  bool insert(std::size_t fp);

  /// \brief Returns the number of elements in the quotient filter.
  std::size_t size() const noexcept { return num_elements; }

  /// \brief Checks if the quotient filter is empty.
  bool empty() const noexcept { return num_elements == 0; }

  /// \brief Checks if the quotient filter is full i.e
  /// <tt>size() == slots()</tt>
  bool full() const noexcept { return num_elements == num_slots; }

  /// \brief Returns the maximum number of elements which the quotient filter
  /// can hold.
  std::size_t slots() const noexcept { return num_slots; }

  /// \brief Returns the number of bits used for the quotient.
  std::size_t quotient_bits() const noexcept { return q_bits; }

  /// \brief Returns the number of bits used for the remainder.
  std::size_t remainder_bits() const noexcept { return r_bits; }

private:
  value_type get_remainder(std::size_t pos) const noexcept;
  void set_remainder(std::size_t pos, value_type value) noexcept;
  value_type exchange_remainder(std::size_t pos, value_type new_value) noexcept;

  std::size_t next_pos(std::size_t pos) const noexcept;
  std::size_t prev_pos(std::size_t pos) const noexcept;

  value_type quotient_part(value_type fp) const noexcept;
  value_type remainder_part(value_type fp) const noexcept;

  std::size_t find_run(value_type fp) const noexcept;

  void insert_into(std::size_t pos, bool is_head,
                   value_type fp_remainder) noexcept;

private:
  std::size_t q_bits;
  std::size_t r_bits;
  std::size_t num_slots;
  std::size_t num_elements;
  value_type q_mask;
  value_type r_mask;
  std::vector<bool> flags; // TODO:Benchmark on separated vector bool for flags.
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
