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
# architecture, and compiler. Unfortunately, the directory tree and library
# names change from version to version.
#
# Oculus SDK 0.4.x
#
# Lib/Linux/Debug/i386/libovr.a
# Lib/Linux/Debug/x86_64/libovr.a
# Lib/Linux/Release/i386/libovr.a
# Lib/Linux/Release/x86_64/libovr.a
# Lib/Mac/Debug/libovr.a
# Lib/Mac/Release/libovr.a
# Lib/Win32/VS2010/libovrd.lib
# Lib/Win32/VS2010/libovr.lib
# Lib/Win32/VS2012/libovrd.lib
# Lib/Win32/VS2012/libovr.lib
# Lib/Win32/VS2013/libovrd.lib
# Lib/Win32/VS2013/libovr.lib
# Lib/x64/VS2010/libovr64d.lib
# Lib/x64/VS2010/libovr64.lib
# Lib/x64/VS2012/libovr64d.lib
# Lib/x64/VS2012/libovr64.lib
# Lib/x64/VS2013/libovr64d.lib
# Lib/x64/VS2013/libovr64.lib
#
# Oculus 0.5.x
#
# Lib/Linux/i386/Debug/libOVR.a
# Lib/Linux/i386/Debug/libOVRRT32_0.so.5.0.1
# Lib/Linux/i386/Release/libOVR.a
# Lib/Linux/i386/Release/libOVRRT32_0.so.5.0.1
# Lib/Linux/x86_64/Debug/libOVR.a
# Lib/Linux/x86_64/Debug/libOVRRT64_0.so.5.0.1
# Lib/Linux/x86_64/Release/libOVR.a
# Lib/Linux/x86_64/Release/libOVRRT64_0.so.5.0.1
# Lib/Windows/Win32/Debug/VS2010/LibOVR.lib
# Lib/Windows/Win32/Debug/VS2012/LibOVR.lib
# Lib/Windows/Win32/Debug/VS2013/LibOVR.lib
# Lib/Windows/Win32/Release/VS2010/LibOVR.lib
# Lib/Windows/Win32/Release/VS2012/LibOVR.lib
# Lib/Windows/Win32/Release/VS2013/LibOVR.lib
# Lib/Windows/x64/Debug/VS2010/LibOVR.lib
# Lib/Windows/x64/Debug/VS2012/LibOVR.lib
# Lib/Windows/x64/Debug/VS2013/LibOVR.lib
# Lib/Windows/x64/Release/VS2010/LibOVR.lib
# Lib/Windows/x64/Release/VS2012/LibOVR.lib
# Lib/Windows/x64/Release/VS2013/LibOVR.lib
#
# Oculus 0.6.x and above
#
# Lib/Windows/Win32/Release/VS2010/LibOVR.lib
# Lib/Windows/Win32/Release/VS2012/LibOVR.lib
# Lib/Windows/Win32/Release/VS2013/LibOVR.lib
# Lib/Windows/x64/Release/VS2010/LibOVR.lib
# Lib/Windows/x64/Release/VS2012/LibOVR.lib
# Lib/Windows/x64/Release/VS2013/LibOVR.lib
#
# Library path locations:
#
# ROOT + "Lib" + OS + BUILD + ARCH
# ROOT + "Lib" + OS + BUILD
# ROOT + "Lib" + ARCH + COMP
# ROOT + "Lib" + OS + ARCH + BUILD
# ROOT + "Lib" + OS + ARCH + BUILD + COMP
#
# Library filenames:
#
# libovr64d.lib
# libovr64.lib
# libovr.a
# libOVR.a
# libovr.ax64
# libovrd.lib
# libovr.lib
# LibOVR.lib
# libOVRRT32_0.so.5.0.1
# libOVRRT64_0.so.5.0.1
#


set(OVR_LIBRARY_PATH_SUFFIX "Lib")

# Test compiler
if(MSVC10) # Microsoft Visual Studio 2010
	set(_ovr_library_compiler "VS2010")
elseif(MSVC11) # Microsoft Visual Studio 2012
	set(_ovr_library_compiler "VS2012")
elseif(MSVC12) # Microsoft Visual Studio 2013
	set(_ovr_library_compiler "VS2013")
elseif(MSVC14) # Microsoft Visual Studio 2015
	set(_ovr_library_compiler "VS2015")
endif()

# Test 32/64 bits
if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	set(_ovr_libname_bitsuffix "64")
	set(_ovr_library_arch "x86_64")
else()
	set(_ovr_libname_bitsuffix "32")
	set(_ovr_library_arch "i386")
endif()

if(CMAKE_VS_PLATFORM_NAME)
	set(_ovr_library_arch ${CMAKE_VS_PLATFORM_NAME})
endif()

# Test platform
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(_ovr_operating_system "Linux")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(_ovr_operating_system "Mac")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(_ovr_operating_system "Windows")
endif()

# Generate list of potential library locations
list(APPEND _ovr_library_paths
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_operating_system}/Release/${_ovr_library_arch}
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_operating_system}/Release
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_operating_system}/${_ovr_library_arch}/Release
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_operating_system}/${_ovr_library_arch}/Release/${_ovr_library_compiler}
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_library_arch}/Release/${_ovr_library_compiler}
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_library_arch}/${_ovr_library_compiler})

list(APPEND _ovr_library_debug_paths
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_operating_system}/Debug/${_ovr_library_arch}
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_operating_system}/Debug
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_operating_system}/${_ovr_library_arch}/Debug
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_operating_system}/${_ovr_library_arch}/Debug/${_ovr_library_compiler}
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_library_arch}/Debug/${_ovr_library_compiler}
	${OVR_LIBRARY_PATH_SUFFIX}/${_ovr_library_arch}/${_ovr_library_compiler})

