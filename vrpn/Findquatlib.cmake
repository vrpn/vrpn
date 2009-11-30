# - Find quatlib
# Find the quatlib headers and libraries.
#
#  QUATLIB_INCLUDE_DIRS - where to find quat.h
#  QUATLIB_LIBRARIES    - List of libraries when using quatlib.
#  QUATLIB_FOUND        - True if quatlib found.

# Look for the header file.
FIND_PATH(QUATLIB_INCLUDE_DIR NAMES quat.h
		PATHS
		"C:/Program Files/quatlib/include"
		"../quat"
)
MARK_AS_ADVANCED(QUATLIB_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(QUATLIB_LIBRARY NAMES quat.lib libquat.a
		PATHS
		"C:/Program Files/quatlib/lib"
		"../buildquat"
		"../buildquat/release"
)
MARK_AS_ADVANCED(QUATLIB_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set QUATLIB_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(QUATLIB DEFAULT_MSG QUATLIB_LIBRARY QUATLIB_INCLUDE_DIR)

IF(QUATLIB_FOUND)
  SET(QUATLIB_LIBRARIES ${QUATLIB_LIBRARY})
  SET(QUATLIB_INCLUDE_DIRS ${QUATLIB_INCLUDE_DIR})
ELSE(QUATLIB_FOUND)
  SET(QUATLIB_LIBRARIES)
  SET(QUATLIB_INCLUDE_DIRS)
ENDIF(QUATLIB_FOUND)
