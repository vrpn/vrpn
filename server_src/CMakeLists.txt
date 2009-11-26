CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
# 
# Move applications into subprojects in visual studio
# XXX tracker_to_poser
# XXX daq_server
# XXX vrpn_hid_device_watcher
# Fix the ones that are commented out so that they compile and run?

#-----------------------------------------------------------------------------
# Libraries we need to do our thing.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VRPN_SOURCE_DIR})
FIND_PACKAGE(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES
if (NOT WIN32)
	set (SYSTEM_LIBRARY pthread m)
	set (VRPN_ATMEL_LIBRARY vrpn_atmel)
endif (NOT WIN32)

#-----------------------------------------------------------------------------
# Phantom library (if configured)
# XXX Does not work with GHOST library
IF (VRPN_USE_PHANTOM_SERVER)
	FIND_PACKAGE(HDAPI REQUIRED)
	ADD_LIBRARY (vrpn_phantom
		ghostEffects/InstantBuzzEffect.h ghostEffects/InstantBuzzEffect.cpp
		buzzForceField.h buzzForceField.C
		constraint.h constraint.C
		forcefield.h forcefield.C
		ghost.h
		plane.h plane.C
		texture_plane.h texture_plane.C
		trimesh.h trimesh.C
		vrpn_Phantom.h vrpn_Phantom.C
	)
	INSTALL (TARGETS vrpn_phantom
		ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
	)
	INSTALL(FILES
		ghostEffects/InstantBuzzEffect.h
		buzzForceField.h constraint.h forcefield.h ghost.h
		plane.h trimesh.h vrpn_Phantom.h
		DESTINATION ${CMAKE_INSTALL_PREFIX}/include	
	)
	set (PHANTOM_LIBS vrpn_phantom ${HDAPI_LIBRARIES})
ENDIF (VRPN_USE_PHANTOM_SERVER)

#-----------------------------------------------------------------------------
# DirectShow base-class library (if configured)
if (BUILD_DIRECTSHOW_VIDEO_SERVER)
	FIND_PACKAGE(DirectShow REQUIRED)	# Need to have DirectShow SDK
	ADD_DEFINITIONS( -DCOINIT_DISABLE_OLE1DDE=0x4 )
	ADD_LIBRARY (DirectShow_baseclasses
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
		${DirectShow_BASECLASS_DIR}/wxutil.cpp
	)
	set (DirectShow_LIBS DirectShow_baseclasses)

	SET(TARGET_NAME directshow_video_server)
	ADD_EXECUTABLE(directshow_video_server
		directshow_video_server/directx_video_imager_server.cpp
		directshow_video_server/directx_camera_server.cpp
		directshow_video_server/directx_camera_server.h
	)
	TARGET_LINK_LIBRARIES(directshow_video_server
		vrpn
		${DirectShow_LIBS}
		${SYSTEM_LIBRARY}
	)
	SET_TARGET_PROPERTIES(directshow_video_server PROPERTIES SOLUTION_FOLDER servers)
endif (BUILD_DIRECTSHOW_VIDEO_SERVER)

add_subdirectory (timecode_generator_server)

#-----------------------------------------------------------------------------
# Include directories needed
INCLUDE_DIRECTORIES(
	${VRPN_SOURCE_DIR}
	${QUATLIB_INCLUDE_DIR}
	${VRPN_SOURCE_DIR}/server_src
	${VRPN_SOURCE_DIR}/server_src/timecode_generator_server
	${VRPN_SOURCE_DIR}/server_src/ghostEffects
	${HDAPI_INCLUDE_DIR} ${HDAPI_HDU_INCLUDE_DIR}
	${DirectShow_BASECLASS_DIR} ${PLATFORM_SDK_INCLUDE_DIR}
		${PLATFORM_SDK_ATL_INCLUDE_DIR}
		${PLATFORM_SDK_ATL_INCLUDE_DIR} ${DIRECTX_SDK_INCLUDE_DIR}
)

#-----------------------------------------------------------------------------
# Applications that need more than one file to run.
SET(TARGET_NAME vrpn_server)
ADD_EXECUTABLE(vrpn_server vrpn.C vrpn_Generic_server_object.C
	vrpn_Generic_server_object.h)
TARGET_LINK_LIBRARIES(vrpn_server
	vrpn vrpn_timecode_generator
	${VRPN_ATMEL_LIBRARY}
	${PHANTOM_LIBS}
	${QUATLIB_LIBRARY}
	${SYSTEM_LIBRARY}
)
SET_TARGET_PROPERTIES(vrpn_server PROPERTIES SOLUTION_FOLDER servers)
install(TARGETS vrpn_server RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

#-----------------------------------------------------------------------------
# Install the vrpn.cfg file needed by the server.
INSTALL(FILES ${VRPN_SOURCE_DIR}/server_src/vrpn.cfg DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

#-----------------------------------------------------------------------------
# A little utility helper macro to setup default linkages and filenames
MACRO(CAPC_APPLICATION APPLICATION_NAME SOLUTION_NAME)
  SET(TARGET_NAME ${APPLICATION_NAME})
  ADD_EXECUTABLE(${APPLICATION_NAME} ${APPLICATION_NAME}.C)
  TARGET_LINK_LIBRARIES(${APPLICATION_NAME} vrpn ${QUATLIB_LIBRARY} ${SYSTEM_LIBRARY})
  SET_TARGET_PROPERTIES(${APPLICATION_NAME} PROPERTIES SOLUTION_FOLDER ${SOLUTION_NAME})
  install(TARGETS ${APPLICATION_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
ENDMACRO(CAPC_APPLICATION)

MACRO(CPP_APPLICATION APPLICATION_NAME SOLUTION_NAME)
  SET(TARGET_NAME ${APPLICATION_NAME})
  ADD_EXECUTABLE(${APPLICATION_NAME} ${APPLICATION_NAME}.cpp)
  TARGET_LINK_LIBRARIES(${APPLICATION_NAME} vrpn ${QUATLIB_LIBRARY} ${SYSTEM_LIBRARY})
  SET_TARGET_PROPERTIES(${APPLICATION_NAME} PROPERTIES SOLUTION_FOLDER ${SOLUTION_NAME})
  install(TARGETS ${APPLICATION_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
ENDMACRO(CPP_APPLICATION)

#-----------------------------------------------------------------------------
# Declaration of the simple test applications that don't have extra files

CAPC_APPLICATION(client_and_server tests)
CAPC_APPLICATION(test_analogfly tests)
CAPC_APPLICATION(test_freespace tests)
CAPC_APPLICATION(test_radamec_spi tests)
CAPC_APPLICATION(test_logging tests)
CAPC_APPLICATION(test_rumble tests)
CPP_APPLICATION(testimager_server tests)
CAPC_APPLICATION(test_auxiliary_logger tests)
CAPC_APPLICATION(test_vrpn tests)
CAPC_APPLICATION(test_peerMutex tests)
#CAPC_APPLICATION(forward tests)
#CAPC_APPLICATION(last_of_sequence tests)
#CAPC_APPLICATION(testSharedObject tests)
#CAPC_APPLICATION(text tests)
#CAPC_APPLICATION(sample_server tests)
#CAPC_APPLICATION(sample_analog tests)