# Generate list of potential library names
list(APPEND _ovr_dynamic_library_names
	OVRRT${_ovr_libname_bitsuffix}
	OVRRT${_ovr_libname_bitsuffix}_0
	libOVRRT${_ovr_libname_bitsuffix}.so.5
	libOVRRT${_ovr_libname_bitsuffix}_0.so.5)

list(APPEND _ovr_static_library_names
	libovr.ax${_ovr_libname_bitsuffix}
	libovr.a
	ovr
	OVR
	LibOVR
	ovr${_ovr_libname_bitsuffix}
	libovr${_ovr_libname_bitsuffix})

list(APPEND _ovr_dynamic_library_debug_names
	OVRRT${_ovr_libname_bitsuffix}
	OVRRT${_ovr_libname_bitsuffix}_0
	libOVRRT${_ovr_libname_bitsuffix}.so.5
	libOVRRT${_ovr_libname_bitsuffix}_0.so.5)

list(APPEND _ovr_static_library_debug_names
	libovr.a
	ovr
	OVR
	LibOVR
	ovr${_ovr_libname_bitsuffix}d
	libovr${_ovr_libname_bitsuffix}d
	OVR${_ovr_libname_bitsuffix}d)

find_library(OVR_DYNAMIC_LIBRARY_RELEASE
	NAMES
	${_ovr_dynamic_library_names}
	PATHS
	"${OVR_ROOT_DIR}"
	"${OVR_ROOT_DIR}/LibOVR"
	PATH_SUFFIXES
	${_ovr_library_paths}
	lib
	lib64
)

find_library(OVR_DYNAMIC_LIBRARY_DEBUG
	NAMES
	${_ovr_dynamic_library_debug_names}
	PATHS
	"${OVR_ROOT_DIR}"
	"${OVR_ROOT_DIR}/LibOVR"
	PATH_SUFFIXES
	${_ovr_library_debug_paths}
	lib
	lib64
)

find_library(OVR_STATIC_LIBRARY_RELEASE
	NAMES
	${_ovr_static_library_names}
	PATHS
	"${OVR_ROOT_DIR}"
	"${OVR_ROOT_DIR}/LibOVR"
	PATH_SUFFIXES
	${_ovr_library_paths}
	lib
	lib64
)

find_library(OVR_STATIC_LIBRARY_DEBUG
	NAMES
	${_ovr_static_library_debug_names}
	PATHS
	"${OVR_ROOT_DIR}"
	"${OVR_ROOT_DIR}/LibOVR"
	PATH_SUFFIXES
	${_ovr_library_debug_paths}
	lib
	lib64
)

# Prefer dynamic libraries over static libraries
foreach(_config DEBUG RELEASE)
    if(OVR_DYNAMIC_LIBRARY_${_config})
        set(OVR_LIBRARY_${_config} ${OVR_DYNAMIC_LIBRARY_${_config}})
    else()
        set(OVR_LIBRARY_${_config} ${OVR_STATIC_LIBRARY_${_config}})
    endif()
endforeach()

include(SelectLibraryConfigurations)
select_library_configurations(OVR)

if(OVR_DYNAMIC_LIBRARY_RELEASE)
    get_filename_component(_libdir "${OVR_DYNAMIC_LIBRARY_RELEASE}" PATH)
elseif(OVR_STATIC_LIBRARY_RELEASE)
    get_filename_component(_libdir "${OVR_STATIC_LIBRARY_RELEASE}" PATH)
endif()

find_path(OVR_INCLUDE_DIR
	NAMES
	OVR.h
	HINTS
	"${_libdir}"
	"${_libdir}/.."
	"${_libdir}/../.."
	"${_libdir}/../../.."
	PATHS
	"${OVR_ROOT_DIR}"
	"${OVR_ROOT_DIR}/LibOVR"
	PATH_SUFFIXES
	include
	Include
)

# Some versions of Oculus SDK keep additional header files in a separate directory.
# They keep changing the names of these files, too.

find_path(OVR_SOURCE_DIR
	NAMES
	OVR_CAPI.h
	HINTS
	"${_libdir}"
	"${_libdir}/.."
	"${_libdir}/../.."
	"${_libdir}/../../.."
	PATHS
	"${OVR_ROOT_DIR}"
	"${OVR_ROOT_DIR}/LibOVR"
	PATH_SUFFIXES
	Src
	Include
	include
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

if(WIN32)
	#find_library(OVR_WINMM_LIBRARY winmm)
	#find_library(OVR_WS2_LIBRARY ws2_32)
	list(APPEND _ovr_dependency_libraries winmm ws2_32)#${OVR_WINMM_LIBRARY} ${OVR_WS2_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OVR
	DEFAULT_MSG
	OVR_LIBRARY
	OVR_SOURCE_DIR
	${_ovr_dependencies}
)

if(OVR_FOUND)
	set(OVR_LIBRARIES ${OVR_LIBRARY} ${_ovr_dependency_libraries} ${CMAKE_DL_LIBS})
	list(APPEND OVR_INCLUDE_DIRS ${OVR_SOURCE_DIR} ${_ovr_dependency_includes})
	if (OVR_INCLUDE_DIR)
		list(APPEND OVR_INCLUDE_DIRS ${OVR_INCLUDE_DIR})
	endif()
	mark_as_advanced(OVR_ROOT_DIR)
endif()

mark_as_advanced(OVR_INCLUDE_DIR
	OVR_SOURCE_DIR
	OVR_LIBRARY
	OVR_WINMM_LIBRARY
	OVR_WS2_LIBRARY)

