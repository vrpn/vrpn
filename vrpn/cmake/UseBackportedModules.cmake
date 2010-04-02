# - Do a version-dependent check and auto-include backported modules dirs
# 
# Name your module directories cmake-*-modules where * is the full
# (maj.min.patch) version number that they came from.  You can use
# subdirectories within those directories, if you like - all directories
# inside of a cmake-*-modules dir for a newer version of CMake that what
# we're running, that contain one or more .cmake files, will be appended
# to the CMAKE_MODULE_PATH.
#
# When backporting modules, be sure to test them and follow copyright
# instructions (usually updating copyright notices)
#
# Original Author:
# 2009 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

if(NOT CMAKE_VERSION) # defined in >=2.6.3
	set(_cmver "${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}")
else()
	set(_cmver "${CMAKE_VERSION}")
endif()

get_filename_component(_moddir ${CMAKE_CURRENT_LIST_FILE} PATH)
file(GLOB _globbed "${_moddir}/cmake-*-modules")
foreach(_dir "${_globbed}")
	string(REGEX MATCH "cmake-[0-9].[0-9].[0-9]-modules" _dirname "${_dir}")
	string(REGEX REPLACE "cmake-([0-9].[0-9].[0-9])-modules" "\\1" _ver "${_dirname}")
	
	if("${_cmver}" VERSION_LESS "${_ver}")
		file(GLOB_RECURSE _modules "${_dir}/*.cmake")
		list(APPEND _bpmods "${_modules}")
	endif()
endforeach()

foreach(_mod "${_bpmods}")
	get_filename_component(_path "${_mod}" PATH)
	list(APPEND _bppaths "${_path}")
endforeach()

list(REMOVE_DUPLICATES _bppaths)
list(APPEND CMAKE_MODULE_PATH "${_bppaths}")

