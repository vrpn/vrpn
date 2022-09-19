# Try to find libi2c-dev, which is a header-only library on
# Raspberry Pi
#
# Cache Variables: (probably not for direct use in your scripts)
#  I2CDEV_INCLUDE_DIR
#
# Non-cache variables you might use in your CMakeLists.txt:
#  I2CDEV_FOUND
#  I2CDEV_INCLUDE_DIRS
#     (You need to include linux/i2c-dev.h from this location)
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Copyright ReliaSolve 2016.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(I2CDEV_ROOT_DIR
	"${I2CDEV_ROOT_DIR}"
	CACHE
	PATH
	"Directory to search for libi2c-dev")

if(CMAKE_SIZEOF_VOID_P MATCHES "8")
	set(_LIBSUFFIXES /lib64 /lib)
else()
	set(_LIBSUFFIXES /lib)
endif()

find_path(I2CDEV_INCLUDE_DIR
	NAMES
	linux/i2c-dev.h
	HINTS
	"${_libdir}"
	PATHS
	"${I2CDEV_ROOT_DIR}"
	PATH_SUFFIXES
	include/)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(I2CDEV
	DEFAULT_MSG
	I2CDEV_INCLUDE_DIR
	)

if(I2CDEV_FOUND)
	set(I2CDEV_INCLUDE_DIRS "${I2CDEV_INCLUDE_DIR}")
	mark_as_advanced(I2CDEV_ROOT_DIR)
endif()

mark_as_advanced(I2CDEV_INCLUDE_DIR)

