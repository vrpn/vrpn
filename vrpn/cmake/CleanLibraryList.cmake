# - A smarter replacement for list(REMOVE_DUPLICATES) for library lists
#
#  clean_library_list(<listvar> [<additional list items>...]) - where
#  WHATEVER_LIBRARIES is the name of a variable, such as a variable being
#  created in a Find script.
#
# Removes duplicates from the list then sorts while preserving "optimized",
# "debug", and "general" labeling
#
# Requires CMake 2.6 or newer (uses the 'function' command)
#
# Original Author:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

function(clean_library_list _var)
	# combine variable's current value with additional list items
	set(_work ${${_var}} ${ARGN})
	if(_work)
		# Turn each of optimized, debug, and general into flags
		# prefixed on their respective library (combining list items)
		string(REGEX REPLACE "optimized;" "1CLL%O%" _work "${_work}")
		string(REGEX REPLACE "debug;" "1CLL%D%" _work "${_work}")
		string(REGEX REPLACE "general;" "1CLL%G%" _work "${_work}")

		# clean up list
		list(REMOVE_DUPLICATES _work)
		list(SORT _work)

		# Split list items back out again: turn prefixes into the
		# library type flags.
		string(REGEX REPLACE "1CLL%G%" "general;" _work "${_work}")
		string(REGEX REPLACE "1CLL%D%" "debug;" _work "${_work}")
		string(REGEX REPLACE "1CLL%O%" "optimized;" _work "${_work}")

		# Return _work
		set(${_var} ${_work} PARENT_SCOPE)
	endif()
endfunction()
