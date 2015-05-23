#include <quofil/quotient_filter.hpp>
#include <gtest/gtest.h>

TEST(quotient_filter, getFunctions) {
  quofil::quotient_filter qfilter;
  EXPECT_TRUE(qfilter.getTrue());
  EXPECT_FALSE(qfilter.getFalse());
}
