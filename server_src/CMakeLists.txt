cmake_minimum_required(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
#
# Move applications into subprojects in visual studio
# XXX tracker_to_poser
# XXX daq_server
# Fix the ones that are commented out so that they compile and run?

if(NOT VRPN_BUILD_SERVER_LIBRARY)
	message(FATAL_ERROR
		"Cannot compile servers without the full (server) library.  Enable it and try again.")
endif()

set(VRPN_SERVER_LIBRARY vrpnserver)
set(VRPN_GPSNMEA_LIBRARY gpsnmea)

if(NOT WIN32)
	set(VRPN_ATMEL_LIBRARY vrpn_atmel)
endif()

#-----------------------------------------------------------------------------
# Phantom library (if configured)
# XXX Does not work with GHOST library
find_package(OpenGL)
if(VRPN_USE_PHANTOM_SERVER AND OPENHAPTICS_FOUND AND OPENGL_FOUND)
	set(PHANTOM_SOURCES
		ghostEffects/InstantBuzzEffect.cpp
		buzzForceField.C
		constraint.C
		forcefield.C
		plane.C
		texture_plane.C
		trimesh.C
		vrpn_Phantom.C)

	set(PHANTOM_HEADERS
		ghostEffects/InstantBuzzEffect.h
		buzzForceField.h
		constraint.h
		forcefield.h
		ghost.h
		plane.h
		texture_plane.h
		trimesh.h
		vrpn_Phantom.h)
	include_directories(${OPENHAPTICS_INCLUDE_DIRS})
	include_directories(${VRPN_SOURCE_DIR}/server_src/ghostEffects)
	add_library(vrpn_phantom ${PHANTOM_SOURCES} ${PHANTOM_HEADERS})
	target_link_libraries(vrpn_phantom vrpnserver ${OPENGL_LIBRARIES} ${OPENHAPTICS_LIBRARIES})

	set_property(TARGET
		vrpn_phantom
		PROPERTY
		PUBLIC_HEADER
		${PHANTOM_HEADERS})
	set_target_properties(vrpn_phantom PROPERTIES FOLDER Library)

	install(TARGETS
		vrpn_phantom
		ARCHIVE
		DESTINATION
		lib
		COMPONENT serversdk
		PUBLIC_HEADER
		DESTINATION
		include
		COMPONENT serversdk)
	set(VRPNPHANTOMLIB vrpn_phantom)
endif()

#-----------------------------------------------------------------------------
# DirectShow base-class library (if configured)
if(VRPN_BUILD_DIRECTSHOW_VIDEO_SERVER AND DIRECTSHOW_FOUND)
	# Add this if you can't compile.  Otherwise, don't
	# add_definitions(-DCOINIT_DISABLE_OLE1DDE=0x4)
	add_library(DirectShow_baseclasses
		${DirectShow_BASECLASS_DIR}/amextra.cpp
		${DirectShow_BASECLASS_DIR}/amfilter.cpp
		${DirectShow_BASECLASS_DIR}/amvideo.cpp
		${DirectShow_BASECLASS_DIR}/combase.cpp
		${DirectShow_BASECLASS_DIR}/cprop.cpp
		${DirectShow_BASECLASS_DIR}/ctlutil.cpp
		${DirectShow_BASECLASS_DIR}/ddmm.cpp
		${DirectShow_BASECLASS_DIR}/dllentry.cpp
		${DirectShow_BASECLASS_DIR}/dllsetup.cpp
		${DirectShow_BASECLASS_DIR}/mtype.cpp
		${DirectShow_BASECLASS_DIR}/outputq.cpp
		${DirectShow_BASECLASS_DIR}/pstream.cpp
		${DirectShow_BASECLASS_DIR}/pullpin.cpp
		${DirectShow_BASECLASS_DIR}/refclock.cpp
		${DirectShow_BASECLASS_DIR}/renbase.cpp
		${DirectShow_BASECLASS_DIR}/schedule.cpp
		${DirectShow_BASECLASS_DIR}/seekpt.cpp
		${DirectShow_BASECLASS_DIR}/source.cpp
		${DirectShow_BASECLASS_DIR}/strmctl.cpp
		${DirectShow_BASECLASS_DIR}/sysclock.cpp
		${DirectShow_BASECLASS_DIR}/transfrm.cpp
		${DirectShow_BASECLASS_DIR}/transip.cpp
		${DirectShow_BASECLASS_DIR}/videoctl.cpp
		${DirectShow_BASECLASS_DIR}/vtrans.cpp
		${DirectShow_BASECLASS_DIR}/winctrl.cpp
		${DirectShow_BASECLASS_DIR}/winutil.cpp
		${DirectShow_BASECLASS_DIR}/wxdebug.cpp
		${DirectShow_BASECLASS_DIR}/wxlist.cpp
		${DirectShow_BASECLASS_DIR}/wxutil.cpp)
	set(DirectShow_LIBS DirectShow_baseclasses)

	include_directories(${DirectShow_INCLUDE_DIRS})
	add_executable(directshow_video_server
		directshow_video_server/directx_video_imager_server.cpp
		directshow_video_server/directx_camera_server.cpp
		directshow_video_server/directx_camera_server.h)
	target_link_libraries(directshow_video_server
		${VRPN_SERVER_LIBRARY}
		${DirectShow_LIBS}
		${DIRECTSHOW_LIBRARIES})
	set_target_properties(directshow_video_server
		PROPERTIES
		FOLDER
		Servers)
	install(TARGETS directshow_video_server RUNTIME DESTINATION bin COMPONENT servers)
endif()

add_subdirectory(timecode_generator_server)

#-----------------------------------------------------------------------------
# Include directories needed
include_directories(${VRPN_SOURCE_DIR}/server_src
	${VRPN_SOURCE_DIR}/server_src/timecode_generator_server)

#-----------------------------------------------------------------------------
# Applications that need more than one file to run.
add_executable(vrpn_server
	vrpn.cfg
	vrpn.C)
target_link_libraries(vrpn_server
	${VRPN_SERVER_LIBRARY}
	vrpn_timecode_generator
	${VRPN_ATMEL_LIBRARY}
	${VRPN_GPSNMEA_LIBRARY}
	${VRPNPHANTOMLIB})
set_target_properties(vrpn_server PROPERTIES SOLUTION_FOLDER servers)
set_property(TARGET
	vrpn_server
	PROPERTY
	PROJECT_LABEL
	"vrpn_server")
set_target_properties(vrpn_server PROPERTIES FOLDER "Main Server")
install(TARGETS vrpn_server
	RUNTIME DESTINATION bin
	COMPONENT mainserver)

#-----------------------------------------------------------------------------
# Install the vrpn.cfg file needed by the server.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/vrpn.cfg" "${CMAKE_CURRENT_BINARY_DIR}/vrpn.cfg" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/vrpn.cfg"
	DESTINATION etc
	COMPONENT mainserver)


