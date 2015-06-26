function(add_unittest TEST_NAME)
  set(TARGET_ID "${TEST_NAME}_test")
  
  add_executable(${TARGET_ID} ${ARGN})
  target_link_libraries(${TARGET_ID} quotient_filter gtest_main gtest)
  set_property(TARGET ${TARGET_ID} PROPERTY CXX_EXTENSIONS OFF)

  # Set warnings on AppleClang and Clang COMPILER IDS
  target_compile_options(${TARGET_ID} PRIVATE 
  $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:
  -Weverything 
  -Wno-c++98-compat 
  -Wno-c++98-compat-pedantic
  -Wno-padded
  -Wno-sign-conversion 
  -Wno-global-constructors
  >)

  add_test(NAME ${TEST_NAME} COMMAND ${TARGET_ID})
endfunction()

add_unittest("quofil" "quotient_filter_test.cpp")
