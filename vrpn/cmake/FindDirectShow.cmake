# - Find Microsoft DirectShow sample files, library, and headers.
#
#  DirectShow_INCLUDE_DIRS - where to find needed include file
#  DirectShow_BASECLASS_DIR- Directory containing the DirectShow baseclass sample code.
#  DIRECTSHOW_FOUND        - True if DirectShow found.

# Look for one of the sample files.
find_path(DirectShow_BASECLASS_DIR
	NAMES
	streams.h
	PATHS
	"C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2/Samples/Multimedia/DirectShow/BaseClasses")
mark_as_advanced(DirectShow_BASECLASS_DIR)

# Look for the header files (picking any one from the directories we need).
find_path(PLATFORM_SDK_INCLUDE_DIR
	NAMES
	AclAPI.h
	PATHS
	"C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2/Include")
mark_as_advanced(PLATFORM_SDK_INCLUDE_DIR)
find_path(PLATFORM_SDK_ATL_INCLUDE_DIR
	NAMES
	atlbase.h
	PATHS
	"C:/Program Files/Microsoft Platform SDK for Windows Server 2003 R2/Include/atl")
mark_as_advanced(PLATFORM_SDK_ATL_INCLUDE_DIR)
find_path(DIRECTX_SDK_INCLUDE_DIR
	NAMES
	comdecl.h
	PATHS
	"C:/Program Files/Microsoft DirectX SDK (August 2006)/Include")
mark_as_advanced(DIRECTX_SDK_INCLUDE_DIR)

# handle the QUIETLY and REQUIRED arguments and set DirectShow_FOUND to TRUE if
# all listed variables are TRUE.  The package name seems to need to be all-caps for this
# to work.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DIRECTSHOW
	DEFAULT_MSG
	PLATFORM_SDK_INCLUDE_DIR
	DirectShow_BASECLASS_DIR)

if(DIRECTSHOW_FOUND)
	set(DirectShow_INCLUDE_DIRS
		${PLATFORM_SDK_INCLUDE_DIR}
		${PLATFORM_SDK_ATL_INCLUDE_DIR}
		${DirectShow_BASECLASS_DIR}
		${DirectShow_BASECLASS_DIR}
		${DIRECTX_SDK_INCLUDE_DIR})
else()
	set(DirectShow_INCLUDE_DIRS)
endif()
mark_as_advanced(DirectShow_INCLUDE_DIRS)
