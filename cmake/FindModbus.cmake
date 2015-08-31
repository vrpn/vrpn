# - Find Modbus library
# Find the Modbus headers and libraries.
#
#  MODBUS_INCLUDE_DIRS - where to find modbus.h
#  MODBUS_LIBRARIES    - List of libraries when using modbus.
#  MODBUS_FOUND        - True if modbus library found.
#

# Based on Findquatlib.cmake, Original Author:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
#
# Copyright Iowa State University 2009-2010.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(MODBUS_ROOT_DIR
	"${MODBUS_ROOT_DIR}"
	CACHE
	PATH
	"Root directory to search for libmodbus")

if("${CMAKE_SIZEOF_VOID_P}" MATCHES "8")
	set(_libsuffixes lib64 lib)

	# 64-bit dir: only set on win64
	file(TO_CMAKE_PATH "$ENV{ProgramW6432}" _progfiles)
else()
	set(_libsuffixes lib)
	set(_PF86 "ProgramFiles(x86)")
	if(NOT "$ENV{${_PF86}}" STREQUAL "")
		# 32-bit dir: only set on win64
		file(TO_CMAKE_PATH "$ENV{${_PF86}}" _progfiles)
	else()
		# 32-bit dir on win32, useless to us on win64
		file(TO_CMAKE_PATH "$ENV{ProgramFiles}" _progfiles)
	endif()
endif()

# Look for the header file.
find_path(MODBUS_INCLUDE_DIR
	NAMES
	modbus.h
	HINTS
	"${MODBUS_ROOT_DIR}"
	PATH_SUFFIXES
	include
	PATHS
	"${_progfiles}/libmodbus"
	C:/usr/local
	/usr/local)

# Look for the library.
find_library(MODBUS_LIBRARY
	NAMES
	libmodbus.lib
	libmodbus.a
	HINTS
	"${MODBUS_ROOT_DIR}"
	PATH_SUFFIXES
	${_libsuffixes}
	PATHS
	"${_progfiles}/libmodbus"
	C:/usr/local
	/usr/local)

# handle the QUIETLY and REQUIRED arguments and set MODBUS_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Modbus
	DEFAULT_MSG
	MODBUS_LIBRARY
	MODBUS_INCLUDE_DIR)

if(MODBUS_FOUND)
	set(MODBUS_LIBRARIES ${MODBUS_LIBRARY})
	set(MODBUS_INCLUDE_DIRS ${MODBUS_INCLUDE_DIR})

	mark_as_advanced(MODBUS_ROOT_DIR)
else()
	set(MODBUS_LIBRARIES)
	set(MODBUS_INCLUDE_DIRS)
endif()

mark_as_advanced(MODBUS_LIBRARY MODBUS_INCLUDE_DIR)

