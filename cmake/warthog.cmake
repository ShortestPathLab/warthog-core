cmake_minimum_required(VERSION 3.13)

# include this file to check submodule include

if(COMMAND warthog_submodule)
	return() # do not redefine
endif()

include(FetchContent)

set(WARTHOG_SUBMODULE_ROOT_ONLY ON CACHE BOOL "add submodule if present only allowed for root project")

function(warthog_top_level)
set(_is_top OFF)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.21)
	if(${PROJECT_IS_TOP_LEVEL})
		set(_is_top ON)
	endif()
else()
	if(${CMAKE_SOURCE_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		set(_is_top ON)
	endif()
endif()
set(_warthog_is_top ${_is_top} PARENT_SCOPE)
endfunction()

# warthog_submodule submodule [override_top]
# adds a submodule to warthog
# has checks to insure submodule is only added once
# if called from the top level project (or optional variable override_top is true)
# then if extern/${submodule}/CMakeLists.txt exists, adds that version instead
#    intended for use with git submodule/subtree.
# otherwise, fetchcontent ${submodule} is made available
# user must add FetchContent_Declare
function(warthog_submodule submodule)
get_property(_submodule_added GLOBAL PROPERTY WARTHOG_${submodule} SET)
if(${_submodule_added}) # submodule already added
	return()
endif()
if((${ARGC} GREATER 1) AND (${ARGV1}))
	set(is_top_level ON)
else()
	warthog_top_level()
	set(is_top_level ${_warthog_is_top})
endif()
if(${is_top_level})
	if(EXISTS "${PROJECT_SOURCE_DIR}/extern/${submodule}/CMakeLists.txt")
		# submodule or subtree, include and exit
		add_subdirectory("${PROJECT_SOURCE_DIR}/extern/${submodule}")
		set_property(GLOBAL PROPERTY WARTHOG_${submodule} 1)
		return()
	endif()
endif()
# failed to add submodule, fetch if able
FetchContent_MakeAvailable(${submodule})
set_property(GLOBAL PROPERTY WARTHOG_${submodule} 1)
endfunction()
