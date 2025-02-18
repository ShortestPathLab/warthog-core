cmake_minimum_required(VERSION 3.13)

project(Warthog
	VERSION 0.1.0
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(WARTHOG_INT128 "Enable support for __int128 on gcc and clang" OFF)

include(cmake/submodule.cmake)

add_library(warthog_compile INTERFACE)
add_library(warthog::compile ALIAS warthog_compile)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	target_compile_options(warthog_compile INTERFACE
		$<$<BOOL:${WARTHOG_INT128}>:-march=native>)
endif()

add_library(warthog_core)
add_library(warthog::core ALIAS warthog_core)
target_include_directories(warthog_core PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_link_libraries(warthog_core PUBLIC warthog::compile)
include(cmake/headers.cmake)

add_executable(warthog_app)
add_executable(warthog::warthog ALIAS warthog_app)
set_target_properties(warthog_app PROPERTIES OUTPUT_NAME "warthog")
target_link_libraries(warthog_app PUBLIC warthog::core)

add_subdirectory(src)
add_subdirectory(apps)


#
# Testing setup
#

warthog_top_level()
if(_is_top)
	option(BUILD_TESTING "Building testing suite" OFF)
	include(CTest)
	include(FetchContent)
	FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2.git
		GIT_TAG        v3.7.1
	)

	if(BUILD_TESTING)
		FetchContent_MakeAvailable(Catch2)
		include(Catch)
		add_subdirectory(tests)
	endif()
endif()
