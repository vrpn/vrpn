# - Find Microsoft DirectShow sample files, library, and headers.
#
#  DirectShow_INCLUDE_DIRS - where to find needed include file
#  DirectShow_BASECLASS_DIR- Directory containing the DirectShow baseclass sample code.
#  DIRECTSHOW_FOUND        - True if DirectShow found.

# Look for one of the sample files.
FIND_PATH(DirectShow_BASECLASS_DIR NAMES streams.h
		PATHS
		"C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2/Samples/Multimedia/DirectShow/BaseClasses"
)
MARK_AS_ADVANCED(DirectShow_BASECLASS_DIR)

# Look for the header files (picking any one from the directories we need).
FIND_PATH(PLATFORM_SDK_INCLUDE_DIR NAMES AclAPI.h
		PATHS
		"C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2/Include"
)
MARK_AS_ADVANCED(PLATFORM_SDK_INCLUDE_DIR)
FIND_PATH(PLATFORM_SDK_ATL_INCLUDE_DIR NAMES atlbase.h
		PATHS
		"C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2/Include/atl"
)
MARK_AS_ADVANCED(PLATFORM_SDK_ATL_INCLUDE_DIR)
FIND_PATH(DIRECTX_SDK_INCLUDE_DIR NAMES comdecl.h
		PATHS
		"C:/Program Files/Microsoft DirectX SDK (August 2006)/Include"
)
MARK_AS_ADVANCED(DIRECTX_SDK_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set DirectShow_FOUND to TRUE if 
# all listed variables are TRUE.  The package name seems to need to be all-caps for this
# to work.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DIRECTSHOW DEFAULT_MSG PLATFORM_SDK_INCLUDE_DIR DirectShow_BASECLASS_DIR)

IF(DIRECTSHOW_FOUND)
  SET(DirectShow_INCLUDE_DIRS ${PLATFORM_SDK_INCLUDE_DIR} ${PLATFORM_SDK_ATL_INCLUDE_DIR} ${DirectShow_BASECLASS_DIR} ${DirectShow_BASECLASS_DIR} ${DIRECTX_SDK_INCLUDE_DIR} )
ELSE(DIRECTSHOW_FOUND)
  SET(DirectShow_INCLUDE_DIRS)
ENDIF(DIRECTSHOW_FOUND)
MARK_AS_ADVANCED(DirectShow_INCLUDE_DIRS)
