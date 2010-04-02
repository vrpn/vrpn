cmake_minimum_required(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
#
# XXX clock_drift_estimator
# Move applications into subprojects in visual studio
# Make the files with ".c" extension also compile as C++.
# Make the c_interface_example work

#-----------------------------------------------------------------------------
# Build the client applications if we've been asked to.
if(VRPN_BUILD_CLIENTS)

	if(VRPN_BUILD_CLIENT_LIBRARY)
		set(VRPN_CLIENT_LIBRARY vrpn)
	elseif(VRPN_BUILD_SERVER_LIBRARY)
		set(VRPN_CLIENT_LIBRARY vrpnserver)
	else()
		message(FATAL_ERROR "Cannot compile clients without either the client or full library.  Enable one and try again.")
	endif()

	#-----------------------------------------------------------------------------
	# Libraries we need to do our thing.
	find_package(GLUT)
	if(APPLE)
		mark_as_advanced(GLUT_cocoa_LIBRARY)
	endif()
	find_package(OpenGL)

	set(CLIENTS_SOURCES
		vrpn_print_devices.C
		#vrpn_print_messages.C
	)

	set(TEST_SOURCES
		add_vrpn_cookie.C
		bdbox_client.C
		clock_drift_estimator.C
		ff_client.C
		forcedevice_test_client.cpp
		forwarderClient.C
		#midi_client.c # cannot find type vrpn_Sound_Remote
		#ohm_client.C
		phan_client.C
		printcereal.C
		printvals.c
		#sound_client.c # cannot find type vrpn_Sound_Remote
		sphere_client.C
		testSharedObject.C
		test_Zaber.C
		test_imager.C
		test_mutex.C
		text.C
		tracker_to_poser.cpp
		vrpn_LamportClock.t.C
		vrpn_ping.C
	)
	
	if(NOT WIN32)
		# These need "open" calls modified to build on win.
		list(APPEND TEST_SOURCES
			logfilesenders.c
			logfiletypes.c
			checklogfile.c
		)
	endif()

	###
	# Tests
	###
	foreach(SOURCE ${TEST_SOURCES})
		get_filename_component(APP ${SOURCE} NAME_WE)
		add_executable(${APP} ${SOURCE})
		target_link_libraries(${APP} ${VRPN_CLIENT_LIBRARY})

		set_target_properties(${APP} PROPERTIES SOLUTION_FOLDER tests)
		# Force C compilation and linking
		set_property(SOURCE ${SOURCE} PROPERTY LANGUAGE CXX)
		set_target_properties(${APP} PROPERTIES HAS_CXX yes)

		install(TARGETS ${APP}
			RUNTIME DESTINATION bin)
	endforeach()

	if(GLUT_FOUND AND OPENGL_FOUND)
		include_directories(${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
		add_executable(testimager_client testimager_client.C)
		target_link_libraries(testimager_client ${VRPN_CLIENT_LIBRARY} ${GLUT_LIBRARIES} ${OPENGL_LIBRARIES})

		set_target_properties(testimager_client PROPERTIES SOLUTION_FOLDER tests)

		install(TARGETS testimager_client
			RUNTIME DESTINATION bin)
	endif()

	add_executable(c_interface_example c_interface_example.c c_interface.cpp)
	target_link_libraries(c_interface_example ${VRPN_CLIENT_LIBRARY})

	set_target_properties(c_interface_example PROPERTIES SOLUTION_FOLDER tests)
	# Force C compilation and linking
	set_property(SOURCE c_interface_example.c PROPERTY LANGUAGE CXX)
	set_target_properties(c_interface_example PROPERTIES HAS_CXX yes)

	install(TARGETS c_interface_example RUNTIME DESTINATION bin)


	###
	# Clients
	###
	foreach(SOURCE ${CLIENTS_SOURCES})
		get_filename_component(APP ${SOURCE} NAME_WE)
		add_executable(${APP} ${SOURCE})
		target_link_libraries(${APP} ${VRPN_CLIENT_LIBRARY})

		set_target_properties(${APP} PROPERTIES SOLUTION_FOLDER clients)
		# Force C compilation and linking
		set_property(SOURCE ${SOURCE} PROPERTY LANGUAGE CXX)
		set_target_properties(${APP} PROPERTIES HAS_CXX yes)

		install(TARGETS ${APP}
			RUNTIME DESTINATION bin)
	endforeach()

	###
	# Servers
	###
	add_executable(run_auxiliary_logger run_auxiliary_logger.C)
	target_link_libraries(run_auxiliary_logger ${VRPN_CLIENT_LIBRARY})
	set_target_properties(run_auxiliary_logger PROPERTIES SOLUTION_FOLDER servers)
	install(TARGETS run_auxiliary_logger
		RUNTIME DESTINATION bin)

endif()
