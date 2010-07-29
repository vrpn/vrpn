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
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# 2010 Axel Kohlmeyer, <akohlmey@gmail.com>

find_library(LIBNIFALCON_LIBRARY
	NAMES libnifalcon)
get_filename_component(_libdir "${LIBNIFALCON_LIBRARY}" PATH)

find_path(LIBNIFALCON_INCLUDE_DIR
	NAMES falcon/core/FalconDevice.h
	HINTS "${_libdir}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libnifalcon DEFAULT_MSG
	LIBNIFALCON_LIBRARY
	LIBNIFALCON_INCLUDE_DIR)

if(LIBNIFALCON_FOUND)
	set(LIBNIFALCON_LIBRARIES "${LIBNIFALCON_LIBRARY}")
	set(LIBNIFALCON_LIBRARY_DIRS "${_libdir}")

	set(LIBNIFALCON_INCLUDE_DIRS "${LIBNIFALCON_INCLUDE_DIR}")

endif()

if(LIBNIFALCON_FOUND OR LIBNIFALCON_MARK_AS_ADVANCED)
	foreach(_cachevar
		LIBNIFALCON_INCLUDE_DIR
		LIBNIFALCON_LIBRARY)

		mark_as_advanced(${_cachevar})
	endforeach()
endif()
