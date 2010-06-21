# Changes made by:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

# XXX TODO Install these bindings, don't just build them!

if(SWIG_FOUND AND PYTHONLIBS_FOUND AND VRPN_BUILD_PYTHON)
	include(${SWIG_USE_FILE})
	include_directories(${PYTHON_INCLUDE_DIRS})

	set(_client_lib_src ${VRPN_CLIENT_SOURCES} ${VRPN_CLIENT_PUBLIC_HEADERS})
	set(LIBS ${EXTRA_LIBS})
	
	set(libsrc)
	foreach(_src ${_client_lib_src})
		if(IS_ABSOLUTE "${_src}")
			list(APPEND libsrc "${_src}")
		else()
			list(APPEND libsrc "../${_src}")
		endif()
	endforeach()
	
	get_property(_quat_src TARGET quat PROPERTY SOURCES)
	foreach(_src ${_quat_src})
		list(APPEND libsrc "../../quat/${_src}")
	endforeach()
	

#	XXX TODO: add dependency on these files - perhaps change extension to .h?
#
#	set(COMMON
#		client-only.i
#		python-callback-wrapper.i)
#	set_source_files_properties(${COMMON} PROPERTIES CPLUSPLUS ON)

	

	set(MODULES
		vrpn_Analog
		vrpn_Analog_Output
		vrpn_Auxiliary_Logger
		vrpn_Button
		vrpn_Dial
		vrpn_ForceDevice
		vrpn_Poser
		vrpn_Text
		vrpn_Tracker)
	foreach(module ${MODULES})
		set_source_files_properties(${module}.i PROPERTIES CPLUSPLUS ON)
		swig_add_module(${module} Python ${module}.i ${libsrc})
		swig_link_libraries(${module} ${PYTHON_LIBRARIES} ${LIBS})
	endforeach()

endif()
