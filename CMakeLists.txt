cmake_minimum_required(VERSION 3.13)

project(Warthog
	VERSION 0.1.0
	LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(cmake/submodule.cmake)

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
