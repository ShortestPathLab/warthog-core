cmake_minimum_required(VERSION 3.13)

add_executable(warthog_test_units)
target_link_libraries(warthog_test_memory Catch2::Catch2WithMain warthog::core)
# catch_discover_tests(warthog_test_memory)
