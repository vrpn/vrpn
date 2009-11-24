CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
# 
# Move applications into subprojects in visual studio
# XXX tracker_to_poser
# XXX daq_server
# Fix the ones that are commented out so that they compile and run?

#-----------------------------------------------------------------------------
# Libraries we need to do our thing.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VRPN_SOURCE_DIR})
FIND_PACKAGE(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES
if (NOT WIN32)
	set (PTHREAD_LIBRARY pthread)
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
		${PTHREAD_LIBRARY}
	)
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
	${PTHREAD_LIBRARY}
)

#-----------------------------------------------------------------------------
# A little utility helper macro to setup default linkages and filenames
MACRO(CAPC_APPLICATION APPLICATION_NAME)
  SET(TARGET_NAME ${APPLICATION_NAME})
  ADD_EXECUTABLE(${APPLICATION_NAME} ${APPLICATION_NAME}.C)
  TARGET_LINK_LIBRARIES(${APPLICATION_NAME} vrpn ${QUATLIB_LIBRARY} ${PTHREAD_LIBRARY})
ENDMACRO(CAPC_APPLICATION)

MACRO(CPP_APPLICATION APPLICATION_NAME)
  SET(TARGET_NAME ${APPLICATION_NAME})
  ADD_EXECUTABLE(${APPLICATION_NAME} ${APPLICATION_NAME}.cpp)
  TARGET_LINK_LIBRARIES(${APPLICATION_NAME} vrpn ${QUATLIB_LIBRARY} ${PTHREAD_LIBRARY})
ENDMACRO(CPP_APPLICATION)

#-----------------------------------------------------------------------------
# Declaration of the simple test applications that don't have extra files

CAPC_APPLICATION(client_and_server)
CAPC_APPLICATION(test_analogfly)
CAPC_APPLICATION(test_freespace)
CAPC_APPLICATION(test_radamec_spi)
CAPC_APPLICATION(test_logging)
CAPC_APPLICATION(test_rumble)
CPP_APPLICATION(testimager_server)
CAPC_APPLICATION(test_auxiliary_logger)
CAPC_APPLICATION(test_vrpn)
CAPC_APPLICATION(test_peerMutex)
#CAPC_APPLICATION(forward)
#CAPC_APPLICATION(last_of_sequence)
#CAPC_APPLICATION(testSharedObject)
#CAPC_APPLICATION(text)
#CAPC_APPLICATION(sample_server)
#CAPC_APPLICATION(sample_analog)
