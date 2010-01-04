# - Removes duplicate entries and non-directories from a provided list
#
# Original Author:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

function(clean_directory_list var)
	set(_in ${ARGN})
	if(_in)
		list(SORT _in)
		list(REMOVE_DUPLICATES _in)
	endif()
	set(_out)
	foreach(_dir ${_in})
		if(IS_DIRECTORY "${_dir}")
			get_filename_component(_dir "${_dir}" ABSOLUTE)
			list(APPEND _out "${_dir}")
		endif()
	endforeach()
	if(_out)
		list(SORT _out)
		list(REMOVE_DUPLICATES _out)
	endif()
	set(${var} "${_out}" PARENT_SCOPE)
endfunction()
