cmake_minimum_required(VERSION 3.19)

project(functor2c)

option(FUNCTOR2C_BUILD_TESTS "Whether to build automated tests" OFF)

add_library(functor2c INTERFACE functor2c.hpp)
target_compile_features(functor2c INTERFACE cxx_std_11)
target_include_directories(functor2c INTERFACE .)

if(FUNCTOR2C_BUILD_TESTS)
  include(CTest)
  add_subdirectory(tests)
endif()
