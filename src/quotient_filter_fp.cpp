#include <quofil/quotient_filter_fp.hpp>
#include <algorithm>   // for std::min
#include <limits>      // for std::numeric_limits
#include <memory>      // for std::make_unique
#include <type_traits> // for std::is_unsigned
#include <cassert>     // for assert

// ==========================================
// General declarations.
// ==========================================

using qfilter = ::quofil::quotient_filter_fp;
using qf_iterator = qfilter::iterator;

using std::make_pair;

using size_type = qfilter::size_type;
using value_type = qfilter::value_type;
using block_type = value_type;

static_assert(std::is_unsigned<block_type>::value,
              "The block-type must be unsigned.");

static_assert(sizeof(block_type) >= sizeof(unsigned),
              "The block-type must have a capacity equal to or greater than "
              "'unsigned int'.");

static constexpr size_type bits_per_block =
    std::numeric_limits<block_type>::digits;

// ==========================================
// Miscellaneous auxiliary functions
// ==========================================

// Returns the ceil of x / y
static constexpr size_type ceil_div(size_type x, size_type y) noexcept {
  return x / y + size_type(x % y == 0 ? 0 : 1);
}

static constexpr block_type low_mask(size_type num_bits) noexcept {
  return ~(~static_cast<block_type>(0) << num_bits);
}

// ==========================================
// Flag functions
// ==========================================

using bit_reference = std::vector<bool>::reference;

static bool exchange(bit_reference bit, bool new_value) noexcept {
  const bool old_value = bit;
  bit = new_value;
  return old_value;
}

bool qfilter::is_empty_slot(size_type pos) const noexcept {
  return !is_occupied[pos] && !is_continuation[pos] && !is_shifted[pos];
}

// ==========================================
// Data access functions
// ==========================================

value_type qfilter::get_remainder(const size_type pos) const noexcept {
  const size_type num_bit = r_bits * pos;
  const size_type block = num_bit / bits_per_block;
  const size_type offset = num_bit % bits_per_block;

  size_type pending_bits = r_bits;
  size_type bits_to_read = std::min(pending_bits, bits_per_block - offset);

  value_type ans = (data[block] >> offset) & low_mask(bits_to_read);
  pending_bits -= bits_to_read;
  if (pending_bits) {
    value_type next = data[block + 1] & low_mask(pending_bits);
    ans |= next << bits_to_read;
  }
  return ans;
}

// Requires: value < 2^r_bits
void qfilter::set_remainder(const size_type pos,
                            const value_type value) noexcept {

  assert(value == (value & r_mask));

  const size_type num_bit = r_bits * pos;
  const size_type block = num_bit / bits_per_block;
  const size_type offset = num_bit % bits_per_block;

  size_type pending_bits = r_bits;
  size_type bits_to_write = std::min(pending_bits, bits_per_block - offset);

  data[block] &= ~(low_mask(bits_to_write) << offset);
  data[block] |= value << offset;

  pending_bits -= bits_to_write;
  if (pending_bits) {
    data[block + 1] &= ~low_mask(pending_bits);
    data[block + 1] |= value >> bits_to_write;
  }
}

value_type qfilter::exchange_remainder(size_type pos,
                                       value_type new_value) noexcept {
  value_type old_value = get_remainder(pos);
  set_remainder(pos, new_value);
  return old_value;
}

// ==========================================
// Slot navigation
// ==========================================

size_type qfilter::incr_pos(const size_type pos) const noexcept {
  return (pos + 1) & static_cast<size_type>(q_mask);
}

size_type qfilter::decr_pos(const size_type pos) const noexcept {
  return (pos - 1) & static_cast<size_type>(q_mask);
}

// ==========================================
// Parts of finger print
// ==========================================

value_type qfilter::extract_quotient(value_type fp) const noexcept {
  return (fp >> r_bits) & q_mask;
}

value_type qfilter::extract_remainder(value_type fp) const noexcept {
  return fp & r_mask;
}

// ==========================================
// Constructor
// ==========================================

qfilter::quotient_filter_fp(size_type q, size_type r)
    : q_bits{q}, r_bits{r}, num_slots{size_type(1) << q}, num_elements{0},
      q_mask{low_mask(q)}, r_mask{low_mask(r)}, is_occupied(num_slots),
      is_continuation(num_slots), is_shifted(num_slots), data{} {
  assert(r != 0 && "The remainder must have at least one bit");
  const size_type required_bits = r_bits * num_slots;
  const size_type required_blocks = ceil_div(required_bits, bits_per_block);
  data = std::make_unique<block_type[]>(required_blocks);
}

// ==========================================
// Search
// ==========================================

size_type qfilter::find_next_occupied(size_type pos) const noexcept {
  do
    pos = incr_pos(pos);
  while (!is_occupied[pos]);
  return pos;
}

size_type qfilter::find_next_run_quotient(size_type pos) const noexcept {
  assert(pos < num_slots);
  assert(is_occupied[pos]);
  do
    ++pos;
  while (pos != num_slots && !is_occupied[pos]);
  return pos;
}

// Find the position of the first slot of the run with the given canonical pos.
// The run must exists.
size_type qfilter::find_run_start(const size_type canonical_pos) const
    noexcept {
  assert(is_occupied[canonical_pos]);
  size_type pos = canonical_pos;

  // If the run is in its canonical slot returns pos immediately.
  if (!is_shifted[pos])
    return pos;

  do
    pos = decr_pos(pos);
  while (is_shifted[pos]);

  size_type quotient_pos = pos;
  while (quotient_pos != canonical_pos) {
    do
      pos = incr_pos(pos);
    while (is_continuation[pos]);

    quotient_pos = find_next_occupied(quotient_pos);
  }

  return pos;
}

