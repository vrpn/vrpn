# - try to find Thalmix Labs' myo library (WIN32 ONLY)
#
# Cache Variables: (probably not for direct use in your scripts)
#  THALMICLABSMYO_INCLUDE_DIRS
#  THALMICLABSMYO_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  THALMICLABSMYO_FOUND
#  THALMICLABSMYO_INCLUDE_DIRS
#  THALMICLABSMYO_LIBRARIES
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
if(WIN32)
set(THALMICLABSMYO_ROOT_DIR
	"${THALMICLABSMYO_ROOT_DIR}"
	CACHE
	PATH
	"Directory to search for the Thalmic Labs Myo SDK")


if(CMAKE_SIZEOF_VOID_P MATCHES "8")
	set(_ARCH x86_64)
else()
	set(_ARCH x86_32)
endif()


set(_SDKDIR Windows)


find_path(THALMICLABSMYO_INCLUDE_DIR
	NAMES myo/libmyo.h
	PATHS "${THALMICLABSMYO_ROOT_DIR}/include")



include(FindPackageHandleStandardArgs)

if(CMAKE_SIZEOF_VOID_P MATCHES "8")
find_library(THALMICLABSMYO_LIBRARY
	NAMES myo64
	PATHS "${THALMICLABSMYO_ROOT_DIR}/lib"
	PATH_SUFFIXES "${_SDKDIR}/${_ARCH}")
else()
find_library(THALMICLABSMYO_LIBRARY
	NAMES myo32
	PATHS "${THALMICLABSMYO_ROOT_DIR}/lib"
	PATH_SUFFIXES "${_SDKDIR}/${_ARCH}")
endif()




find_package_handle_standard_args(ThalmicLabsMyo
	DEFAULT_MSG
	THALMICLABSMYO_LIBRARY
	THALMICLABSMYO_INCLUDE_DIR
	${_deps_check})


if(THALMICLABSMYO_FOUND)
	set(THALMICLABSMYO_LIBRARIES "${THALMICLABSMYO_LIBRARY}")
	set(THALMICLABSMYO_INCLUDE_DIRS "${THALMICLABSMYO_INCLUDE_DIR}")
	mark_as_advanced(THALMICLABSMYO_ROOT_DIR)
endif()


mark_as_advanced(THALMICLABSMYO_INCLUDE_DIR THALMICLABSMYO_LIBRARY)
endif()
