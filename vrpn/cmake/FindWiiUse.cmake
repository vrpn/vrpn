# - try to find WiiUse library
#
# Cache Variables: (probably not for direct using in your scripts)
#  WIIUSE_INCLUDE_DIR
#  WIIUSE_LIBRARY
#
# Non-cache variables you should use in your CMakeLists.txt:
#  WIIUSE_INCLUDE_DIRS
#  WIIUSE_LIBRARIES
#  WIIUSE_RUNTIME_LIBRARY_DIRS
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Original Author:
# 2009 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

find_library(WIIUSE_LIBRARY
	NAMES wiiuse)
get_filename_component(_libdir "${WIIUSE_LIBRARY}" PATH)

find_path(WIIUSE_INCLUDE_DIR
	NAMES wiiuse.h
	HINTS "${_libdir}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WiiUse DEFAULT_MSG
	WIIUSE_LIBRARY
	WIIUSE_INCLUDE_DIR)

if(WIIUSE_FOUND)
	set(WIIUSE_LIBRARIES "${WIIUSE_LIBRARY}")
	set(WIIUSE_LIBRARY_DIRS "${_libdir}")

	set(WIIUSE_INCLUDE_DIRS "${WIIUSE_INCLUDE_DIR}")

	foreach(_cachevar
		WIIUSE_INCLUDE_DIR
		WIIUSE_LIBRARY)

		mark_as_advanced(${_cachevar})
	endforeach()

endif()
