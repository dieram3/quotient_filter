#ifndef QUOFIL_QUOTIENT_FILTER_HPP
#define QUOFIL_QUOTIENT_FILTER_HPP

namespace quofil {

class quotient_filter {
public:
  constexpr bool get_true() const { return true; }
  constexpr bool get_false() const { return false; }
};

} // namespace quofil

#endif // Header guard
