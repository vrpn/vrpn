CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
# 
# Move applications into subprojects in visual studio
# Make the files with ".c" extension also compile as C++.
# Make the c_interface_example work

#-----------------------------------------------------------------------------
# Libraries we need to do our thing.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VRPN_SOURCE_DIR})
FIND_PACKAGE(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES
FIND_PACKAGE(GLUT)

if (NOT WIN32)
	set (PTHREAD_LIBRARY pthread)
endif (NOT WIN32)

# Keep GLUT from generating a lot of errors if it is not found.
if (GLUT_INCLUDE_DIR)
	set (GLUT_INC ${GLUT_INCLUDE_DIR})
	set (GLUT_LIB ${GLUT_glut_LIBRARY})
else (GLUT_INCLUDE_DIR)
	set (GLUT_INC "")
	set (GLUT_LIB "")
endif (GLUT_INCLUDE_DIR)

# Include directory needed by all of the files
INCLUDE_DIRECTORIES(${VRPN_SOURCE_DIR} ${QUATLIB_INCLUDE_DIR} ${GLUT_INC})

# A little utility helper macro to setup default linkages and filenames
MACRO(CAPC_APPLICATION APPLICATION_NAME)
  SET(TARGET_NAME ${APPLICATION_NAME})
  ADD_EXECUTABLE(${APPLICATION_NAME} ${APPLICATION_NAME}.C)
  TARGET_LINK_LIBRARIES(${APPLICATION_NAME} vrpn ${QUATLIB_LIBRARY} ${GLUT_LIB} ${PTHREAD_LIBRARY})
ENDMACRO(CAPC_APPLICATION)

MACRO(CPP_APPLICATION APPLICATION_NAME)
  SET(TARGET_NAME ${APPLICATION_NAME})
  ADD_EXECUTABLE(${APPLICATION_NAME} ${APPLICATION_NAME}.cpp)
  TARGET_LINK_LIBRARIES(${APPLICATION_NAME} vrpn ${QUATLIB_LIBRARY} ${GLUT_LIB} ${PTHREAD_LIBRARY})
ENDMACRO(CPP_APPLICATION)

# Declaration of the actual test applications themselves
CAPC_APPLICATION(vrpn_print_devices)
CPP_APPLICATION(forcedevice_test_client)
CAPC_APPLICATION(printcereal)
CAPC_APPLICATION(ff_client)
CAPC_APPLICATION(phan_client)
CAPC_APPLICATION(sphere_client)
CAPC_APPLICATION(forwarderClient)
CAPC_APPLICATION(add_vrpn_cookie)
CAPC_APPLICATION(testSharedObject)
CAPC_APPLICATION(vrpn_ping)
CAPC_APPLICATION(test_mutex)
CAPC_APPLICATION(text)
CAPC_APPLICATION(test_imager)
IF(GLUT_INCLUDE_DIR)
	CAPC_APPLICATION(testimager_client)
ENDIF(GLUT_INCLUDE_DIR)
CAPC_APPLICATION(run_auxiliary_logger)
CAPC_APPLICATION(test_Zaber)
#CAPC_APPLICATION(c_interface_example)
#CAPC_APPLICATION(ohm_client)
#CAPC_APPLICATION(sound_client)
#CAPC_APPLICATION(printvals)
#CAPC_APPLICATION(vrpn_print_messages)
#CAPC_APPLICATION(testLamport)
#CPP_APPLICATION(listen_client)
#CAPC_APPLICATION(bbox_client)
#CAPC_APPLICATION(checklogfile)
