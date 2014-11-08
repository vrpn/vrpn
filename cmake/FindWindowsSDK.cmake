# - Find the Windows SDK aka Platform SDK
#
# Relevant Wikipedia article: http://en.wikipedia.org/wiki/Microsoft_Windows_SDK
#
# Variables:
#  WINDOWSSDK_FOUND - if any version of the windows or platform SDK was found that is usable with the current version of visual studio
#  WINDOWSSDK_LATEST_DIR
#  WINDOWSSDK_LATEST_NAME
#  WINDOWSSDK_FOUND_PREFERENCE - if we found an entry indicating a "preferred" SDK listed for this visual studio version
#  WINDOWSSDK_PREFERRED_DIR
#  WINDOWSSDK_PREFERRED_NAME
#
#  WINDOWSSDK_DIRS - contains no duplicates, ordered most recent first.
#  WINDOWSSDK_PREFERRED_FIRST_DIRS - contains no duplicates, ordered with preferred first, followed by the rest in descending recency
#
# Functions:
#  windowssdk_name_lookup(<directory> <output variable>) - Find the name corresponding with the SDK directory you pass in, or
#     NOTFOUND if not recognized. Your directory must be one of WINDOWSSDK_DIRS for this to work.
#
# Requires these CMake modules:
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Original Author:
# 2012 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
#
# Copyright Iowa State University 2012.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

set(_preferred_sdk_dirs)
set(_win_sdk_dirs)
set(_win_sdk_versanddirs)
if(MSVC_VERSION GREATER 1310) # Newer than VS .NET/VS Toolkit 2003

	# Environment variable for SDK dir
	if(EXISTS "$ENV{WindowsSDKDir}" AND (NOT "$ENV{WindowsSDKDir}" STREQUAL ""))
		message(STATUS "Got $ENV{WindowsSDKDir} - Windows/Platform SDK directories: ${_win_sdk_dirs}")
		list(APPEND _preferred_sdk_dirs "$ENV{WindowsSDKDir}")
	endif()

	if(MSVC_VERSION LESS 1600)
		# Per-user current Windows SDK for VS2005/2008
		get_filename_component(_sdkdir
			"[HKEY_CURRENT_USER\\Software\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]"
			ABSOLUTE)
		if(EXISTS "${_sdkdir}")
			list(APPEND _preferred_sdk_dirs "${_sdkdir}")
		endif()

		# System-wide current Windows SDK for VS2005/2008
		get_filename_component(_sdkdir
			"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows;CurrentInstallFolder]"
			ABSOLUTE)
		if(EXISTS "${_sdkdir}")
			list(APPEND _preferred_sdk_dirs "${_sdkdir}")
		endif()
	endif()

	if(MSVC_VERSION LESS 1700)
		# VC 10 and older has broad target support
		set(_winsdk_vistaonly)
	else()
		# VC 11 by default targets Vista and later only, so we can add a few more SDKs that (might?) only work on vista+
		if("${CMAKE_VS_PLATFORM_TOOLSET}" MATCHES "_xp")
			# This is the XP-compatible v110 toolset
		elseif("${CMAKE_VS_PLATFORM_TOOLSET}" STREQUAL "v100")
			# This is the VS2010 toolset
		else()
			message(STATUS "FindWindowsSDK: Detected Visual Studio 2012 or newer, not using the _xp toolset variant: including SDK versions that drop XP support in search!")
			# These versions have no XP (and possibly Vista pre-SP1) support
			set(_winsdk_vistaonly
				# Windows Software Development Kit (SDK) for Windows 8.1
				# http://msdn.microsoft.com/en-gb/windows/desktop/bg162891
				v8.1

				# Included in Visual Studio 2012
				v8.0A

				# Microsoft Windows SDK for Windows 8 and .NET Framework 4.5
				# This is the first version to also include the DirectX SDK
				# http://msdn.microsoft.com/en-US/windows/desktop/hh852363.aspx
				v8.0

				# Microsoft Windows SDK for Windows 7 and .NET Framework 4
				# http://www.microsoft.com/downloads/en/details.aspx?FamilyID=6b6c21d2-2006-4afa-9702-529fa782d63b
				v7.1
				)
		endif()
	endif()
	foreach(_winsdkver
		${_winsdk_vistaonly}

		# Included in Visual Studio 2013
		# Includes the v120_xp toolset
		v8.1A

		# Included with VS 2012 Update 1 or later
		# Introduces v110_xp toolset
		v7.1A

		# Included with VS 2010
		v7.0A

		# Windows SDK for Windows 7 and .NET Framework 3.5 SP1
		# Works with VC9
		#http://www.microsoft.com/en-us/download/details.aspx?id=18950
		v7.0

		# Two versions call themselves "v6.1":
		# Older:
		# Windows Vista Update & .NET 3.0 SDK
		# http://www.microsoft.com/en-us/download/details.aspx?id=14477

		# Newer:
		# Windows Server 2008 & .NET 3.5 SDK
		# may have broken VS9SP1? they recommend v7.0 instead, or a KB...
		# http://www.microsoft.com/en-us/download/details.aspx?id=24826
		v6.1

		# Included in VS 2008
		v6.0A

		# Microsoft Windows Software Development Kit for Windows Vista and .NET Framework 3.0 Runtime Components
		# http://blogs.msdn.com/b/stanley/archive/2006/11/08/microsoft-windows-software-development-kit-for-windows-vista-and-net-framework-3-0-runtime-components.aspx
		v6.0)

		get_filename_component(_sdkdir
			"[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\${_winsdkver};InstallationFolder]"
			ABSOLUTE)
		if(EXISTS "${_sdkdir}")
			list(APPEND _win_sdk_dirs "${_sdkdir}")
			list(APPEND
				_win_sdk_versanddirs
				"Windows SDK ${_winsdkver}"
				"${_sdkdir}")
		endif()
	endforeach()
