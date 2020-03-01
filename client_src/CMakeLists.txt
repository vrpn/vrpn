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

	if(NOT VRPN_CLIENT_LIBRARY)
		message(FATAL_ERROR
			"Cannot compile clients without either the client or full library.  Enable one and try again.")
	endif()

	#-----------------------------------------------------------------------------
	# Libraries we need to do our thing.
	find_package(GLUT)
	if(APPLE)
		mark_as_advanced(GLUT_cocoa_LIBRARY)
	endif()
	find_package(OpenGL)

	set(CLIENTS_SOURCES
	vrpn_average_analogs.C
		vrpn_print_devices.C
		vrpn_log_devices.C
		vrpn_print_performance.C
		vrpn_print_messages.C
	)

	set(TEST_SOURCES
		add_vrpn_cookie.C
		bdbox_client.C
		clock_drift_estimator.C
		ff_client.C
		forcedevice_test_client.cpp
		forwarderClient.C
		#midi_client.C # XXX TODO No vrpn_Sound_Remote ever defined in this repository
		#ohm_client.C # XXX TODO No vrpn_Ohmmeter (vrpn_Ohmmeter.h) defined in this repository
		phan_client.C
		printcereal.C
		printvals.C
		sound_client.C
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
		# TODO: These need "open" calls modified to build on win.
		list(APPEND
			TEST_SOURCES
			logfilesenders.C
			logfiletypes.C
			checklogfile.C)
	endif()

	###
	# Tests
	###
	if(BUILD_TESTING)
		foreach(SOURCE ${TEST_SOURCES})
			get_filename_component(APP ${SOURCE} NAME_WE)
			add_executable(${APP} ${SOURCE})
			target_link_libraries(${APP} ${VRPN_CLIENT_LIBRARY})

			set_target_properties(${APP} PROPERTIES FOLDER Tests)
			if(VRPN_INSTALL)
				install(TARGETS ${APP} RUNTIME DESTINATION bin COMPONENT tests)
			endif()
		endforeach()

    add_test(test_imager test_imager)

		if(GLUT_FOUND AND OPENGL_FOUND)

			option(VRPN_GLUT_IS_STATIC_FREEGLUT
				"Is the GLUT used a static build of FreeGLUT?"
				NO)
			mark_as_advanced(VRPN_GLUT_IS_STATIC_FREEGLUT)

			include_directories(${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
			add_executable(testimager_client testimager_client.C)
			target_link_libraries(testimager_client
				${VRPN_CLIENT_LIBRARY}
				${GLUT_glut_LIBRARY}
				${OPENGL_LIBRARIES})
			set_target_properties(testimager_client PROPERTIES FOLDER Tests)

			if(VRPN_INSTALL)
				install(TARGETS
					testimager_client
					RUNTIME
					DESTINATION
					bin
					COMPONENT
					tests)
			endif()

			if(VRPN_GLUT_IS_STATIC_FREEGLUT)
				set_property(TARGET
					testimager_client
					PROPERTY
					COMPILE_DEFINITIONS
					"FREEGLUT_STATIC")
			endif()

			include_directories(${GLUT_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR})
			add_executable(vrpn_orientation vrpn_orientation.C)
			target_link_libraries(vrpn_orientation
				${VRPN_CLIENT_LIBRARY}
				${GLUT_glut_LIBRARY}
				${OPENGL_LIBRARIES})
			set_target_properties(vrpn_orientation PROPERTIES FOLDER Clients)

			if(VRPN_INSTALL)
				install(TARGETS
					vrpn_orientation
					RUNTIME
					DESTINATION
					bin
					COMPONENT
					clients)
			endif()

			if(VRPN_GLUT_IS_STATIC_FREEGLUT)
				set_property(TARGET
					vrpn_orientation
					PROPERTY
					COMPILE_DEFINITIONS
					"FREEGLUT_STATIC")
			endif()

		endif()

	endif()

	add_executable(c_interface_example
		c_interface_example.c
		c_interface.h
		c_interface.cpp)
	target_link_libraries(c_interface_example ${VRPN_CLIENT_LIBRARY})

	set_target_properties(c_interface_example PROPERTIES FOLDER Tests)

	if(VRPN_INSTALL)
		install(TARGETS
			c_interface_example
			RUNTIME
			DESTINATION
			bin
			COMPONENT
			tests)
	endif()


	###
	# Clients
	###
	foreach(SOURCE ${CLIENTS_SOURCES})
		get_filename_component(APP ${SOURCE} NAME_WE)
		add_executable(${APP} ${SOURCE})
		target_link_libraries(${APP} ${VRPN_CLIENT_LIBRARY})

		set_target_properties(${APP} PROPERTIES FOLDER Clients)

		if(VRPN_INSTALL)
			install(TARGETS ${APP} RUNTIME DESTINATION bin COMPONENT clients)
		endif()
	endforeach()

	###
	# Servers
	###
	add_executable(run_auxiliary_logger run_auxiliary_logger.C)
	target_link_libraries(run_auxiliary_logger ${VRPN_CLIENT_LIBRARY})
	set_target_properties(run_auxiliary_logger PROPERTIES FOLDER Servers)
	if(VRPN_INSTALL)
		install(TARGETS
			run_auxiliary_logger
			RUNTIME
			DESTINATION
			bin
			COMPONENT
			servers)
	endif()

endif()
