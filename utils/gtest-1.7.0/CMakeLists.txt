include_directories("include" ".")
add_library(gtest "src/gtest-all.cc")
add_library(gtest_main "src/gtest_main.cc")

set(GTEST_INCLUDE_DIRS "$CMAKE_CURRENT_SOURCE_DIR/include" PARENT_SCOPE)
