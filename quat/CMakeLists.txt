cmake_minimum_required(VERSION 2.6)
project(quatlib)

set(QUATLIB_SOURCES matrix.c quat.c vector.c xyzquat.c)
set(QUATLIB_HEADER quat.h)

# Build the library itself and declare what bits need to be installed
add_library(quat ${QUATLIB_SOURCES} ${QUATLIB_HEADER})
set_property(TARGET quat PROPERTY PUBLIC_HEADER ${QUATLIB_HEADER})
set_target_properties(quat PROPERTIES FOLDER Library)
install(TARGETS
	quat
	ARCHIVE
	DESTINATION
	lib
	COMPONENT serversdk
	PUBLIC_HEADER
	DESTINATION
	include
	COMPONENT serversdk)
install(TARGETS
	quat
	ARCHIVE
	DESTINATION
	lib
	COMPONENT clientsdk
	PUBLIC_HEADER
	DESTINATION
	include
	COMPONENT clientsdk)

add_subdirectory(testapps)
