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
# Utility functions
# ---------------------------------------

function(target_enable_all_warnings TARGET_ID)
  target_compile_options(${TARGET_ID} PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:
    -Weverything
    -Wno-c++98-compat
    -Wno-c++98-compat-pedantic
    -Wno-padded
  >)
endfunction()

function(target_disable_global_constructor_warning TARGET_ID)
  target_compile_options(${TARGET_ID} PRIVATE
  $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>:
    -Wno-global-constructors
  >)
endfunction()

function(configure_qf_target TARGET_ID)
  target_enable_all_warnings(${TARGET_ID})
  set_property(TARGET ${TARGET_ID} PROPERTY CXX_EXTENSIONS OFF)
endfunction()

function(add_qf_executable TARGET_ID)
  add_executable(${TARGET_ID} ${ARGN})
  target_link_libraries(${TARGET_ID} quotient_filter)
  configure_qf_target(${TARGET_ID})
endfunction()

# ---------------------------------------
# Add the Quotient-Filter library  
# ---------------------------------------

add_subdirectory(src)

# ---------------------------------------
# Add the unit tests  
# ---------------------------------------

enable_testing()
add_subdirectory("test")

# ---------------------------------------
# Add documentation
# ---------------------------------------

add_subdirectory("doc")

# ---------------------------------------
# Add benchmarks
# ---------------------------------------

add_subdirectory("benchmarks")
