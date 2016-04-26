# - try to find Hillcrest Labs' libfreespace library
#
# Cache Variables: (probably not for direct use in your scripts)
#  LIBFREESPACE_INCLUDE_DIR
#  LIBFREESPACE_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  LIBFREESPACE_FOUND
#  LIBFREESPACE_INCLUDE_DIRS
#  LIBFREESPACE_LIBRARIES
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Original Author:
# 2014 Ryan Pavlik <ryan@sensics.com> <abiryan@ryand.net>
# http://academic.cleardefinition.com
#
# Copyright Sensics, Inc. 2014.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(LIBFREESPACE_ROOT_DIR
	"${LIBFREESPACE_ROOT_DIR}"
	CACHE
	PATH
	"Directory to search for libfreespace")

find_library(LIBFREESPACE_LIBRARY
	NAMES
	freespace
	libfreespace
	PATHS
	"${LIBFREESPACE_ROOT_DIR}"
	PATH_SUFFIXES
	lib)

get_filename_component(_libdir "${LIBFREESPACE_LIBRARY}" PATH)

find_path(LIBFREESPACE_INCLUDE_DIR
	NAMES
	freespace/freespace.h
	HINTS
	"${_libdir}"
	"${_libdir}/.."
	PATHS
	"${LIBFREESPACE_ROOT_DIR}"
	PATH_SUFFIXES
	include/)


include(FindPackageHandleStandardArgs)
if(WIN32)
	find_package(WinHID QUIET)
	find_package_handle_standard_args(LibFreespace
		DEFAULT_MSG
		LIBFREESPACE_LIBRARY
		LIBFREESPACE_INCLUDE_DIR
		WINHID_LIBRARIES)
else()
	find_package_handle_standard_args(LibFreespace
		DEFAULT_MSG
		LIBFREESPACE_LIBRARY
		LIBFREESPACE_INCLUDE_DIR)
endif()

if(LIBFREESPACE_FOUND)
	set(LIBFREESPACE_LIBRARIES "${LIBFREESPACE_LIBRARY}")
	if(WIN32)
		list(APPEND LIBFREESPACE_LIBRARIES ${WINHID_LIBRARIES})
	endif()
	set(LIBFREESPACE_INCLUDE_DIRS "${LIBFREESPACE_INCLUDE_DIR}")
	mark_as_advanced(LIBFREESPACE_ROOT_DIR)
endif()

# Get the version number of the Freespace library
set(_libfreespace_version_srcfile "${CMAKE_BINARY_DIR}/freespace_version.cpp")
file(WRITE "${_libfreespace_version_srcfile}"
"
#include <freespace/freespace.h>
#include <algorithm>
#include <iostream>
#include <string>

int main() {
	std::string version = freespace_version();
	std::replace(begin(version), end(version), '.', ';');
	std::cout << version << std::endl;
	return 0;
}
"
)
try_run(_libfreespace_run_result _libfreespace_compile_result
	${CMAKE_BINARY_DIR}
	${_libfreespace_version_srcfile}
	CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${LIBFREESPACE_INCLUDE_DIRS}"
	LINK_LIBRARIES ${LIBFREESPACE_LIBRARIES}
	#COMPILE_OUTPUT_VARIABLE _libfreespace_compiler_output
	RUN_OUTPUT_VARIABLE LIBFREESPACE_VERSION)

if(LIBFREESPACE_VERSION)
	list(GET LIBFREESPACE_VERSION 0 LIBFREESPACE_MAJOR_VERSION)
	list(GET LIBFREESPACE_VERSION 1 LIBFREESPACE_MINOR_VERSION)
	list(GET LIBFREESPACE_VERSION 2 LIBFREESPACE_PATCH_VERSION)
endif()

mark_as_advanced(LIBFREESPACE_INCLUDE_DIR
	LIBFREESPACE_LIBRARY)

