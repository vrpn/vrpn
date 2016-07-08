# - try to find Libnifalcon library
#
#
# Cache Variables: (probably not for direct use in your scripts)
#  LIBNIFALCON_ROOT_DIR - where to start searching
#  LIBNIFALCON_INCLUDE_DIR
#  LIBNIFALCON_LIBRARY
#  LIBNIFALCON_LIBUSB1_LIBRARY - Unix only
#  LIBNIFALCON_LIBUSB1_INCLUDE_DIR - Unix only
#
# Non-cache variables you might use in your CMakeLists.txt:
#  LIBNIFALCON_FOUND
#  LIBNIFALCON_INCLUDE_DIRS
#  LIBNIFALCON_LIBRARIES
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known to be included with CMake >=2.6.2)
#
# 2010 Axel Kohlmeyer, <akohlmey@gmail.com>

set(LIBNIFALCON_ROOT_DIR
	"${LIBNIFALCON_ROOT_DIR}"
	CACHE
	PATH
	"Where to start searching")
mark_as_advanced(LIBNIFALCON_ROOT_DIR)

find_library(LIBNIFALCON_LIBRARY
	NAMES
		libnifalcon
		nifalcon
	HINTS
		"${LIBNIFALCON_ROOT_DIR}"
	PATH_SUFFIXES
		include)
mark_as_advanced(LIBNIFALCON_LIBRARY)

get_filename_component(_libdir "${LIBNIFALCON_LIBRARY}" PATH)

find_path(LIBNIFALCON_INCLUDE_DIR
	NAMES
		falcon/core/FalconDevice.h
	HINTS
		"${_libdir}")
mark_as_advanced(LIBNIFALCON_INCLUDE_DIR)

set(_deps_check)
if(NOT WIN32)
	find_library(LIBNIFALCON_LIBUSB1_LIBRARY NAMES libusb-1.0 usb-1.0)
	mark_as_advanced(LIBNIFALCON_LIBUSB1_LIBRARY)

	get_filename_component(_libdir "${LIBNIFALCON_LIBUSB1_LIBRARY}" PATH)

	find_path(LIBNIFALCON_LIBUSB1_INCLUDE_DIR
		NAMES
			libusb-1.0/libusb.h
		HINTS
			"${_libdir}")
	mark_as_advanced(LIBNIFALCON_LIBUSB1_INCLUDE_DIR)

	set(_deps_check LIBNIFALCON_LIBUSB1_LIBRARY LIBNIFALCON_LIBUSB1_INCLUDE_DIR)

endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibNifalcon
	DEFAULT_MSG
	LIBNIFALCON_LIBRARY
	LIBNIFALCON_INCLUDE_DIR
	${_deps_check})

if(LIBNIFALCON_FOUND)
	set(LIBNIFALCON_LIBRARIES "${LIBNIFALCON_LIBRARY}")
	set(LIBNIFALCON_INCLUDE_DIRS "${LIBNIFALCON_INCLUDE_DIR}" "${Boost_INCLUDE_DIR}")

	if(NOT WIN32)
		list(APPEND LIBNIFALCON_LIBRARIES "${LIBNIFALCON_LIBUSB1_LIBRARY}")
		list(APPEND LIBNIFALCON_INCLUDE_DIRS "${LIBNIFALCON_LIBUSB1_INCLUDE_DIR}")
	endif()
endif()