#-----------------------------------------------------------------------------
# Declaration of the simple test applications that don't have extra files

# XXX Are these files compiled somewhere?
#NIUtil.cpp
#NIUtil.h
#clock_server.C
#daq_server.C
#directXFF.C
#make_vrpn_libs_build.cpp
#nidaqClient.cpp
#nidaqServer.cpp



set(SRV_TEST_SOURCES
	client_and_server.C
	#forward.C
	#last_of_sequence.C
	#sample_analog.C
	#sample_server.C
	#testSharedObject.C
	test_analogfly.C
	test_auxiliary_logger.C
	test_freespace.C
	test_logging.C
	#test_mutex.C
	test_peerMutex.C
	test_radamec_spi.C
	test_rumble.C
	test_vrpn.C
	testimager_server.cpp
	#text.C
	vrpn_HID_device_watcher.cpp
)

set(SRV_SERVER_SOURCES
	wiimote_head_tracker.C
)


###
# Tests
###
if(BUILD_TESTING)
	foreach(SOURCE ${SRV_TEST_SOURCES})
		get_filename_component(APP ${SOURCE} NAME_WE)
		add_executable(${APP} ${SOURCE})
		target_link_libraries(${APP} ${VRPN_SERVER_LIBRARY})
		set_target_properties(${APP} PROPERTIES FOLDER Tests)
		set_target_properties(${APP} PROPERTIES HAS_CXX yes)
		install(TARGETS ${APP}
			RUNTIME DESTINATION bin COMPONENT tests)
	endforeach()
	add_test(test_vrpn test_vrpn)
endif()

###
# Servers
###
foreach(SOURCE ${SRV_SERVER_SOURCES})
	get_filename_component(APP ${SOURCE} NAME_WE)
	add_executable(${APP} ${SOURCE})
	target_link_libraries(${APP} ${VRPN_SERVER_LIBRARY})
	set_target_properties(${APP} PROPERTIES FOLDER Servers)
	set_target_properties(${APP} PROPERTIES HAS_CXX yes)
	install(TARGETS ${APP} RUNTIME DESTINATION bin COMPONENT servers)
endforeach()

###
# Install extra files
###

# Wiimote jconf files for fishtank VR
set(VRPN_WIIMOTE_JCONFS
	vrpn.wiimoteheadtracker.fishtankvr.1280x1024.jconf
	vrpn.wiimoteheadtracker.fishtankvr.1024x768.jconf
	vrpn.wiimoteheadtracker.fishtankvr.800x600.jconf)

install(FILES
	${VRPN_WIIMOTE_JCONFS}
	DESTINATION
	share/vrpn-${BRIEF_VERSION}/vrjuggler-2.2-config
	COMPONENT servers)

