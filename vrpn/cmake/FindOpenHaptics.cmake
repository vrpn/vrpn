# - try to find OpenHaptics libraries
#
# Cache Variables: (probably not for direct use in your scripts)
#  HDAPI_INCLUDE_DIR
#  HDAPI_LIBRARY
#  HDAPI_HDU_INCLUDE_DIR
#  HDAPI_HDU_LIBRARY
#  HDAPI_HDU_LIBRARY_RELEASE
#  HDAPI_HDU_LIBRARY_DEBUG
#  HLAPI_INCLUDE_DIR
#  HLAPI_LIBRARY
#  HLAPI_HLU_INCLUDE_DIR
#  HLAPI_HLU_LIBRARY
#  HLAPI_HLU_LIBRARY_RELEASE
#  HLAPI_HLU_LIBRARY_DEBUG
#
# Non-cache variables you might use in your CMakeLists.txt:
#  OPENHAPTICS_FOUND
#  HDAPI_INCLUDE_DIRS
#  HDAPI_LIBRARIES
#  HDAPI_HDU_INCLUDE_DIRS
#  HDAPI_HDU_LIBRARIES
#  HLAPI_INCLUDE_DIRS
#  HLAPI_LIBRARIES
#  HLAPI_HLU_INCLUDE_DIRS
#  HLAPI_HLU_LIBRARIES
#  OPENHAPTICS_LIBRARIES - includes HD, HDU, HL, HLU
#  OPENHAPTICS_LIBRARY_DIRS
#  OPENHAPTICS_INCLUDE_DIRS
#  OPENHAPTICS_MARK_AS_ADVANCED - whether to mark our vars as advanced even
#    if we don't find this library.
#
# Requires these CMake modules:
#  CleanDirectoryList
#  ListCombinations
#  ProgramFilesGlob
#  SelectLibraryConfigurations (included with CMake >=2.8.0)
#  FindPackageHandleStandardArgs (known included with CMake >=2.6.2)
#
# Original Author:
# 2009 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

if(HDAPI_LIBRARY AND HLAPI_LIBRARY)
	# in cache already
	set(OPENHAPTICS_FIND_QUIETLY TRUE)
endif()

###
# Configure OpenHaptics
###

include(SelectLibraryConfigurations)
include(ListCombinations)

set(_incsearchdirs)
set(_libsearchdirs)

if(WIN32)
	include(ProgramFilesGlob)
	program_files_glob(_dirs "/SensAble/3DTouch*/")
		
	if(CMAKE_SIZEOF_VOID_P MATCHES "8")
		# 64-bit
		list_combinations(_libsearch PREFIXES "${_dirs}" SUFFIXES "/lib/x64")
		list_combinations(_libsearch2 PREFIXES "${_dirs}" SUFFIXES "/utilities/lib/x64")
	else()
		# 32-bit
		list_combinations(_libsearch PREFIXES "${_dirs}" SUFFIXES "/lib/win32" "/lib")
		list_combinations(_libsearch2 PREFIXES "${_dirs}" SUFFIXES "/utilities/lib/Win32" "/utilities/lib")
	endif()
	clean_directory_list(_libsearchdirs ${_libsearch} ${_libsearch2})

	list_combinations(_incsearch PREFIXES "${_dirs}" SUFFIXES "/include")
	list_combinations(_incsearch2 PREFIXES "${_dirs}" SUFFIXES "/utilities/include")
	clean_directory_list(_incsearchdirs ${_incsearch} ${_incsearch2})
endif()


###
# HDAPI: HD
###
find_path(HDAPI_INCLUDE_DIR
	NAMES HD/hd.h
	HINTS ${_incsearchdirs})

find_library(HDAPI_LIBRARY
	NAMES hd
	HINTS ${_libsearchdirs})

###
# HDAPI: HDU
###
find_path(HDAPI_HDU_INCLUDE_DIR
	NAMES HDU/hdu.h
	HINTS ${_incsearchdirs})

find_library(HDAPI_HDU_LIBRARY_RELEASE
	NAMES hdu
	PATH_SUFFIXES ReleaseAcademicEdition Release
	HINTS ${_libsearchdirs})

find_library(HDAPI_HDU_LIBRARY_DEBUG
	NAMES hdu
	PATH_SUFFIXES DebugAcademicEdition Debug
	HINTS ${_libsearchdirs})

# Fallback
find_library(HDAPI_HDU_LIBRARY_DEBUG
	NAMES hdud
	PATH_SUFFIXES DebugAcademicEdition Debug)

select_library_configurations(HDAPI_HDU)


###
# HLAPI: HL
###
find_path(HLAPI_INCLUDE_DIR
	NAMES HL/hl.h
	HINTS ${_incsearchdirs})

find_library(HLAPI_LIBRARY
	NAMES hl
	HINTS ${_libsearchdirs})


###
# HLAPI: HLU
###
find_path(HLAPI_HLU_INCLUDE_DIR
	NAMES HLU/hlu.h
	HINTS ${_incsearchdirs})

