cmake_minimum_required(VERSION 3.13)

foreach(GENNAME ${GEN_COMPILE})
	configure_file(${GEN_TEMPLATE} ${CMAKE_CURRENT_BINARY_DIR}/${GENNAME}.cpp)
endforeach()
