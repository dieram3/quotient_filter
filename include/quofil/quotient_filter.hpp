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
  static constexpr std::size_t hash_bits = Bits;

public:
  using key_type = Key;
  using value_type = Key;
  using reference = value_type &;
  using const_reference = const value_type &;

  using iterator = quotient_filter_fp::iterator;
  using const_iterator = quotient_filter_fp::const_iterator;

  using size_type = quotient_filter_fp::size_type;
  using difference_type = iterator::difference_type;

  using hasher = Hash;

public:
  /// \brief Constructs an empty filter.
  ///
  /// Sets the <tt>max_load_factor()</tt> to an implementation defined value.
  ///
  /// \param slot_count The minimal number of slots to be allocated.
  /// \param hash The hash function to be used.
  ///
  explicit quotient_filter(size_type slot_count = 0, const Hash &hash = Hash())
      : hash_fn(hash) {
    regenerate(slot_count);
  }

  /// \brief Constructs the filter with the contents of the given range.
  ///
  /// Sets the <tt>max_load_factor()</tt> to an implementation defined value.
  ///
  /// \param first Beginning of the input range.
  /// \param last End of the the input range.
  /// \param slot_count The minimal number of slots to be allocated.
  /// \param hash The hash function to be used.
  ///
  template <typename InputIt>
  quotient_filter(InputIt first, InputIt last, size_type slot_count = 0,
                  const Hash &hash = Hash())
      : quotient_filter(slot_count, hash) {
    insert(first, last);
  }

  /// \brief Constructs the filter with the contents of an initializer list.
  ///
  /// Sets the <tt>max_load_factor()</tt> to an implementation defined value.
  ///
  /// \param init The initializer list to initialize the contents.
  /// \param slot_count The minimal number of slots to be allocated.
  /// \param hash The hash function to be used.
  ///
  quotient_filter(std::initializer_list<value_type> init,
                  size_type slot_count = 0, const Hash &hash = Hash())
      : quotient_filter(init.begin(), init.end(), slot_count, hash) {}

  /// \brief Returns an iterator to the first element of the filter.
  ///
  /// If the filter is empty, the returned iterator will be equal to
  /// <tt>end()</tt>.
  const_iterator begin() const noexcept { return filter.begin(); }

  /// \brief Returns an iterator to the one-past end element of the filter.
  ///
  /// \note Increment or dereference the <tt>end()</tt> iterator is undefined
  /// behaviour.
  const_iterator end() const noexcept { return filter.end(); }

  /// \brief Checks whether the filter is empty.
  bool empty() const noexcept { return filter.empty(); }

  /// \brief Returns the number of elements (hash values) contained in the
  /// filter.
  size_type size() const noexcept { return filter.size(); }

  /// \brief Returns the maximum possible number of elements according to
  /// <tt>hash_bits</tt>.
  size_type max_size() const noexcept {
    return size_type{1} << (hash_bits - 1);
  }

  /// \brief Returns current number of allocated slots.
  ///
  /// \note The slot count is always a power of two.
  ///
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
    return empty() ? 0.0f : float(size()) / float(slot_count());
  }

  float max_load_factor() const noexcept { return max_load_factor_; }

  void max_load_factor(float ml) noexcept;

  /// \brief Sets the <tt>slot_count()</tt> to the minimal valid value greater
  /// than or equal to the given value.
  ///
  /// The minimal valid value is a power of two which depends on the current
  /// number of stored elements (hash values) and the current
  /// <tt>max_load_factor()</tt>.
  ///
  /// If <tt>slot_count()</tt> has changed, the filter will be regenerated to
  /// use the required storage.
  ///
  /// \param slot_count The minimal number of slots to be used.
  ///
  void regenerate(size_type slot_count);

  /// \brief Reserves space for at least the specified number of elements.
  ///
  /// Sets the number of slots to the minimal value needed for holding at least
  /// \p count elements.
  ///
  /// \param count New capacity of the filter.
  ///
  void reserve(size_type count) {
    assert(size() <= max_allowed_size() && "The filter is corrupted");
    const float min_slot_count = std::ceil(count / max_load_factor_);
    regenerate(static_cast<size_type>(min_slot_count));
  }

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
  // Returns the minimal q_bits such that at least slot_count slots are
  // available.
  constexpr static size_type calc_required_q(size_type slot_count) noexcept {
    size_t q_bits = 0;
    while ((size_type{1} << q_bits) < slot_count)
      ++q_bits;
    return q_bits;
  }

  // Returns the current max allowed size according to the number of allocated
  // slots and the maximum load factor.
  size_type max_allowed_size() const noexcept {
    const float ans = max_load_factor() * slot_count();
    return std::min(static_cast<size_type>(ans), slot_count());
  }

  static_assert(hash_bits != 0,
                "The generated hashes must have at least one bit");

private:
  quotient_filter_fp filter{};
  Hash hash_fn{};
  float max_load_factor_{0.75f};
};

template <typename Key, typename Hash, std::size_t Bits>
void quotient_filter<Key, Hash, Bits>::max_load_factor(float ml) noexcept {
  assert(size() <= max_allowed_size());

  max_load_factor_ = std::min(std::max(ml, 0.01f), 1.0f);

  if (size() > max_allowed_size()) {
    regenerate(0);
    assert(size() <= max_allowed_size());
  }
}

template <typename Key, typename Hash, std::size_t Bits>
auto quotient_filter<Key, Hash, Bits>::insert(const value_type &elem)
    -> std::pair<iterator, bool> {
  assert(size() <= max_allowed_size() && "The filter is corrupted");
  const auto hash_value = hash_fn(elem);

  if (size() == max_allowed_size()) {
    auto it = filter.find(hash_value);
    if (it != end())
      return std::make_pair(it, false);
    reserve(size() + 1);
    assert(size() < max_allowed_size() && "Reserve is not working");
  }

  return filter.insert(hash_value);
}

template <typename Key, typename Hash, std::size_t Bits>
void quotient_filter<Key, Hash, Bits>::regenerate(size_type count) {

  const auto min_slot_count =
      static_cast<size_type>(std::ceil(size() / max_load_factor()));

  const auto new_slot_count = std::max(min_slot_count, count);

  if (!new_slot_count) {
    filter = quotient_filter_fp();
    assert(max_allowed_size() == 0);
    return;
  }

  const size_type q_bits = calc_required_q(new_slot_count);
  const size_type r_bits = hash_bits - q_bits;

  if (q_bits == filter.quotient_bits() && r_bits == filter.remainder_bits()) {
    // Note that remainder_bits() is not always fp - quotient_bits(). If filter
    // was default constructed both of them are zero.
    return; // No regeneration is necessary.
  }

  if (r_bits == 0)
    throw std::length_error("The number of bits of elements (hash values) "
                            "contained in the filter is not enough to hold the "
                            "required slot count.");

  quotient_filter_fp temp(q_bits, r_bits);

  assert(temp.capacity() != filter.capacity() &&
         "Regeneration should not have been required");

  for (const auto hash_value : filter)
    temp.insert(hash_value);

  assert(temp.size() == filter.size()); // Everything is ok.
  filter = std::move(temp);
  assert(count <= slot_count()); // Meets the requirements.
}

} // End namespace quofil

#endif // Header guard
