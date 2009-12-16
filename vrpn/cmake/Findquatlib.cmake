# - Find quatlib
# Find the quatlib headers and libraries.
#
#  QUATLIB_INCLUDE_DIRS - where to find quat.h
#  QUATLIB_LIBRARIES    - List of libraries when using quatlib.
#  QUATLIB_FOUND        - True if quatlib found.

if(TARGET quat)
	# Look for the header file.
	find_path(QUATLIB_INCLUDE_DIR NAMES quat.h
			PATHS
			${quatlib_SOURCE_DIR}
	)
	mark_as_advanced(QUATLIB_INCLUDE_DIR)

	set(QUATLIB_LIBRARY "quat")

else()
	# Look for the header file.
	find_path(QUATLIB_INCLUDE_DIR NAMES quat.h
			PATHS
			"C:/Program Files/quatlib/include"
			"../quat"
	)
	mark_as_advanced(QUATLIB_INCLUDE_DIR)

	# Look for the library.
	find_library(QUATLIB_LIBRARY NAMES quat.lib libquat.a
			PATHS
			"C:/Program Files/quatlib/lib"
			"../buildquat"
			"../buildquat/release"
	)
	mark_as_advanced(QUATLIB_LIBRARY)
endif()

# handle the QUIETLY and REQUIRED arguments and set QUATLIB_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QUATLIB DEFAULT_MSG QUATLIB_LIBRARY QUATLIB_INCLUDE_DIR)

if(QUATLIB_FOUND)
  set(QUATLIB_LIBRARIES ${QUATLIB_LIBRARY})
  if(NOT WIN32)
  	list(APPEND QUATLIB_LIBRARIES m)
  endif()
  set(QUATLIB_INCLUDE_DIRS ${QUATLIB_INCLUDE_DIR})
else()
  set(QUATLIB_LIBRARIES)
  set(QUATLIB_INCLUDE_DIRS)
endif()
