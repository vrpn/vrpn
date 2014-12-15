# - try to find Oculus VR's SDK for Oculus Rift support
#
# Cache Variables: (probably not for direct use in your scripts)
#  OVR_INCLUDE_DIR
#  OVR_SOURCE_DIR
#  OVR_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  OVR_FOUND
#  OVR_INCLUDE_DIRS
#  OVR_LIBRARIES
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Original Author:
# 2014 Kevin M. Godby <kevin@godby.org>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(OVR_ROOT_DIR
	"${OVR_ROOT_DIR}"
	CACHE
	PATH
    "Directory to search for Oculus SDK")

find_library(OVR_LIBRARY
	NAMES
	ovr
	PATHS
	"${OVR_ROOT_DIR}"
	PATH_SUFFIXES
	lib
	Lib/Linux/Release/i386
	Lib/Linux/Release/x86_64
	)

get_filename_component(_libdir "${OVR_LIBRARY}" PATH)

find_path(OVR_INCLUDE_DIR
	NAMES
	OVR.h
	HINTS
	"${_libdir}"
	"${_libdir}/.."
	PATHS
	"${OVR_ROOT_DIR}"
	PATH_SUFFIXES
	include
	Include
	)

find_path(OVR_SOURCE_DIR
	NAMES
	OVR_CAPI.h
	HINTS
	"${_libdir}"
	"${_libdir}/.."
	PATHS
	"${OVR_ROOT_DIR}"
	PATH_SUFFIXES
	Src
	)

# Dependencies

set(_ovr_dependency_libraries "")
set(_ovr_dependency_includes "")

if(NOT OPENGL_FOUND)
	find_package(OpenGL)
	list(APPEND _ovr_dependency_libraries ${OPENGL_LIBRARIES})
	list(APPEND _ovr_dependency_includes ${OPENGL_INCLUDE_DIR})
endif()

if(NOT THREADS_FOUND)
	find_package(Threads)
	list(APPEND _ovr_dependency_libraries ${CMAKE_THREAD_LIBS_INIT})
endif()

# Linux-only dependencies
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	if(NOT X11_FOUND)
		find_package(X11)
		list(APPEND _ovr_dependency_libraries ${X11_LIBRARIES})
		list(APPEND _ovr_dependency_includes ${X11_INCLUDE_DIR})
	endif()

	if(NOT XRANDR_FOUND)
		find_package(Xrandr)
		list(APPEND _ovr_dependency_libraries ${XRANDR_LIBRARIES})
		list(APPEND _ovr_dependency_includes ${XRANDR_INCLUDE_DIR})
	endif()

	if(NOT UDEV_FOUND)
		find_package(udev)
		list(APPEND _ovr_dependency_libraries ${UDEV_LIBRARIES})
		list(APPEND _ovr_dependency_includes ${UDEV_INCLUDE_DIR})
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OVR
	DEFAULT_MSG
	OVR_LIBRARY
	OVR_INCLUDE_DIR
	OVR_SOURCE_DIR
	OPENGL_FOUND
	THREADS_FOUND
	X11_FOUND
	XRANDR_FOUND
	UDEV_FOUND
	)

if(OVR_FOUND)
	list(APPEND OVR_LIBRARIES ${OVR_LIBRARY} ${_ovr_dependency_libraries})
	list(APPEND OVR_INCLUDE_DIRS ${OVR_INCLUDE_DIR} ${OVR_SOURCE_DIR} ${_ovr_dependency_includes})
	mark_as_advanced(OVR_ROOT_DIR)
endif()

mark_as_advanced(OVR_INCLUDE_DIR
	OVR_SOURCE_DIR
	OVR_LIBRARY)

