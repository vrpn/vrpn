# - A smarter replacement for list(REMOVE_DUPLICATES) for library lists
#
#  clean_library_list(WHATEVER_LIBRARIES) - where WHATEVER_LIBRARIES is
#  the name of a variable, such as a variable being created in a Find
#  script.
# 
# Removes duplicates from the list then sorts while preserving "optimized",
# "debug", and "general" labeling
#
# 2009 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

function (clean_library_list _instring)
	string (REGEX REPLACE "optimized;" "1CLL%O%" _WORK "${${_instring}}")
	string (REGEX REPLACE "debug;" "1CLL%D%" _WORK "${_WORK}")
	string (REGEX REPLACE "general;" "1CLL%G%" _WORK "${_WORK}")
	list(REMOVE_DUPLICATES _WORK)
	list(SORT _WORK)
	string (REGEX REPLACE "1CLL%G%" "general;" _WORK "${_WORK}")
	string (REGEX REPLACE "1CLL%D%" "debug;" _WORK "${_WORK}")
	string (REGEX REPLACE "1CLL%O%" "optimized;" _WORK "${_WORK}")
	set(${_instring} ${_WORK} PARENT_SCOPE)
endfunction ()
