cmake_minimum_required(VERSION 3.13)

list(TRANSFORM WARTHOG_CORE_HEADERS
	APPEND .cpp
	OUTPUT_VARIABLE W_COMPILE_SOURCES)
list(TRANSFORM W_COMPILE_SOURCES
	PREPEND "${CMAKE_CURRENT_BINARY_DIR}/"
	OUTPUT_VARIABLE W_COMPILE_SOURCES)

list(JOIN WARTHOG_CORE_HEADERS "\\;" COMPILE_HEADERS_STR)


add_custom_command(OUTPUT ${W_COMPILE_SOURCES}
	COMMAND ${CMAKE_COMMAND} -DGEN_TEMPLATE=${CMAKE_CURRENT_SOURCE_DIR}/compile_file.cpp.in -DGEN_COMPILE="${COMPILE_HEADERS_STR}" -P ${CMAKE_CURRENT_SOURCE_DIR}/generate_src.cmake
)

add_executable(warthog_test_compile compile.cpp)
add_executable(warthog::test::compile ALIAS warthog_test_compile)
get_target_property(W_COMPILE_LINK warthog::core INTERFACE_LINK_LIBRARIES)
target_link_libraries(warthog_test_compile PUBLIC ${W_COMPILE_LINK})
get_target_property(W_COMPILE_INCLUDE warthog::core INTERFACE_INCLUDE_DIRECTORIES)
target_include_directories(warthog_test_compile PUBLIC ${W_COMPILE_INCLUDE})
target_sources(warthog_test_compile PRIVATE ${W_COMPILE_SOURCES})
