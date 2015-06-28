#include <quofil/quotient_filter.hpp>
#include <algorithm>   // for std::min
#include <limits>      // for std::numeric_limits
#include <memory>      // for std::make_unique
#include <type_traits> // for std::is_unsigned
#include <vector>      // for std::vector
#include <cassert>     // for assert
#include <cstddef>     // for std::size_t

// ==========================================
// General declarations.
// ==========================================

using qfilter = ::quofil::quotient_filter_fp;
using qf_iterator = qfilter::iterator;

using std::size_t;
using value_type = qfilter::value_type;
using block_type = value_type;

static_assert(std::is_unsigned<block_type>::value,
              "The block-type must be unsigned.");

static_assert(sizeof(block_type) >= sizeof(unsigned),
              "The block-type must have a capacity equal to or greater than "
              "'unsigned int'.");

static constexpr size_t bits_per_block =
    std::numeric_limits<block_type>::digits;

// ==========================================
// Miscellaneous auxiliary functions
// ==========================================

// Returns the ceil of x / y
static constexpr block_type ceil_div(block_type x, block_type y) noexcept {
  return x / y + block_type(x % y == 0 ? 0 : 1);
}

static constexpr block_type low_mask(std::size_t num_bits) noexcept {
  return ~(~static_cast<block_type>(0) << num_bits);
}

// ==========================================
// Flag auxiliary functions
// ==========================================

using bit_reference = std::vector<bool>::reference;

static bool exchange(bit_reference bit, bool new_value) noexcept {
  const bool old_value = bit;
  bit = new_value;
  return old_value;
}

bool qfilter::is_empty_slot(size_t pos) const noexcept {
  return !is_occupied[pos] && !is_continuation[pos] && !is_shifted[pos];
}

bool qfilter::is_run_start(size_t pos) const noexcept {
  return !is_continuation[pos] && (is_shifted[pos] || is_occupied[pos]);
}

// Returns true if the slot at pos is the beginning of a cluster.
// static constexpr bool is_cluster_start(const std::vector<bool> &flags,
//                                       size_t pos) {
//  return is_ocuppied(flags, pos) && !is_continuation(flags, pos) &&
//         !is_shifted(flags, pos);
//}

// ==========================================
// Data access functions
// ==========================================

value_type qfilter::get_remainder(const size_t pos) const noexcept {
  const size_t num_bit = r_bits * pos;
  const size_t block = num_bit / bits_per_block;
  const size_t offset = num_bit % bits_per_block;

  size_t pending_bits = r_bits;
  size_t bits_to_read = std::min(pending_bits, bits_per_block - offset);

  value_type ans = (data[block] >> offset) & low_mask(bits_to_read);
  pending_bits -= bits_to_read;
  if (pending_bits) {
    value_type next = data[block + 1] & low_mask(pending_bits);
    ans |= next << bits_to_read;
  }
  return ans;
}

