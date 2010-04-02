# - try to find WiiUse library
#
# Cache Variables: (probably not for direct use in your scripts)
#  WIIUSE_INCLUDE_DIR
#  WIIUSE_LIBRARY
#
# Non-cache variables you might use in your CMakeLists.txt:
#  WIIUSE_FOUND
#  WIIUSE_INCLUDE_DIRS
#  WIIUSE_LIBRARIES
#  WIIUSE_RUNTIME_LIBRARY_DIRS
#  WIIUSE_MARK_AS_ADVANCED - whether to mark our vars as advanced even
#    if we don't find this library.
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

endif()

if(WIIUSE_FOUND OR WIIUSE_MARK_AS_ADVANCED)
	foreach(_cachevar
		WIIUSE_INCLUDE_DIR
		WIIUSE_LIBRARY)

		mark_as_advanced(${_cachevar})
	endforeach()
endif()
