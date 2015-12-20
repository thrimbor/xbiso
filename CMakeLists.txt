cmake_minimum_required (VERSION 2.6)
set (CMAKE_CXX_STANDARD 11)

project (xbiso)

set (XBISO_VERSION 0.7.1)
set (XBISO_OS ${CMAKE_SYSTEM_NAME})

configure_file (
	"${PROJECT_SOURCE_DIR}/xbisoConfig.h.in"
	"${PROJECT_BINARY_DIR}/xbisoConfig.h"
)

include_directories("${PROJECT_BINARY_DIR}")

add_executable(xbiso xbiso.cpp xdvdfs.cpp)

install (TARGETS xbiso DESTINATION bin)

