cmake_minimum_required(VERSION 3.23)

project(Warthog
	VERSION 0.6.1
	LANGUAGES CXX C)

set_property(GLOBAL PROPERTY WARTHOG_warthog-core ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

option(WARTHOG_POSTHOC "Compile trace support generation for posthoc" OFF)
option(WARTHOG_INT128 "Enable support for __int128 on gcc and clang" OFF)
option(WARTHOG_BMI "Enable support cpu BMI for WARTHOG_INTRIN_HAS(BMI)" OFF)
option(WARTHOG_BMI2 "Enable support cpu BMI2 for WARTHOG_INTRIN_HAS(BMI2), use for Zen 3+" OFF)
option(WARTHOG_INTRIN_ALL "Enable march=native and support x86 intrinsics if able (based on system), supersedes all manual instruction sets" OFF)

find_package(Threads REQUIRED)

include(cmake/warthog.cmake)

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
target_link_libraries(warthog_core PUBLIC warthog::compile Threads::Threads)

# headers
include(cmake/headers.cmake)
list(TRANSFORM WARTHOG_CORE_HEADERS PREPEND "include/" OUTPUT_VARIABLE WARTHOG_CORE_SOURCE_HEADERS)

configure_file(cmake/config.h.in include/warthog/config.h @ONLY)
target_sources(warthog_core
	PUBLIC
	${CMAKE_CURRENT_BINARY_DIR}/include/warthog/config.h

	PUBLIC
	FILE_SET HEADERS
	BASE_DIRS include ${CMAKE_CURRENT_BINARY_DIR}/include
	FILES
		${WARTHOG_CORE_SOURCE_HEADERS}
		${CMAKE_CURRENT_BINARY_DIR}/include/warthog/config.h
	)

add_executable(warthog_app)
add_executable(warthog::warthog ALIAS warthog_app)
set_target_properties(warthog_app PROPERTIES OUTPUT_NAME "warthog")
target_link_libraries(warthog_app PUBLIC warthog::core)

add_subdirectory(src)
add_subdirectory(apps)


install(TARGETS warthog_core warthog_app
    RUNTIME 
        COMPONENT Runtime
    LIBRARY 
        COMPONENT Runtime
    ARCHIVE 
        COMPONENT Development
	FILE_SET HEADERS
        COMPONENT Development
)

#
# Testing setup
#

warthog_top_level()
if(${_warthog_is_top})
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
