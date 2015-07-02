//          Copyright Diego Ramírez July 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/// \file
/// \brief Defines the quotient_filter_fp class.

#ifndef QUOFIL_QUOTIENT_FILTER_FP_HPP
#define QUOFIL_QUOTIENT_FILTER_FP_HPP

#include <exception> // for std::exception
#include <iterator>  // for std::forward_iterator_tag
#include <utility>   // for std::pair
#include <vector>    // for std::vector
#include <cassert>   // for assert
#include <cstddef>   // for std::size_t, std::ptrdiff_t

namespace quofil {

/// \brief Exception thrown when an insertion on a full filter is attempted.
struct filter_is_full : public std::exception {
public:
  const char *what() const noexcept override;
};

/// \brief Quotient-Filter implementation class.
class quotient_filter_fp {
public:
  using value_type = std::size_t; // std::hash evaluates to std::size_t
  using size_type = std::size_t;
  class iterator;
  using const_iterator = iterator;
  friend class iterator;

public:
  /// \brief Constructs a quotient filter with zero capacity.
  quotient_filter_fp() = default;

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
  quotient_filter_fp(size_type q, size_type r);

  /// \brief Searchs for a given fingerprint.
  /// \param fp The fingerprint to be searched.
  /// \returns Iterator to the slot that contains the fingerprint. If no such
  /// fingerprint was found, it returns <tt>end()</tt>.
  const_iterator find(value_type fp) const noexcept;

  /// \brief Counts how many times a fingerprint is contained into the filter.
  ///
  /// Effectively returns 0 or 1.
  size_type count(value_type fp) const noexcept;

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
  ///
  std::pair<iterator, bool> insert(value_type fp);

  /// \brief Erases the given element.
  ///
  /// Invalidates all iterators.
  ///
  void erase(const_iterator pos) noexcept;

  /// \brief Erases the given fingerprint if it exists.
  ///
  /// If the fingerprint was found, all iterators are invalidated.
  ///
  /// \param fp The fingerprint to be erased.
  ///
  /// \returns The number of erased elements, effectively 0 or 1.
  size_type erase(value_type fp) noexcept;

  /// \brief Clears the contents.
  void clear() noexcept;

  /// \brief Returns the number of elements in the quotient filter.
  size_type size() const noexcept { return num_elements; }

  /// \brief Checks whether the quotient filter is empty.
  bool empty() const noexcept { return num_elements == 0; }

  /// \brief Checks whether the quotient filter is full i.e
  /// <tt>size() == capacity()</tt>
  bool full() const noexcept { return size() == capacity(); }

  /// \brief Returns the maximum number of elements which the quotient filter
  /// can hold.
  size_type capacity() const noexcept { return num_slots; }

  /// \brief Returns the number of bits used for the quotient.
  size_type quotient_bits() const noexcept { return q_bits; }

  /// \brief Returns the number of bits used for the remainder.
  size_type remainder_bits() const noexcept { return r_bits; }

  /// \brief Returns an iterator to the beginning of the filter.
  const_iterator begin() const noexcept;

  /// \brief Returns an iterator to the end of the filter.
  /// \note The end iterator is never invalidated.
  const_iterator end() const noexcept;

private:
  value_type get_remainder(size_type) const noexcept;
  void set_remainder(size_type, value_type) noexcept;
  value_type exchange_remainder(size_type, value_type) noexcept;

  size_type incr_pos(size_type) const noexcept;
  size_type decr_pos(size_type) const noexcept;

  value_type extract_quotient(value_type) const noexcept;
  value_type extract_remainder(value_type) const noexcept;

  size_type find_next_occupied(size_type) const noexcept;
  size_type find_next_run_quotient(size_type) const noexcept;
  size_type find_run_start(size_type) const noexcept;

  void insert_into(size_type, value_type, bool) noexcept;
  void remove_entry(size_type, size_type) noexcept;

  bool is_empty_slot(size_type) const noexcept;

private:
  size_type q_bits = 0;
  size_type r_bits = 0;
  size_type num_slots = 0;
  size_type num_elements = 0;
  value_type quotient_mask = 0;
  value_type remainder_mask = 0;
  std::vector<bool> is_occupied;
  std::vector<bool> is_continuation;
  std::vector<bool> is_shifted;
  std::vector<value_type> data;
};

/// \brief Iterator to navigate through the elements of a quotient filter.
class quotient_filter_fp::iterator {
  friend class quotient_filter_fp;
  using size_type = quotient_filter_fp::size_type;

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
  iterator(const quotient_filter_fp *, size_type, size_type) noexcept;

  void increment() noexcept;

  bool equal(const iterator &that) const noexcept {
    return pos == that.pos && filter == that.filter;
  }

  reference dereference() const noexcept {
    const auto quotient = static_cast<value_type>(canonical_pos);
    return (quotient << filter->r_bits) | filter->get_remainder(pos);
  }

private:
  const quotient_filter_fp *filter = nullptr;
  size_type pos = 0;           // Current position.
  size_type canonical_pos = 0; // Where the remainder should be.
};

inline auto quotient_filter_fp::begin() const noexcept -> iterator {
  return iterator(this);
}
inline auto quotient_filter_fp::end() const noexcept -> iterator {
  return iterator();
}
inline auto quotient_filter_fp::count(value_type fp) const
    noexcept -> size_type {
  return find(fp) != end();
}
inline void quotient_filter_fp::erase(const const_iterator it) noexcept {
  assert(it.filter == this);
  remove_entry(it.pos, it.canonical_pos);
  --num_elements;
}
inline auto quotient_filter_fp::erase(value_type fp) noexcept -> size_type {
  const auto it = find(fp);
  if (it == end())
    return 0;
  erase(it);
  return 1;
}

} // end namespace quofil

#endif // Header guard
