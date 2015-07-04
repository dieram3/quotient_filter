//          Copyright Diego Ramírez June 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/// \file
/// \brief Defines the quotient_filter class.

#ifndef QUOFIL_QUOTIENT_FILTER_HPP
#define QUOFIL_QUOTIENT_FILTER_HPP

#include <quofil/quotient_filter_fp.hpp> // for quofil::quotient_filter_fp

#include <algorithm>        // for std::{equal, min, max, for_each}
#include <functional>       // for std::hash
#include <initializer_list> // for std::initializer_list
#include <limits>           // for std::numeric_limits
#include <utility>          // for std::{pair, move, swap}
#include <cassert>          // for assert
#include <cmath>            // for std::ceil
#include <cstddef>          // for std::size_t

namespace quofil {

template <typename Key, typename Hash = std::hash<Key>,
          std::size_t Bits = std::numeric_limits<std::size_t>::digits>
class quotient_filter {

public:
  static constexpr std::size_t fp_bits = Bits;

public:
  using key_type = Key;
  using value_type = Key;
  using hasher = Hash;
  using size_type = quotient_filter_fp::size_type;
  using reference = value_type &;
  using const_reference = const value_type &;
  using iterator = quotient_filter_fp::iterator;
  using const_iterator = quotient_filter_fp::const_iterator;

public:
  // Constructors

  quotient_filter() noexcept {}

  explicit quotient_filter(size_type min_cap, const Hash &hash = Hash())
      : hash_fn(hash) {
    reserve(min_cap);
  }

  template <typename InputIt>
  quotient_filter(InputIt first, InputIt last, size_type min_cap = 0,
                  const Hash &hash = Hash())
      : quotient_filter(min_cap, hash) {
    insert(first, last);
  }

  quotient_filter(std::initializer_list<value_type> init, size_type min_cap = 0,
                  const Hash &hash = Hash())
      : quotient_filter(init.begin(), init.end(), min_cap, hash) {}

  // Iterators
  const_iterator begin() const noexcept { return filter.begin(); }

  const_iterator end() const noexcept { return filter.end(); }

  // Capacity
  bool empty() const noexcept { return filter.empty(); }

  size_type size() const noexcept { return filter.size(); }

  size_type max_size() const noexcept { return size_type{1} << (fp_bits - 1); }

  /// \brief Returns the number of elements that can be held in currently
  /// allocated storage according to the maximum load factor.
  size_type capacity() const noexcept {
    const float ans = max_load_factor() * slot_count();
    return std::min(static_cast<size_type>(ans), slot_count());
  }

  /// \brief Returns the number of slots in the Quotient-Filter.
  size_type slot_count() const noexcept { return filter.capacity(); }

  // Modifiers.
  void clear() noexcept { filter.clear(); }

  std::pair<iterator, bool> insert(const value_type &elem);

  template <typename InputIt>
  void insert(InputIt first, InputIt last) {
    std::for_each(first, last, [this](const_reference elem) { insert(elem); });
  }

  void insert(std::initializer_list<value_type> ilist) {
    insert(ilist.begin(), ilist.end());
  }

  template <typename... Args>
  std::pair<iterator, bool> emplace(Args &&... args) {
    value_type temp(std::forward<Args>(args)...);
    return insert(temp);
  }

  void erase(const_iterator pos) noexcept { filter.erase(pos); }

  size_type erase(const key_type &key) noexcept {
    return filter.erase(hash_fn(key));
  }

  void swap(quotient_filter &other) { std::swap(*this, other); }

  // Lookup
  size_type count(const key_type &key) const noexcept {
    return filter.count(hash_fn(key));
  }

  const_iterator find(const key_type &key) const noexcept {
    return filter.find(hash_fn(key));
  }

  // Hash policy
  float load_factor() const noexcept {
    return empty() ? 1.0f : float(size()) / float(slot_count());
  }

  float max_load_factor() const noexcept { return max_load_factor_; }

  void max_load_factor(float ml) noexcept;

  void reserve(size_type count);

  // Observers
  hasher hash_function() const { return hash_fn; }

  // Non-member functions.

  friend bool operator==(const quotient_filter &lhs,
                         const quotient_filter &rhs) noexcept {
    return lhs.size() == rhs.size() &&
           std::equal(lhs.begin(), lhs.end(), rhs.begin());
  }

  friend bool operator!=(const quotient_filter &lhs,
                         const quotient_filter &rhs) noexcept {
    return !(lhs == rhs);
  }

  friend void swap(quotient_filter &lhs, quotient_filter &rhs) noexcept {
    lhs.swap(rhs);
  }

private:
  constexpr static size_type calc_required_q(size_type cap) noexcept {
    size_t q_bits = 0;
    while ((size_type{1} << q_bits) < cap)
      ++q_bits;
    return q_bits;
  }

  static_assert(fp_bits != 0, "The fingerprint must have at least one bit");

private:
  quotient_filter_fp filter{};
  Hash hash_fn{};
  float max_load_factor_{0.8f};
};

template <typename Key, typename Hash, std::size_t Bits>
void quotient_filter<Key, Hash, Bits>::max_load_factor(float ml) noexcept {
  assert(size() <= capacity());

  max_load_factor_ = std::min(std::max(ml, 0.01f), 1.0f);

  if (size() > capacity()) {
    reserve(0);
    assert(size() <= capacity());
  }
}

template <typename Key, typename Hash, std::size_t Bits>
auto quotient_filter<Key, Hash, Bits>::insert(const value_type &elem)
    -> std::pair<iterator, bool> {
  assert(size() <= capacity());
  const auto fingerprint = hash_fn(elem);

  if (size() == capacity()) {
    auto it = filter.find(fingerprint);
    if (it != end())
      return std::make_pair(it, false);
    reserve(size() + 1);
    assert(size() < capacity());
  }

  return filter.insert(fingerprint);
}

template <typename Key, typename Hash, std::size_t Bits>
void quotient_filter<Key, Hash, Bits>::reserve(size_type count) {
  count = std::max(count, size());

  if (!count) {
    filter = quotient_filter_fp();
    assert(capacity() == 0);
    return;
  }

  const auto min_valid_capacity =
      static_cast<size_type>(std::ceil(count / max_load_factor_));

  const size_type q_bits = calc_required_q(min_valid_capacity);
  const size_type r_bits = fp_bits - q_bits;

  if (q_bits == filter.quotient_bits() && r_bits == filter.remainder_bits()) {
    // Note that remainder_bits() is not always fp - quotient_bits(). If filter
    // was default constructed both of them are zero.
    return; // No resize is necessary.
  }

  if (r_bits == 0)
    throw std::length_error(
        "The number of bits of fingerprints used by the "
        "Quotient-Filter is not enough to hold the required capacity.");

  quotient_filter_fp temp(q_bits, r_bits);

  // The reallocation has sense.
  assert(temp.capacity() != filter.capacity());

  for (const auto fingerprint : filter)
    temp.insert(fingerprint);

  // Everything is ok.
  assert(temp.size() == filter.size());

  filter = std::move(temp);

  // The new capacity meets the requirements.
  assert(count <= capacity());
}

} // End namespace quofil

#endif // Header guard
