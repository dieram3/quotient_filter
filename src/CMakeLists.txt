add_library(quotient_filter "quotient_filter_fp.cpp")

target_include_directories(quotient_filter PUBLIC "${CMAKE_SOURCE_DIR}/include")

# The following list some features introduced in C++14. Thereby, CMake induces 
# which must compile the library using a C++ standard >= 14
target_compile_features(quotient_filter PUBLIC
	cxx_generic_lambdas
	cxx_lambda_init_captures
	cxx_return_type_deduction
	)

# Set warnings on AppleClang and Clang COMPILER IDS
target_compile_options(quotient_filter PRIVATE 
  $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:
  -Weverything 
  -Wno-c++98-compat
  -Wno-c++98-compat-pedantic
  -Wno-padded
  >)