// Requires: value < 2^r_bits
void qfilter::set_remainder(const size_t pos, const value_type value) noexcept {

  assert(value == (value & r_mask));

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

value_type qfilter::exchange_remainder(size_t pos,
                                       value_type new_value) noexcept {
  value_type old_value = get_remainder(pos);
  set_remainder(pos, new_value);
  return old_value;
}

// ==========================================
// Slot navigation
// ==========================================

size_t qfilter::next_pos(const size_t pos) const noexcept {
  return (pos + 1) & q_mask;
}

size_t qfilter::prev_pos(const size_t pos) const noexcept {
  return (pos - 1) & q_mask;
}

// ==========================================
// Parts of finger print
// ==========================================

value_type qfilter::quotient_part(value_type fp) const noexcept {
  return (fp >> r_bits) & q_mask;
}

value_type qfilter::remainder_part(value_type fp) const noexcept {
  return fp & r_mask;
}

// ==========================================
// Constructor
// ==========================================

qfilter::quotient_filter_fp(size_t q, size_t r)
    : q_bits{q}, r_bits{r}, num_slots{static_cast<size_t>(1) << q},
      num_elements{0}, q_mask{low_mask(q)}, r_mask{low_mask(r)},
      is_occupied(num_slots), is_continuation(num_slots), is_shifted(num_slots),
      data{} {
  assert(r != 0 && "The remainder must have at least one bit");
  const size_t required_bits = r_bits * num_slots;
  const size_t required_blocks = ceil_div(required_bits, bits_per_block);
  data = std::make_unique<block_type[]>(required_blocks);
}

// ==========================================
// Search
// ==========================================

// Find the position of the first slot of the run given in the argument.
// The run must exists.
size_t qfilter::find_run(const value_type quotient) const noexcept {
  size_t pos = static_cast<size_t>(quotient);
  assert(is_occupied[pos]);

  // If the run is in its canonical slot returns pos immediately.
  if (!is_shifted[pos])
    return pos;

  // Find the beginning of the cluster.
  size_t running_count = 0;
  do {
    pos = prev_pos(pos);
    running_count += is_occupied[pos];
  } while (is_shifted[pos]);

  // Find the beginning of the target run.
  for (; running_count; --running_count) {
    // Advance to the next run.
    do {
      pos = next_pos(pos);
    } while (is_continuation[pos]);
  }

  return pos;
}

qf_iterator qfilter::find(value_type fp) const noexcept {

  const auto fp_quotient = quotient_part(fp);
  const auto fp_remainder = remainder_part(fp);
  const size_t canonical_slot_pos = static_cast<size_t>(fp_quotient);

  // If the quotient has no run, fp can't exist.
  if (!is_occupied[canonical_slot_pos])
    return end();

  // Search on the sorted run for fp_remainder.
  size_t pos = find_run(fp_quotient);
  do {
    const auto remainder = get_remainder(pos);
    if (remainder == fp_remainder)
      return iterator{this, pos, fp_quotient};
    if (remainder > fp_remainder)
      return end();
    pos = next_pos(pos);
  } while (is_continuation[pos]);
  return end();
}

// ==========================================
// Insertion
// ==========================================

// Inserts the element into the required pos moving all element from pos until
// the first empty slot one position to the right. The inserted and the moved
// elements are marked as shifted. Note that the inserted element could actually
// not be shifted so it should be corrected outside.
void qfilter::insert_into(size_t pos, value_type remainder,
                          bool continuation) noexcept {

  bool found_empty_slot = false;

  do {
    found_empty_slot = is_empty_slot(pos);
    continuation = exchange(is_continuation[pos], continuation);
    remainder = exchange_remainder(pos, remainder);
    is_shifted[pos] = true;
    pos = next_pos(pos);
  } while (!found_empty_slot);
}

std::pair<qf_iterator, bool> qfilter::insert(value_type fp) {

  if (full())
    throw quofil::filter_is_full();

  const auto fp_quotient = quotient_part(fp);
  const auto fp_remainder = remainder_part(fp);
  const size_t canonical_slot_pos = static_cast<size_t>(fp_quotient);

  if (is_empty_slot(canonical_slot_pos)) {
    is_occupied[canonical_slot_pos] = true;
    set_remainder(canonical_slot_pos, fp_remainder);
    ++num_elements;
    return std::make_pair(iterator{this, canonical_slot_pos, fp_quotient},
                          true);
  }

  // Indicates that the run has no element. However the slot of the run start
  // can be ocuppied.
  const bool run_is_empty = !is_occupied[canonical_slot_pos];
  if (run_is_empty)
    is_occupied[canonical_slot_pos] = true;

  size_t pos = find_run(fp_quotient);
  const size_t run_start = pos;

  // Search the correct position.
  if (!run_is_empty) {
    do {
      const auto remainder = get_remainder(pos);
      if (remainder == fp_remainder)
        return std::make_pair(iterator{this, pos, fp_quotient}, false);
      if (remainder > fp_remainder)
        break;
      pos = next_pos(pos);
    } while (is_continuation[pos]);

    if (pos == run_start) {
      is_continuation[pos] = true;
    }
  }

  insert_into(pos, fp_remainder, pos != run_start);
  if (pos == canonical_slot_pos)
    is_shifted[pos] = false;

  ++num_elements;
  return std::make_pair(iterator{this, pos, fp_quotient}, true);
}

// ==========================================
// Iterator
// ==========================================

void qf_iterator::update_value() noexcept {
  value = (run_quotient << filter->r_bits) | filter->get_remainder(pos);
}

qf_iterator::iterator(const qfilter *the_filter) noexcept {
  assert(the_filter != nullptr);

  if (the_filter->empty())
    return;

  filter = the_filter;

  size_t quotient_pos = 0;
  while (!filter->is_occupied[quotient_pos])
    ++quotient_pos;

  run_quotient = static_cast<value_type>(quotient_pos);
  pos = filter->find_run(run_quotient);

  update_value();
}

qf_iterator::iterator(const qfilter *filter_, const size_t pos_,
                      value_type run_quotient_) noexcept
    : filter{filter_},
      pos{pos_},
      run_quotient{run_quotient_} {
  update_value();
}

void qf_iterator::increment() noexcept {
  assert(filter && "Can't increment end iterator");

  pos = filter->next_pos(pos);

  if (filter->is_continuation[pos]) {
    update_value();
    return;
  }

  size_t quotient_pos = static_cast<size_t>(run_quotient);
  do {
    ++quotient_pos;

    // End was reached.
    if (quotient_pos == filter->num_slots) {
      filter = nullptr;
      pos = 0;
      return;
    }

  } while (!filter->is_occupied[quotient_pos]);

  run_quotient = static_cast<value_type>(quotient_pos);

  while (!filter->is_run_start(pos)) {
    pos = filter->next_pos(pos);
  }

  update_value();
}
