# Changes made by:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

# XXX TODO Install these bindings, don't just build them!

if(SWIG_FOUND AND PYTHONLIBS_FOUND AND VRPN_BUILD_PYTHON)
	include(${SWIG_USE_FILE})
	include_directories(${PYTHON_INCLUDE_DIRS})

	if(VRPN_BUILD_CLIENT_LIBRARY)
		set(VRPN_CLIENT_LIBRARY vrpn)
	elseif(VRPN_BUILD_SERVER_LIBRARY)
		set(VRPN_CLIENT_LIBRARY vrpnserver)
	else()
		message(FATAL_ERROR
			"Cannot compile Python wrapper without either the client or full library.  Enable one and try again.")
	endif()

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
		swig_add_module(${module} Python ${module}.i)
		swig_link_libraries(${module} ${PYTHON_LIBRARIES} ${VRPN_CLIENT_LIBRARY})
	endforeach()

endif()
