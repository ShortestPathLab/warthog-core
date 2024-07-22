cmake_minimum_required(VERSION 3.12)

project(Warthog
	VERSION 0.1.0
	LANGUAGE CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_library(warthog_core)
add_library(warthog::core ALIAS warthog_core)
target_include_directories(warthog_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
