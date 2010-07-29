# - Removes duplicate entries and non-directories from a provided list
#
# Original Author:
# 2009 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

function(clean_directory_list var)
	set(_in ${ARGN})
	list(SORT _in)
	list(REMOVE_DUPLICATES _in)
	set(_out)
	foreach(_dir ${_in})
		if(IS_DIRECTORY "${_dir}")
			get_filename_component(_dir "${_dir}" ABSOLUTE)
			list(APPEND _out "${_dir}")
		endif()
	endforeach()
	list(SORT _out)
	list(REMOVE_DUPLICATES _out)
	set(${var} "${_out}" PARENT_SCOPE)
endfunction()
