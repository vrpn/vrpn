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

# The OVR library is built in a directory tree that varies based on platform,
# architecture, and compiler.
#
# The libraries live in one of the following locations:
#
# Lib/Win32/VS2012/libovrd.lib
# Lib/Win32/VS2012/libovr.lib
# Lib/Win32/VS2013/libovrd.lib
# Lib/Win32/VS2013/libovr.lib
# Lib/Win32/VS2010/libovrd.lib
# Lib/Win32/VS2010/libovr.lib
# Lib/x64/VS2012/libovr64d.lib
# Lib/x64/VS2012/libovr64.lib
# Lib/x64/VS2013/libovr64d.lib
# Lib/x64/VS2013/libovr64.lib
# Lib/x64/VS2010/libovr64d.lib
# Lib/x64/VS2010/libovr64.lib
# Lib/Linux/Release/x86_64/libovr.a
# Lib/Linux/Debug/x86_64/libovr.a
# Lib/Linux/Release/i386/libovr.a
# Lib/Linux/Debug/i386/libovr.a
# Lib/Mac/Release/libovr.a
# Lib/Mac/Debug/libovr.a

set(OVR_LIBRARY_PATH_SUFFIX "Lib")

# Test compiler
if(MSVC10) # Microsoft Visual Studio 2010
	set(_ovr_library_compiler "VS2010")
elseif(MSVC11) # Microsoft Visual Studio 2012
	set(_ovr_library_compiler "VS2012")
elseif(MSVC12) # Microsoft Visual Studio 2013
	set(_ovr_library_compiler "VS2013")
endif()


# Test 32/64 bits
if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	set(_ovr_library_arch "x86_64")
	if (WIN32)
		set(_ovr_library_arch "x64")
	endif(WIN32)
else()
	set(_ovr_library_arch "i386")
	if (WIN32)
		set(_ovr_library_arch "Win32")
	endif(WIN32)
endif()

# Test build type
if((${CMAKE_BUILD_TYPE} MATCHES "Debug") OR (${CMAKE_BUILD_TYPE} MATCHES "RelWithDebugInfo"))
	set(_ovr_library_build_type "Debug")
else()
	set(_ovr_library_build_type "Release")
endif()

# Test platform
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(OVR_LIBRARY_PATH_SUFFIX "Lib/Linux/${_ovr_library_build_type}/${_ovr_library_arch}")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(OVR_LIBRARY_PATH_SUFFIX "Lib/Mac/${_ovr_library_build_type}")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(OVR_LIBRARY_PATH_SUFFIX "Lib/${_ovr_library_arch}/${_ovr_library_compiler}")
endif()

find_library(OVR_LIBRARY
	NAMES
	ovr
	libovr
	PATHS
	${OVR_ROOT_DIR}
	PATH_SUFFIXES
	${OVR_LIBRARY_PATH_SUFFIX}
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
	list(APPEND _ovr_dependencies OPENGL_FOUND)
endif()

if(NOT THREADS_FOUND)
	find_package(Threads)
	list(APPEND _ovr_dependency_libraries ${CMAKE_THREAD_LIBS_INIT})
	list(APPEND _ovr_dependencies THREADS_FOUND)
endif()

# Linux-only dependencies
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	if(NOT X11_FOUND)
		find_package(X11)
		list(APPEND _ovr_dependency_libraries ${X11_LIBRARIES})
		list(APPEND _ovr_dependency_includes ${X11_INCLUDE_DIR})
		list(APPEND _ovr_dependencies X11_FOUND)
	endif()

	if(NOT XRANDR_FOUND)
		find_package(Xrandr)
		list(APPEND _ovr_dependency_libraries ${XRANDR_LIBRARIES})
		list(APPEND _ovr_dependency_includes ${XRANDR_INCLUDE_DIR})
		list(APPEND _ovr_dependencies XRANDR_FOUND)
	endif()

	if(NOT UDEV_FOUND)
		find_package(udev)
		list(APPEND _ovr_dependency_libraries ${UDEV_LIBRARIES})
		list(APPEND _ovr_dependency_includes ${UDEV_INCLUDE_DIR})
		list(APPEND _ovr_dependencies UDEV_FOUND)
	endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OVR
	DEFAULT_MSG
	OVR_LIBRARY
	OVR_INCLUDE_DIR
	OVR_SOURCE_DIR
	${_ovr_dependencies}
	)

if(OVR_FOUND)
	list(APPEND OVR_LIBRARIES ${OVR_LIBRARY} ${_ovr_dependency_libraries})
	list(APPEND OVR_INCLUDE_DIRS ${OVR_INCLUDE_DIR} ${OVR_SOURCE_DIR} ${_ovr_dependency_includes})
	mark_as_advanced(OVR_ROOT_DIR)
endif()

mark_as_advanced(OVR_INCLUDE_DIR
	OVR_SOURCE_DIR
	OVR_LIBRARY)

