cmake_minimum_required(VERSION 3.1.3 FATAL_ERROR)
project("Quotient-Filter" CXX C) # C is needed for FindThreads

# ---------------------------------------
# Initial config                        
# ---------------------------------------

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type selected, default to Debug")
  set(CMAKE_BUILD_TYPE "Debug")
endif()

find_package(Threads) # Needed for gtest
find_package(Doxygen)

# Integrate utilities

add_subdirectory("utils")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ---------------------------------------
# Add the Quotient filter header-only library  
# ---------------------------------------

add_library(quotient_filter INTERFACE)
target_include_directories(quotient_filter INTERFACE "include")

# The following list does not contain all features used in the library
# but contain the main ones. It will induce cmake to compile with a C++ standard >= 14
target_compile_features(quotient_filter INTERFACE
	cxx_generic_lambdas
	cxx_lambda_init_captures
	cxx_return_type_deduction
	) 

# ---------------------------------------
# Add the unit tests  
# ---------------------------------------

enable_testing()
add_subdirectory("test")

# ---------------------------------------
# Add documentation
# ---------------------------------------

add_subdirectory("doc")