qf_iterator qfilter::find(const value_type fp) const noexcept {

  const auto fp_quotient = extract_quotient(fp);
  const auto fp_remainder = extract_remainder(fp);
  const auto canonical_pos = static_cast<size_type>(fp_quotient);

  // If the quotient has no run, fp can't exist.
  if (!is_occupied[canonical_pos])
    return end();

  // Search on the sorted run for fp_remainder.
  size_type pos = find_run_start(canonical_pos);
  do {
    const auto remainder = get_remainder(pos);
    if (remainder == fp_remainder)
      return iterator{this, pos, canonical_pos};
    if (remainder > fp_remainder)
      return end();
    pos = incr_pos(pos);
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
void qfilter::insert_into(size_type pos, value_type remainder,
                          bool continuation) noexcept {

  bool found_empty_slot = false;

  do {
    found_empty_slot = is_empty_slot(pos);
    continuation = exchange(is_continuation[pos], continuation);
    remainder = exchange_remainder(pos, remainder);
    is_shifted[pos] = true;
    pos = incr_pos(pos);
  } while (!found_empty_slot);
}

std::pair<qf_iterator, bool> qfilter::insert(const value_type fp) {

  if (full())
    throw quofil::filter_is_full();

  const auto fp_quotient = extract_quotient(fp);
  const auto fp_remainder = extract_remainder(fp);
  const auto canonical_pos = static_cast<size_type>(fp_quotient);

  if (is_empty_slot(canonical_pos)) {
    is_occupied[canonical_pos] = true;
    set_remainder(canonical_pos, fp_remainder);
    ++num_elements;
    return make_pair(iterator{this, canonical_pos, canonical_pos}, true);
  }

  const bool run_was_empty = !exchange(is_occupied[canonical_pos], true);

  const size_type run_start = find_run_start(canonical_pos);
  size_type pos = run_start;

  // Search the correct position.
  if (!run_was_empty) {
    do {
      const auto remainder = get_remainder(pos);
      if (remainder == fp_remainder)
        return make_pair(iterator{this, pos, canonical_pos}, false);
      if (remainder > fp_remainder)
        break;
      pos = incr_pos(pos);
    } while (is_continuation[pos]);

    if (pos == run_start) {
      is_continuation[pos] = true;
    }
  }

  insert_into(pos, fp_remainder, pos != run_start);
  if (pos == canonical_pos)
    is_shifted[pos] = false;

  ++num_elements;
  return make_pair(iterator{this, pos, canonical_pos}, true);
}

// ==========================================
// Deletion
// ==========================================

void qfilter::remove_entry(const size_type remove_pos,
                           const size_type canonical_pos) noexcept {
  assert(!is_empty_slot(remove_pos));
  assert(is_occupied[canonical_pos]);

  const bool was_head = !is_continuation[remove_pos];

  // First, move the elements to the left.
  size_type pos = remove_pos;             // Current position.
  size_type quotient_pos = canonical_pos; // Quotient of the current posistion.

  while (true) {
    const size_type next_pos = incr_pos(pos);

    if (!is_shifted[next_pos])
      break;

    set_remainder(pos, get_remainder(next_pos));
    is_continuation[pos] = is_continuation[next_pos];

    // Check for possible new cluster.
    if (!is_continuation[pos]) {
      quotient_pos = find_next_occupied(quotient_pos);
      assert(quotient_pos != next_pos);
      if (quotient_pos == pos)
        is_shifted[pos] = false;
    }

    pos = next_pos;
  }

  // Now the variable 'pos' points to the last slot of the cluster.
  // The last slot becomes empty.
  is_shifted[pos] = false;
  is_continuation[pos] = false;

  // The last element of a cluster is never ocuppied at least it is the only
  // element of the cluster.
  assert(!is_occupied[pos] || pos == remove_pos && pos == canonical_pos);

  if (was_head) {
    if (is_continuation[remove_pos])
      is_continuation[remove_pos] = false; // And the run still exists.
    else
      is_occupied[canonical_pos] = false;
  }
  // is_shifted[remove_pos] could be true or false. Anyway, the new occupant
  // takes the role so is_shifted[remove_pos] remains unmodificated.
}

// ==========================================
// Iterator
// ==========================================

qf_iterator::iterator(const qfilter *const the_filter) noexcept {
  assert(the_filter != nullptr);

  if (the_filter->empty())
    return;

  filter = the_filter;
  canonical_pos = filter->is_occupied[0] ? 0 : filter->find_next_occupied(0);
  pos = filter->find_run_start(canonical_pos);
}

qf_iterator::iterator(const qfilter *const filter_, const size_type pos_,
                      const size_type canonical_pos_) noexcept
    : filter{filter_},
      pos{pos_},
      canonical_pos{canonical_pos_} {
  assert(filter_ != nullptr);
}

void qf_iterator::increment() noexcept {
  assert(filter && "Can't increment end iterator");

  pos = filter->incr_pos(pos);

  if (filter->is_continuation[pos])
    return;

  canonical_pos = filter->find_next_run_quotient(canonical_pos);

  // End was reached.
  if (canonical_pos == filter->num_slots) {
    filter = nullptr;
    pos = 0;
    return;
  }

  // If it is another run on the cluster.
  if (filter->is_shifted[pos])
    return;

  if (!filter->is_occupied[pos]) {
    assert(filter->is_empty_slot(pos));
    pos = filter->find_next_occupied(pos);
  }

  assert(!filter->is_shifted[pos] && !filter->is_continuation[pos]);
}
