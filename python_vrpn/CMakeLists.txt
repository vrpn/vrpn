# Changes made by:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC

if(SWIG_FOUND AND PYTHONLIBS_FOUND AND VRPN_BUILD_PYTHON)
	include(${SWIG_USE_FILE})
	include_directories(${PYTHON_INCLUDE_DIRS})

	set(_client_lib_src
		${VRPN_CLIENT_SOURCES}
		${VRPN_CLIENT_PUBLIC_HEADERS})
	set(LIBS ${EXTRA_LIBS})

	set(clientlib_fullsrc)
	foreach(_src ${_client_lib_src})
		if(IS_ABSOLUTE "${_src}")
			list(APPEND clientlib_fullsrc "${_src}")
		else()
			list(APPEND clientlib_fullsrc "${PROJECT_SOURCE_DIR}/${_src}")
		endif()
	endforeach()
	source_group("VRPN Client Library Sources" FILES ${clientlib_fullsrc})

	set(quatlib_fullsrc)
	get_property(_quat_src TARGET quat PROPERTY SOURCES)
	foreach(_src ${_quat_src})
		list(APPEND quatlib_fullsrc "${PROJECT_SOURCE_DIR}/quat/${_src}")
	endforeach()
	source_group("Quat Library Sources" FILES ${quatlib_fullsrc})

	#	XXX TODO: add dependency on these files - perhaps change extension to .h?
	#
	#	set(COMMON
	#		client-only.i
	#		python-callback-wrapper.i)
	#	set_source_files_properties(${COMMON} PROPERTIES CPLUSPLUS ON)

	set(PYTHONVERSIONDIR python)
	if(PYTHON_LIBRARY MATCHES "python([23].[0-9])")
		set(PYTHONVERSIONDIR "python${CMAKE_MATCH_1}/")
	endif()

	#Make sure Python wrapper files get correctly generated in Windows
	if(WIN32 AND NOT CYGWIN)
		#Define the preprocessor symbol VRPN_USE_WINSOCK_SOCKETS in swig, since the latter doesn't take the preprocessor definitions from vrpn_Shared.h in account
		set(CMAKE_SWIG_FLAGS "-DVRPN_USE_WINSOCK_SOCKETS")
	endif()

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
		swig_add_module(${module}
			Python
			${module}.i
			${clientlib_fullsrc}
			${quatlib_fullsrc})
		swig_link_libraries(${module} ${PYTHON_LIBRARIES} ${LIBS} ${SERVER_EXTRA_LIBS})
		set_target_properties(_${module} PROPERTIES FOLDER "Python Bindings")

		if(VRPN_INSTALL)
			install(FILES
				"${CMAKE_CURRENT_BINARY_DIR}/${module}.py"
				DESTINATION
				lib/${PYTHONVERSIONDIR}dist-packages/
				COMPONENT
				python)
			install(TARGETS
				${SWIG_MODULE_${module}_REAL_NAME}
				RUNTIME
				DESTINATION
				lib/${PYTHONVERSIONDIR}dist-packages/
				COMPONENT
				python
				LIBRARY
				DESTINATION
				lib/${PYTHONVERSIONDIR}dist-packages/
				COMPONENT
				python)
		endif()
	endforeach()

endif()
