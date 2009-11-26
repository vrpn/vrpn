CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
# 
# XXX clock_drift_estimator
# Move applications into subprojects in visual studio
# Make the files with ".c" extension also compile as C++.
# Make the c_interface_example work

#-----------------------------------------------------------------------------
# Libraries we need to do our thing.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VRPN_SOURCE_DIR})
FIND_PACKAGE(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES
FIND_PACKAGE(GLUT)

if (NOT WIN32)
	set (SYSTEM_LIBRARY pthread m)
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

#-----------------------------------------------------------------------------
# A little utility helper macro to setup default linkages and filenames
MACRO(CAPC_APPLICATION APPLICATION_NAME SOLUTION_NAME)
  SET(TARGET_NAME ${APPLICATION_NAME})
  ADD_EXECUTABLE(${APPLICATION_NAME} ${APPLICATION_NAME}.C)
  TARGET_LINK_LIBRARIES(${APPLICATION_NAME} vrpn ${QUATLIB_LIBRARY} ${SYSTEM_LIBRARY})
  SET_TARGET_PROPERTIES(${APPLICATION_NAME} PROPERTIES SOLUTION_FOLDER ${SOLUTION_NAME})
ENDMACRO(CAPC_APPLICATION)

MACRO(CPP_APPLICATION APPLICATION_NAME SOLUTION_NAME)
  SET(TARGET_NAME ${APPLICATION_NAME})
  ADD_EXECUTABLE(${APPLICATION_NAME} ${APPLICATION_NAME}.cpp)
  TARGET_LINK_LIBRARIES(${APPLICATION_NAME} vrpn ${QUATLIB_LIBRARY} ${SYSTEM_LIBRARY})
  SET_TARGET_PROPERTIES(${APPLICATION_NAME} PROPERTIES SOLUTION_FOLDER ${SOLUTION_NAME})
ENDMACRO(CPP_APPLICATION)

# Declaration of the actual test applications themselves
CAPC_APPLICATION(vrpn_print_devices clients)
CPP_APPLICATION(forcedevice_test_client tests)
CAPC_APPLICATION(printcereal tests)
CAPC_APPLICATION(ff_client tests)
CAPC_APPLICATION(phan_client tests)
CAPC_APPLICATION(sphere_client tests)
CAPC_APPLICATION(forwarderClient tests)
CAPC_APPLICATION(add_vrpn_cookie tests)
CAPC_APPLICATION(testSharedObject tests)
CAPC_APPLICATION(vrpn_ping tests)
CAPC_APPLICATION(test_mutex tests)
CAPC_APPLICATION(text tests)
CAPC_APPLICATION(test_imager tests)
IF(GLUT_INCLUDE_DIR)
	CAPC_APPLICATION(testimager_client tests)
ENDIF(GLUT_INCLUDE_DIR)
CAPC_APPLICATION(run_auxiliary_logger servers)
CAPC_APPLICATION(test_Zaber tests)
#CAPC_APPLICATION(ohm_client tests)
#CAPC_APPLICATION(sound_client tests)
#CAPC_APPLICATION(printvals tests)
#CAPC_APPLICATION(vrpn_print_messages clients)
#CAPC_APPLICATION(testLamport tests)
#CPP_APPLICATION(listen_client tests)
#CAPC_APPLICATION(bbox_client tests)
#CAPC_APPLICATION(checklogfile tests)

#-----------------------------------------------------------------------------
# Clients that need more than one input file, or special treatment.

# c_interface_example needs to be compiled with C++ to avoid linking errors.
# This doesn't work in Windows.
set(CMAKE_C_COMPILER ${CMAKE_CXX_COMPILER})
if (NOT WIN32)
	SET(TARGET_NAME c_interface_example)
	ADD_EXECUTABLE(c_interface_example c_interface_example.c c_interface.cpp)
	TARGET_LINK_LIBRARIES(c_interface_example vrpn ${QUATLIB_LIBRARY} ${SYSTEM_LIBRARY})
	SET_TARGET_PROPERTIES(c_interface_example PROPERTIES SOLUTION_FOLDER tests)
endif (NOT WIN32)
