add_library(quotient_filter "quotient_filter.cpp")
target_include_directories(quotient_filter PUBLIC "${CMAKE_SOURCE_DIR}/include")

# The following list does not contain all features used in the library
# but contain the main ones. It will induce cmake to compile with a C++ standard >= 14
target_compile_features(quotient_filter PUBLIC
	cxx_generic_lambdas
	cxx_lambda_init_captures
	cxx_return_type_deduction
	)
