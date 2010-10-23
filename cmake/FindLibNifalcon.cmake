# - try to find Libnifalcon library
#
# Cache Variables: (probably not for direct use in your scripts)
#  LIBNIFALCON_INCLUDE_DIR
#  LIBNIFALCON_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  LIBNIFALCON_FOUND
#  LIBNIFALCON_INCLUDE_DIRS
#  LIBNIFALCON_LIBRARIES
#  LIBNIFALCON_RUNTIME_LIBRARY_DIRS
#  LIBNIFALCON_MARK_AS_ADVANCED - whether to mark our vars as advanced even
#    if we don't find this library.
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known to be included with CMake >=2.6.2)
#
# 2010 Axel Kohlmeyer, <akohlmey@gmail.com>

find_library(LIBNIFALCON_LIBRARY
	NAMES libnifalcon)
get_filename_component(_libdir "${LIBNIFALCON_LIBRARY}" PATH)

find_path(LIBNIFALCON_INCLUDE_DIR
	NAMES falcon/core/FalconDevice.h
	HINTS "${_libdir}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libnifalcon 
	DEFAULT_MSG
	LIBNIFALCON_LIBRARY
	LIBNIFALCON_INCLUDE_DIR)

if(NOT WIN32)
	find_library(LIBUSB1_LIBRARY
		NAMES libusb-1.0)
	get_filename_component(_libdir "${LIBUSB1_LIBRARY}" PATH)

	find_path(LIBUSB1_INCLUDE_DIR
		NAMES libusb-1.0/libusb.h
		HINTS "${_libdir}")

	find_package_handle_standard_args(libusb-1.0
		DEFAULT_MSG
		LIBUSB1_LIBRARY
		LIBUSB1_INCLUDE_DIR)
else()
	set(LIBUSB1_LIBRARY "")
	set(LIBUSB1_INCLUDE_DIR "")
endif()

if(LIBNIFALCON_FOUND)
	set(LIBNIFALCON_LIBRARIES "${LIBNIFALCON_LIBRARY}")
	set(LIBNIFALCON_LIBRARY_DIRS "${_libdir}")
	set(LIBNIFALCON_INCLUDE_DIRS "${LIBNIFALCON_INCLUDE_DIR}")

	list(APPEND LIBNIFALCON_LIBRARIES "${LIBUSB1_LIBRARY}")
	list(APPEND LIBNIFALCON_INCLUDE_DIRS "${LIBUSB1_INCLUDE_DIR}")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lrt")
endif()

#mark_as_advanced(LIBNIFALCON_INCLUDE_DIR LIBNIFALCON_LIBRARY)
