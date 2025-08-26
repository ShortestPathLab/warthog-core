cmake_minimum_required(VERSION 3.13)

# include this file to check submodule include

# set here as only
set(warthog_https_warthog-core "https://github.com/ShortestPathLab/warthog-core.git" CACHE INTERNAL "")
set(warthog_https_warthog-jps "https://github.com/ShortestPathLab/warthog-jps.git" CACHE INTERNAL "")

if(COMMAND warthog_module)
	return() # do not redefine
endif()

include(FetchContent)

set(WARTHOG_MODULE_ROOT_ONLY ON CACHE BOOL "add submodule if present only allowed for root project")

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

# warthog_module module [override_top]
# adds a module to warthog
# has checks to insure module is only added once
# if called from the top level project (or optional variable override_top is true)
# then if extern/${module}/CMakeLists.txt exists, adds that version instead
#    intended for use with git module/subtree.
# otherwise, fetchcontent ${module} is made available
# user must add FetchContent_Declare
function(warthog_module module)
get_property(_module_added GLOBAL PROPERTY WARTHOG_${module} SET)
if(${_module_added}) # module already added
	return()
endif()
if((${ARGC} GREATER 1) AND (${ARGV1}))
	set(is_top_level ON)
else()
	warthog_top_level()
	set(is_top_level ${_warthog_is_top})
endif()
if(${is_top_level})
	if(EXISTS "${PROJECT_SOURCE_DIR}/extern/${module}/CMakeLists.txt")
		# module or subtree, include and exit
		add_subdirectory("${PROJECT_SOURCE_DIR}/extern/${module}")
		set_property(GLOBAL PROPERTY WARTHOG_${module} ON)
		return()
	endif()
endif()
# failed to add module, fetch if able
warthog_module_declare(${module})
FetchContent_MakeAvailable(${module})
set_property(GLOBAL PROPERTY WARTHOG_${module} ON)
endfunction()

function(warthog_module_declare module)
if(NOT DEFINED warthog_https_${module})
	return()
endif()
if((${ARGC} GREATER 1))
	set(git_tag ${ARGV1})
else()
	set(git_tag "main")
endif()
FetchContent_Declare(${module}
	GIT_REPOSITORY ${warthog_https_${module}}
	GIT_TAG ${git_tag})
endfunction()
