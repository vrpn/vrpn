# Build the testapps if required
option(QUATLIB_BUILD_EXAMPLES "Enable to build quatlib examples" OFF)

if(QUATLIB_BUILD_EXAMPLES)
	# Include directory needed by all of the files
	include_directories(${quatlib_SOURCE_DIR})
	set(TESTAPPS
		eul
		matrix_to_posquat
		qmake
		qmult
		qxform
		timer)


	foreach(APP ${TESTAPPS})
		add_executable(quat_${APP} ${APP}.c)
		target_link_libraries(quat_${APP} quat)

		if(WIN32)	# MS-Windows, both 32 and 64 bits
			# nothing here yet
		elseif(APPLE)	# Apple
			target_link_libraries(quat_${APP} m)	# Math library
		elseif(UNIX)	# other UNIXes
			target_link_libraries(quat_${APP} m)	# Math library
		endif()

	endforeach()

endif()
