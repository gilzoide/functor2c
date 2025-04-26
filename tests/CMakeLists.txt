add_subdirectory(Catch2)
set_target_properties(Catch2 PROPERTIES CXX_STANDARD 17)

add_executable(functor2c_test functor2c_test.cpp)
target_link_libraries(functor2c_test functor2c Catch2::Catch2WithMain)
set_target_properties(functor2c_test PROPERTIES CXX_STANDARD 17)

add_test(NAME test COMMAND functor2c_test)
