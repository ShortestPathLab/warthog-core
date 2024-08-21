cmake_minimum_required(VERSION 3.21)

project(Warthog
	VERSION 0.1.0
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# only compile binaries by default if top level, as apps will search for additional deps
set(WARTHOG_COMPILE_BINARIES ${PROJECT_IS_TOP_LEVEL} CACHE BOOL "Compile warthog binaries located in apps/")

add_library(warthog_core)
add_library(warthog::core ALIAS warthog_core)
target_include_directories(warthog_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
include(cmake/headers.cmake)

add_executable(warthog_app)
add_executable(warthog::warthog ALIAS warthog_app)
set_target_properties(warthog_app PROPERTIES OUTPUT_NAME "warthog")
target_link_libraries(warthog_app PUBLIC warthog::core)

add_subdirectory(src)
add_subdirectory(apps)
