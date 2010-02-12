# - Add flags to compile with extra warnings
#
#  enable_extra_compiler_warnings(<targetname>)
#
# Requires these CMake modules:
#  CleanDirectoryList
#
# Original Author:
# 2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

if(__enable_extra_compiler_warnings)
	return()
endif()
set(__enable_extra_compiler_warnings YES)

function(enable_extra_compiler_warnings _target)
	set(_flags)
	if(MSVC)
		set(_flags /W4)
	elseif(CMAKE_COMPILER_IS_GNUCXX)
		set(_flags "-W -Wall")
	endif()
	get_target_property(_origflags ${_target} COMPILE_FLAGS)
	if(_origflags)
		set_property(TARGET
			${_target}
			PROPERTY
			COMPILE_FLAGS
			"${_flags} ${_origflags}")
	else()
		set_property(TARGET
			${_target}
			PROPERTY
			COMPILE_FLAGS
			"${_flags}")
	endif()

endfunction()
