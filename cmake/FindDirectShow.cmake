# - Find Microsoft DirectShow sample files, library, and headers.
#
#  DirectShow_INCLUDE_DIRS - where to find needed include file
#  DirectShow_BASECLASS_DIR- Directory containing the DirectShow baseclass sample code.
#  DIRECTSHOW_FOUND        - True if DirectShow found.

# Look for one of the sample files.
FIND_PATH(DirectShow_BASECLASS_DIR NAMES streams.h
		PATHS
		"C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2/Samples/Multimedia/DirectShow/BaseClasses"
		"C:/Program Files/Microsoft SDKs/Windows/v7.1/Samples/multimedia/directshow/baseclasses"
)
MARK_AS_ADVANCED(DirectShow_BASECLASS_DIR)

# Look for the header files (picking any one from the directories we need).
FIND_PATH(PLATFORM_SDK_INCLUDE_DIR NAMES qedit.h
		PATHS
		"C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2/Include"
		"C:/Program Files/Microsoft SDKs/Windows/v7.1/Include"
		"C:/Program Files/Microsoft SDKs/Windows/v6.0A/Include"
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
		"C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Include"
		"C:/Program Files/Microsoft DirectX SDK (June 2010)/Include"
)
MARK_AS_ADVANCED(DIRECTX_SDK_INCLUDE_DIR)

if(MSVC AND CMAKE_CL_64)
	set(DIRECTSHOW_LIB_SUBDIR /x64)
else()
	set(DIRECTSHOW_LIB_SUBDIR)
endif()
find_library(DIRECTSHOW_STRMIIDS_LIBRARY
	NAMES strmiids
	PATHS
	"${PLATFORM_SDK_INCLUDE_DIR}/../Lib${DIRECTSHOW_LIB_SUBDIR}")

# handle the QUIETLY and REQUIRED arguments and set DirectShow_FOUND to TRUE if 
# all listed variables are TRUE.  The package name seems to need to be all-caps for this
# to work.
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(DIRECTSHOW DEFAULT_MSG PLATFORM_SDK_INCLUDE_DIR DirectShow_BASECLASS_DIR DIRECTSHOW_STRMIIDS_LIBRARY)

IF(DIRECTSHOW_FOUND)
  SET(DirectShow_INCLUDE_DIRS
	# Baseclass must be before SDK so it gets the correct refclock.h
	${DirectShow_BASECLASS_DIR}
	${DIRECTX_SDK_INCLUDE_DIR}
	${PLATFORM_SDK_INCLUDE_DIR}
  )
  IF (PLATFORM_SDK_ATL_INCLUDE_DIR)
    SET(DirectShow_INCLUDE_DIRS
	${DirectShow_INCLUDE_DIRS}
	${PLATFORM_SDK_ATL_INCLUDE_DIR}
    )
  ENDIF (PLATFORM_SDK_ATL_INCLUDE_DIR)

  set(DIRECTSHOW_LIBRARIES ${DIRECTSHOW_STRMIIDS_LIBRARY})
ELSE(DIRECTSHOW_FOUND)
  SET(DirectShow_INCLUDE_DIRS)
ENDIF(DIRECTSHOW_FOUND)
MARK_AS_ADVANCED(DirectShow_INCLUDE_DIRS)
