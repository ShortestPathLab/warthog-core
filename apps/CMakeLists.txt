cmake_minimum_required(VERSION 3.13)

# find_package(Getopt)

target_sources(warthog_app PRIVATE warthog.cpp cfg.cpp)
target_sources(warthog_app PUBLIC cfg.h)
target_include_directories(warthog_app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