endif()
if(MSVC_VERSION GREATER 1200)
	foreach(_platformsdkinfo
		"D2FF9F89-8AA2-4373-8A31-C838BF4DBBE1_Microsoft Platform SDK for Windows Server 2003 R2"
		"8F9E5EF3-A9A5-491B-A889-C58EFFECE8B3_Microsoft Platform SDK for Windows Server 2003 SP1")
		string(SUBSTRING "${_platformsdkinfo}" 0 36 _platformsdkguid)
		string(SUBSTRING "${_platformsdkinfo}" 37 -1 _platformsdkname)
		foreach(HIVE HKEY_LOCAL_MACHINE HKEY_CURRENT_USER)
			get_filename_component(_sdkdir
				"[${HIVE}\\SOFTWARE\\Microsoft\\MicrosoftSDK\\InstalledSDKs\\${_platformsdkguid};Install Dir]"
				ABSOLUTE)
			if(EXISTS "${_sdkdir}")
				list(APPEND _win_sdk_dirs "${_sdkdir}")
				list(APPEND _win_sdk_versanddirs "${_platformsdkname}" "${_sdkdir}")
			endif()
		endforeach()
	endforeach()
endif()

set(_win_sdk_versanddirs
	"${_win_sdk_versanddirs}"
	CACHE
	INTERNAL
	"mapping between windows sdk version locations and names"
	FORCE)

function(windowssdk_name_lookup _dir _outvar)
	list(FIND _win_sdk_versanddirs "${_dir}" _diridx)
	math(EXPR _nameidx "${_diridx} - 1")
	if(${_nameidx} GREATER -1)
		list(GET _win_sdk_versanddirs ${_nameidx} _sdkname)
	else()
		set(_sdkname "NOTFOUND")
	endif()
	set(${_outvar} "${_sdkname}" PARENT_SCOPE)
endfunction()

if(_win_sdk_dirs)
	# Remove duplicates
	list(REMOVE_DUPLICATES _win_sdk_dirs)
	list(GET _win_sdk_dirs 0 WINDOWSSDK_LATEST_DIR)
	windowssdk_name_lookup("${WINDOWSSDK_LATEST_DIR}"
		WINDOWSSDK_LATEST_NAME)
	set(WINDOWSSDK_DIRS ${_win_sdk_dirs})
endif()
if(_preferred_sdk_dirs)
	list(GET _preferred_sdk_dirs 0 WINDOWSSDK_PREFERRED_DIR)
	windowssdk_name_lookup("${WINDOWSSDK_LATEST_DIR}"
		WINDOWSSDK_PREFERRED_NAME)
	set(WINDOWSSDK_PREFERRED_FIRST_DIRS
		${_preferred_sdk_dirs}
		${_win_sdk_dirs})
	list(REMOVE_DUPLICATES WINDOWSSDK_PREFERRED_FIRST_DIRS)
	set(WINDOWSSDK_FOUND_PREFERENCE ON)

	# In case a preferred dir was found that isn't found otherwise
	#set(WINDOWSSDK_DIRS ${WINDOWSSDK_DIRS} ${WINDOWSSDK_PREFERRED_FIRST_DIRS})
	#list(REMOVE_DUPLICATES WINDOWSSDK_DIRS)
else()
	set(WINDOWSSDK_PREFERRED_DIR "${WINDOWSSDK_LATEST_DIR}")
	set(WINDOWSSDK_PREFERRED_NAME "${WINDOWSSDK_LATEST_NAME}")
	set(WINDOWSSDK_PREFERRED_FIRST_DIRS ${WINDOWSSDK_DIRS})
	set(WINDOWSSDK_FOUND_PREFERENCE OFF)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WindowsSDK
	"No compatible version of the Windows SDK or Platform SDK found."
	WINDOWSSDK_DIRS)

if(WINDOWSSDK_FOUND)
	if(NOT _winsdk_remembered_dirs STREQUAL WINDOWSSDK_DIRS)
		set(_winsdk_remembered_dirs
			"${WINDOWSSDK_DIRS}"
			CACHE
			INTERNAL
			""
			FORCE)
		if(NOT WindowsSDK_FIND_QUIETLY)
			foreach(_sdkdir ${WINDOWSSDK_DIRS})
				windowssdk_name_lookup("${_sdkdir}" _sdkname)
				message(STATUS " - Found ${_sdkname} at ${_sdkdir}")
			endforeach()
		endif()
	endif()
endif()
