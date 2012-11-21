# - try to find NIDAQMX library
#
# Cache Variables: (probably not for direct use in your scripts)
#  NIDAQMX_INCLUDE_DIR
#  NIDAQMX_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  NIDAQMX_FOUND
#  NIDAQMX_INCLUDE_DIRS
#  NIDAQMX_LIBRARIES
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Original Author:
# 2011 Russell M. Taylor II <taylorr@cs.unc.edu>

# Copyright University of North Carolina at Chapel Hill 2011
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

find_library(NIDAQMX_LIBRARY
	NAMES
	NIDAQmx
	PATHS
	"C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev/lib/msvc")

find_path(NIDAQMX_INCLUDE_DIR
	NAMES
	NIDAQmx.h
	PATHS
	"C:/Program Files (x86)/National Instruments/NI-DAQ/DAQmx ANSI C Dev/include")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NIDAQmx
	DEFAULT_MSG
	NIDAQMX_LIBRARY
	NIDAQMX_INCLUDE_DIR)

if(NIDAQMX_FOUND)
	set(NIDAQMX_LIBRARIES "${NIDAQMX_LIBRARY}")

	set(NIDAQMX_INCLUDE_DIRS "${NIDAQMX_INCLUDE_DIR}")
endif()

mark_as_advanced(NIDAQMX_INCLUDE_DIR NIDAQMX_LIBRARY)

