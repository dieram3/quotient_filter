function(add_test test_name)
  include_directories(${PROJECT_SOURCE_DIR}/utils/gtest-1.7.0/include)

  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name}
    gtest
    gtest_main
    )
endfunction()

add_test(quofil_test "quotient_filter_test.cpp")
