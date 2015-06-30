//          Copyright Diego Ramírez July 2015
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
/// \file
/// \brief Defines the class quotient_filter

#ifndef QUOFIL_QUOTIENT_FILTER_HPP
#define QUOFIL_QUOTIENT_FILTER_HPP

#include <quofil/quotient_filter_fp.hpp> // for quofil::quotient_filter_hp
#include <algorithm>                     // for std::equal, std::max
#include <functional>                    // for std::hash
#include <initializer_list>              // for std::initializer_list
#include <limits>                        // for std::numeric_limits
#include <utility>                       // for std::pair, std::move
#include <cassert>                       // for assert
#include <cmath>                         // for std::ceil

namespace quofil {

template <class Key, class Hash = std::hash<Key>,
          std::size_t Bits = std::numeric_limits<std::size_t>::digits>
class quotient_filter {
public:
  static constexpr std::size_t fp_bits = Bits;

public:
  using key_type = Key;
  using value_type = Key;
  using hasher = Hash;
  using size_type = quotient_filter_fp::size_type;
  using iterator = quotient_filter_fp::iterator;
  using const_iterator = quotient_filter_fp::const_iterator;

public:
  // Constructors

  explicit quotient_filter(size_type cap, const Hash &hash = Hash())
      : quotient_filter(hash, calc_required_q(cap)) {}

  quotient_filter() : quotient_filter(default_cap()) {}

  template <class InputIt>
  quotient_filter(InputIt first, InputIt last, size_type cap = default_cap(),
                  const Hash &hash = Hash())
      : quotient_filter(cap, hash) {
    insert(first, last);
  }

  // Iterators
  const_iterator begin() const noexcept { return filter.begin(); }

  const_iterator end() const noexcept { return filter.end(); }

  // Capacity
  bool empty() const noexcept { return filter.empty(); }

  size_type size() const noexcept { return filter.size(); }

  size_type max_size() const noexcept {
    return std::numeric_limits<size_type>::max();
  }

  size_type capacity() const noexcept { return filter.capacity(); }

  // Modifiers.
  void clear() noexcept { filter.clear(); }

  std::pair<iterator, bool> insert(const value_type &elem);

  template <class InputIt>
  void insert(InputIt first, InputIt last) {
    for (; first != last; ++first)
      insert(*first);
  }

  void insert(std::initializer_list<value_type> ilist) {
    insert(ilist.begin(), ilist.end());
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args &&... args) {
    value_type temp(std::forward<Args>(args)...);
    return insert(temp);
  }

  void erase(const_iterator pos) noexcept { filter.erase(pos); }

  size_type erase(const key_type &key) noexcept {
    return filter.erase(hash_fn(key));
  }

  void swap(quotient_filter_fp &other) noexcept { std::swap(*this, other); }

  // Lookup
  size_type count(const key_type &key) const noexcept {
    return filter.count(key);
  }

  const_iterator find(const key_type &key) const noexcept {
    return filter.find(key);
  }

  // Hash policy
  float load_factor() const noexcept {
    return float(size()) / float(capacity());
  }

  float max_load_factor() const noexcept { return max_load_factor_; }

  void max_load_factor(float ml) noexcept {
    max_load_factor_ = std::min(std::max(ml, 0.05f), 1.0f);
  }

  void reserve(size_type cap);

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

  friend void swap(const quotient_filter &lhs,
                   const quotient_filter &rhs) noexcept {
    lhs.swap(rhs);
  }

private:
  // Default load factor is 0.8
  explicit quotient_filter(const Hash &hash, size_type q_bits)
      : filter(q_bits, fp_bits - q_bits), hash_fn(hash),
        max_load_factor_{0.8f} {}

  constexpr static size_type calc_required_q(size_type cap) noexcept {
    size_t q_bits = 0;
    while ((size_type{1} << q_bits) < cap)
      ++q_bits;
    return q_bits;
  }

  constexpr static size_type default_cap() noexcept { return 16; }

private:
  quotient_filter_fp filter;
  Hash hash_fn;
  float max_load_factor_;
};

template <class Key, class Hash, std::size_t Bits>
auto quotient_filter<Key, Hash, Bits>::insert(const value_type &elem)
    -> std::pair<iterator, bool> {
  const std::size_t fp = hash_fn(elem);
  const bool will_exceed_ml = (size() + 1) > max_load_factor() * capacity();

  if (will_exceed_ml || filter.full()) {
    auto it = filter.find(fp);
    if (it != end())
      return std::make_pair(it, false);

    // Capacity might be zero.
    reserve(std::max(2 * capacity(), size_type(1)));
  }

  return filter.insert(fp);
}

template <class Key, class Hash, std::size_t Bits>
void quotient_filter<Key, Hash, Bits>::reserve(size_type cap) {
  cap = std::max(size(), cap);

  const size_type q_bits = calc_required_q(cap);
  const size_type r_bits = fp_bits - q_bits;

  if (r_bits == 0)
    throw std::length_error(
        "The number of bits of fingerpints used by the "
        "Quotient-Filter is not enough to hold the required capacity.");

  quotient_filter_fp temp(q_bits, r_bits);
  for (const auto fingerprint : filter)
    temp.insert(fingerprint);
  filter = std::move(temp);
}

} // End namespace quofil

#endif // Header guard
