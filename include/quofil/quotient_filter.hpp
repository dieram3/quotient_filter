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
#include <iterator>   // for std::forward_iterator_tag
#include <functional> // for std::hash
#include <memory>     // for std::unique_ptr
#include <utility>    // for std::pair
#include <vector>     // for std::vector
#include <cassert>    // for assert
#include <cstddef>    // for std::size_t, std::ptrdiff_t

namespace quofil {

struct filter_is_full : public std::exception {
public:
  const char *what() const noexcept override;
};

/// \brief Quotient-Filter implementation class.
class quotient_filter_fp {
public:
  using value_type = unsigned long long;
  class iterator;
  using const_iterator = iterator;
  friend class iterator;

public:
  /// \brief Constructs a quotient filter using the given bits requirements.
  ///
  /// The constructed filter will use approximately <tt>(r + 3) * pow(2, q)</tt>
  /// bits of memory. Note that all used fingerprints will be truncated to its
  /// <tt>r + q</tt> least significant bits. If the truncation does not affect
  /// to any fingerprint the quotient filter will not give false positives.
  ///
  /// \param q The number of bits for the quotient.
  /// \param r The number of bits for the remainder.
  ///
  /// \pre \p r shall positive.
  ///
  quotient_filter_fp(std::size_t q, std::size_t r);

  /// \brief Searchs for a given fingerprint.
  /// \param fp The fingerprint to be searched.
  /// \returns Iterator to the slot that contains the fingerprint. If no such
  /// fingerprint was found, it returns <tt>end()</tt>.
  iterator find(value_type fp) const noexcept;

  /// \brief Counts how many times a fingerprint is contained into the filter.
  ///
  /// Effectively returns 0 or 1.
  std::size_t count(value_type fp) const noexcept;

  /// \brief Inserts the given fingerprint into the filter.
  ///
  /// If the insertion took place, all iterators become invalidate.
  ///
  /// \param fp The fingerprint to be inserted.
  ///
  /// \returns A pair consisting of an iterator to the inserted element (or to
  /// the element that prevented the insertion) and a bool denoting whether the
  /// insertion took place.
  ///
  /// \throws filter_is_full if \c *this is full.
  std::pair<iterator, bool> insert(value_type fp);

  void erase(iterator pos) noexcept;
  std::size_t erase(value_type fp) noexcept;

  /// \brief Returns the number of elements in the quotient filter.
  std::size_t size() const noexcept { return num_elements; }

  /// \brief Checks whether the quotient filter is empty.
  bool empty() const noexcept { return num_elements == 0; }

  /// \brief Checks whether the quotient filter is full i.e
  /// <tt>size() == slots()</tt>
  bool full() const noexcept { return size() == capacity(); }

  /// \brief Returns the maximum number of elements which the quotient filter
  /// can hold.
  std::size_t capacity() const noexcept { return num_slots; }

  /// \brief Returns the number of bits used for the quotient.
  std::size_t quotient_bits() const noexcept { return q_bits; }

  /// \brief Returns the number of bits used for the remainder.
  std::size_t remainder_bits() const noexcept { return r_bits; }

  /// \brief Returns an iterator to the beginning of the filter.
  const_iterator begin() const noexcept;

  /// \brief Returns an iterator to the end of the filter.
  const_iterator end() const noexcept;

private:
  value_type get_remainder(std::size_t) const noexcept;
  void set_remainder(std::size_t, value_type) noexcept;
  value_type exchange_remainder(std::size_t, value_type) noexcept;

  std::size_t incr_pos(std::size_t) const noexcept;
  std::size_t decr_pos(std::size_t) const noexcept;

  value_type extract_quotient(value_type) const noexcept;
  value_type extract_remainder(value_type) const noexcept;

  std::size_t find_next_occupied(std::size_t) const noexcept;
  std::size_t find_next_run(std::size_t) const noexcept;
  std::size_t find_run_of(value_type) const noexcept;

  void insert_into(std::size_t, value_type, bool) noexcept;
  void remove_entry(std::size_t, std::size_t) noexcept;

  bool is_empty_slot(std::size_t) const noexcept;
  bool is_run_start(std::size_t) const noexcept;

private:
  std::size_t q_bits;
  std::size_t r_bits;
  std::size_t num_slots;
  std::size_t num_elements;
  value_type q_mask;
  value_type r_mask;
  // std::vector<bool> flags; // TODO:Benchmark on separated vector bool for
  // flags.
  std::vector<bool> is_occupied;
  std::vector<bool> is_continuation;
  std::vector<bool> is_shifted;
  std::unique_ptr<value_type[]> data;
};

/// \brief Iterator to navigate through the elements of a quotient filter.

class quotient_filter_fp::iterator {
  friend class quotient_filter_fp;

public:
  using value_type = quotient_filter_fp::value_type;
  using difference_type = std::ptrdiff_t;
  using pointer = const value_type *;
  using reference = value_type;
  using iterator_category = std::forward_iterator_tag;

public:
  iterator() = default;

  void operator++() { increment(); }

  iterator operator++(int) {
    auto old_iter = *this;
    increment();
    return old_iter;
  }

  reference operator*() const { return dereference(); }

  friend bool operator==(const iterator &lhs, const iterator &rhs) noexcept {
    return lhs.equal(rhs);
  }

  friend bool operator!=(const iterator &lhs, const iterator &rhs) noexcept {
    return !lhs.equal(rhs);
  }

private:
  explicit iterator(const quotient_filter_fp *) noexcept;
  iterator(const quotient_filter_fp *, size_t, value_type) noexcept;

  void increment() noexcept;

  bool equal(const iterator &that) const noexcept {
    return pos == that.pos && filter == that.filter;
  }

  reference dereference() const noexcept {
    return (run_quotient << filter->r_bits) | filter->get_remainder(pos);
  }

private:
  const quotient_filter_fp *filter = nullptr;
  size_t pos = 0;              // current position.
  value_type run_quotient = 0; // quotient of current run.
};

inline quotient_filter_fp::iterator quotient_filter_fp::begin() const noexcept {
  return iterator(this);
}
inline quotient_filter_fp::iterator quotient_filter_fp::end() const noexcept {
  return iterator();
}
inline std::size_t quotient_filter_fp::count(value_type fp) const noexcept {
  return find(fp) != end();
}
inline void quotient_filter_fp::erase(iterator slot_iterator) noexcept {
  assert(slot_iterator.filter == this);
  remove_entry(slot_iterator.pos,
               static_cast<size_t>(slot_iterator.run_quotient));
  --num_elements;
}
inline std::size_t quotient_filter_fp::erase(value_type fp) noexcept {
  const auto it = find(fp);
  if (it == end())
    return 0;
  erase(it);
  return 1;
}

} // end namespace quofil

#endif // Header guard