find_library(HLAPI_HLU_LIBRARY_RELEASE
	NAMES hlu
	PATH_SUFFIXES ReleaseAcademicEdition Release
	HINTS ${_libsearchdirs})

find_library(HLAPI_HLU_LIBRARY_DEBUG
	NAMES hlu
	PATH_SUFFIXES DebugAcademicEdition Debug
	HINTS ${_libsearchdirs})

# fallback
find_library(HLAPI_HLU_LIBRARY_DEBUG
	NAMES hlud
	PATH_SUFFIXES DebugAcademicEdition Debug)

select_library_configurations(HLAPI_HLU)


###
# Add dependencies: Libraries
###
set(HDAPI_LIBRARIES "${HDAPI_LIBRARY}")

if(HDAPI_HDU_LIBRARIES AND HDAPI_LIBRARIES)
	list(APPEND HDAPI_HDU_LIBRARIES ${HDAPI_LIBRARIES})
else()
	set(HDAPI_HDU_LIBRARIES)
endif()

if(HLAPI_LIBRARY AND HDAPI_LIBRARIES)
	set(HLAPI_LIBRARIES ${HLAPI_LIBRARY} ${HDAPI_LIBRARIES})
else()
	set(HLAPI_LIBRARIES)
endif()

if(HLAPI_HLU_LIBRARIES AND HLAPI_LIBRARIES)
	list(APPEND HLAPI_HLU_LIBRARIES ${HLAPI_LIBRARIES})
else()
	set(HLAPI_HLU_LIBRARIES)
endif()

###
# Add dependencies: Include dirs
###
if(HDAPI_INCLUDE_DIR)
	set(HDAPI_INCLUDE_DIRS ${HDAPI_INCLUDE_DIR})

	if(HDAPI_HDU_INCLUDE_DIR)
		set(HDAPI_HDU_INCLUDE_DIRS ${HDAPI_INCLUDE_DIRS} ${HDAPI_HDU_INCLUDE_DIR})

		if(HDAPI_HDU_INCLUDE_DIR)
			set(HLAPI_INCLUDE_DIRS ${HDAPI_INCLUDE_DIRS} ${HLAPI_INCLUDE_DIR})

			if(HLAPI_HLU_INCLUDE_DIR)
				set(HLAPI_HLU_INCLUDE_DIRS ${HLAPI_INCLUDE_DIRS} ${HLAPI_HLU_INCLUDE_DIR})

			endif()
		endif()
	endif()
endif()



# handle the QUIETLY and REQUIRED arguments and set xxx_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenHaptics DEFAULT_MSG
	HDAPI_INCLUDE_DIR
	HDAPI_LIBRARY
	HDAPI_HDU_INCLUDE_DIR
	HDAPI_HDU_LIBRARY
	HLAPI_INCLUDE_DIR
	HLAPI_LIBRARY
	HLAPI_HLU_INCLUDE_DIR
	HLAPI_HLU_LIBRARY)

if(OPENHAPTICS_FOUND)
	set(OPENHAPTICS_LIBRARIES ${HDAPI_LIBRARY} ${HDAPI_HDU_LIBRARY} ${HLAPI_LIBRARY} ${HLAPI_HLU_LIBRARY})
	set(OPENHAPTICS_LIBRARY_DIRS)
	foreach(_lib
		HDAPI_LIBRARY
		HDAPI_HDU_LIBRARY_RELEASE HDAPI_HDU_LIBRARY_DEBUG
		HLAPI_LIBRARY
		HLAPI_HLU_LIBRARY_RELEASE HLAPI_HLU_LIBRARY_DEBUG)
		get_filename_component(_libdir ${${_lib}} PATH)
		list(APPEND OPENHAPTICS_LIBRARY_DIRS ${_libdir})
	endforeach()

	include(CleanDirectoryList)
	clean_directory_list(OPENHAPTICS_LIBRARY_DIRS ${OPENHAPTICS_LIBRARY_DIRS})

	clean_directory_list(OPENHAPTICS_INCLUDE_DIRS ${HLAPI_HLU_INCLUDE_DIRS} ${HDAPI_HDU_INCLUDE_DIRS})

	include(CleanLibraryList)
	clean_library_list(OPENHAPTICS_LIBRARIES)
endif()

if(OPENHAPTICS_FOUND OR OPENHAPTICS_MARK_AS_ADVANCED)
	foreach(_cachevar
		HDAPI_INCLUDE_DIR
		HDAPI_LIBRARY
		HDAPI_HDU_INCLUDE_DIR
		HDAPI_HDU_LIBRARY_RELEASE
		HDAPI_HDU_LIBRARY_DEBUG
		HLAPI_INCLUDE_DIR
		HLAPI_LIBRARY
		HLAPI_HLU_INCLUDE_DIR
		HLAPI_HLU_LIBRARY_RELEASE
		HLAPI_HLU_LIBRARY_DEBUG)

		mark_as_advanced(${_cachevar})
	endforeach()
endif()
