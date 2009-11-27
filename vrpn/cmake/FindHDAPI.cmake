# - Find SensAble HDAPI and HL libraries and headers.
#
#  HDAPI_INCLUDE_DIRS - where to find HL/hl.h and others
#  HDAPI_LIBRARIES    - List of libraries when using HDAPI/HL.
#  HDAPI_FOUND        - True if HDAPI found.

# Look for the header file.
FIND_PATH(HDAPI_INCLUDE_DIR NAMES HL/hl.h
		PATHS
		"C:/Program Files/SensAble/3DTouch/include"
)
MARK_AS_ADVANCED(HDAPI_INCLUDE_DIR)

FIND_PATH(HDAPI_HDU_INCLUDE_DIR NAMES HDU/hdu.h
		PATHS
		"C:/Program Files/SensAble/3DTouch/utilities/include"
)
MARK_AS_ADVANCED(HDAPI_HDU_INCLUDE_DIR)

FIND_PATH(HDAPI_HLU_INCLUDE_DIR NAMES HLU/hlu.h
		PATHS
		"C:/Program Files/SensAble/3DTouch/utilities/include"
)
MARK_AS_ADVANCED(HDAPI_HLU_INCLUDE_DIR)

# Look for the library.
FIND_LIBRARY(HDAPI_LIBRARY NAMES hd.lib
		PATHS
		"C:/Program Files/SensAble/3DTouch/lib"
)
FIND_LIBRARY(HL_LIBRARY NAMES hl.lib
		PATHS
		"C:/Program Files/SensAble/3DTouch/lib"
)
MARK_AS_ADVANCED(HDAPI_LIBRARY HL_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set HDAPI_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(HDAPI DEFAULT_MSG HDAPI_LIBRARY HDAPI_INCLUDE_DIR)

IF(HDAPI_FOUND)
  SET(HDAPI_LIBRARIES ${HDAPI_LIBRARY} ${HL_LIBRARY})
  SET(HDAPI_INCLUDE_DIRS ${HDAPI_INCLUDE_DIR})
ELSE(HDAPI_FOUND)
  SET(HDAPI_LIBRARIES)
  SET(HDAPI_INCLUDE_DIRS)
ENDIF(HDAPI_FOUND)
MARK_AS_ADVANCED(HDAPI_LIBRARIES)
