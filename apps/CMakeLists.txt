cmake_minimum_required(VERSION 3.13)

# find_package(Getopt)

target_sources(warthog_apps_common PUBLIC cfg.h PRIVATE cfg.cpp)
target_include_directories(warthog_app INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

target_sources(warthog_app PRIVATE warthog.cpp)

target_sources(warthog_graph PRIVATE graph.cpp)
