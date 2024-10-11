cmake_minimum_required(VERSION 3.13)

# find_package(Getopt)

configure_file(config.h.in config.h @ONLY)
target_sources(warthog_app PRIVATE warthog.cpp cfg.cpp)
target_sources(warthog_app PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/config.h cfg.h)
target_include_directories(warthog_app PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
