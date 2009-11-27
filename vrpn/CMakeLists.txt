CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(VRPN)

#-----------------------------------------------------------------------------
# XXX Things to make better.
# 
# Make Quatlib and VRPN into packages as described by Cory
# Put in hints or whatever to load the libraries when selected in Cmake configure?
# Move HDAPI libraries out of vrpn_Configure.h and into the Cmake realm (they seem there but may not be)
# Move DirectInput libraries out of vrpn_Configure.h and into the Cmake realm
# Move WiiUse libraries out of vrpn_Configure.h and into the Cmake realm
# Figure out how to handle vrpn_Configure.h without overwriting the source dir
# Move the shared-library code over to CMake's normal definition
# Move the version number into Cmake.
# Make a CPack installer.

#-----------------------------------------------------------------------------
# Compiler flags we got from Hans
if(WIN32) # MS-Windows, both 32 and 64 bits
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /fp:fast")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FIXED:NO")
elseif(APPLE) # Apple
#  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
#  set(CMAKE_VERBOSE_MAKEFILE ON)
elseif(UNIX) # other than Apple UNIXes
#  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -Wall")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
#  set(CMAKE_VERBOSE_MAKEFILE ON)
  # note : maybe move CMAKE_C_COMPILER part of vrpn/client_src/CMakeLists.txt here
endif()

#-----------------------------------------------------------------------------
# Libraries we need to do our thing.
set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${VRPN_SOURCE_DIR})
FIND_PACKAGE(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES

#-----------------------------------------------------------------------------
# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/vrpn_Configure.h.cmake_in"
  "${PROJECT_SOURCE_DIR}/vrpn_Configure.h"
  )

# Configuration options controlling what gets included in the build.
OPTION(VRPN_USE_PHANTOM_SERVER "Enable to include SensAble Phantom code in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_DIRECTINPUT "Enable to include Microsoft DirectInput code in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_DIRECTSHOW "Enable to include Microsoft DirectShow code in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_INCLUDE_TIMECODE_SERVER "Enable to include Adrienne time-code server code in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_INCLUDE_INTERSENSE "Enable to include InterSense native library code in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_NATIONAL_INSTRUMENTS "Enable to include National Instruments (old library) in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_NATIONAL_INSTRUMENTS_MX "Enable to include National Instruments (new library) in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_USDIGITAL "Enable to include US Digital SEI/A2 library in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_MICROSCRIBE "Enable to include MicroScribe3D library in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_INCLUDE_PHASESPACE "Enable to include PhaseSpace library in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_SHARED_LIBRARY "Enable to use DLLs on Windows (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_GPM_MOUSE "Enable to include GPM Linux mouse interface in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_MOTIONNODE "Enable to include GLI Interactive LLC MotionNode library in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_WIIUSE "Enable to include WiiUse library in build (see vrpn_Configure.h for more info)" OFF)
OPTION(VRPN_USE_FREESPACE "Enable to include Hillcrest Labs' Freespace devices in build (see vrpn_Configure.h for more info)" OFF)

#OPTION(vrpn_USE_MPI "Enable to include MPI in build (see vrpn_Configure.h for more info)" OFF)

#-----------------------------------------------------------------------------
# Options that control what gets built and how.

# Build a server version of the library
OPTION(VRPN_CLIENT_ONLY "Enable to build a library including only client code" OFF)

# Build various applications if we want them.
OPTION(BUILD_VRPN_CLIENTS "Enable to build VRPN clients" ON)
OPTION(BUILD_VRPN_SERVERS "Enable to build VRPN servers" ON)
OPTION(BUILD_TEST_RPC_GENERATION "Enable to build VRPN RPC generation" OFF)
OPTION(BUILD_DIRECTSHOW_VIDEO_SERVER "Enable to build DirectShow Video Server (Windows)" OFF)

#-----------------------------------------------------------------------------
# Build the library itself and declare what bits need to be installed

IF(VRPN_CLIENT_ONLY)
	ADD_LIBRARY(vrpn vrpn_Connection.C vrpn_Tracker.C vrpn_Button.C 
		vrpn_ForceDevice.C vrpn_Shared.C 
		vrpn_Analog.C vrpn_FileConnection.C 
		vrpn_FileController.C vrpn_Forwarder.C vrpn_Text.C 
		vrpn_ForwarderController.C vrpn_Serial.C vrpn_Dial.C 
		vrpn_SharedObject.C vrpn_BaseClass.C 
		vrpn_Sound.C vrpn_LamportClock.C vrpn_Mutex.C 
		vrpn_RedundantTransmission.C vrpn_Imager.C 
		vrpn_Analog_Output.C vrpn_Poser.C vrpn_Auxiliary_Logger.C

		vrpn_Configure.h
		vrpn_Connection.h vrpn_Tracker.h vrpn_Button.h
		vrpn_Sound.h vrpn_ForceDevice.h vrpn_Shared.h
		vrpn_Analog.h vrpn_FileConnection.h
		vrpn_FileController.h vrpn_Forwarder.h vrpn_Text.h
		vrpn_ForwarderController.h vrpn_Serial.h vrpn_Dial.h
		vrpn_SharedObject.h vrpn_LamportClock.h vrpn_Mutex.h
		vrpn_BaseClass.h vrpn_Imager.h
		vrpn_Analog_Output.h vrpn_Poser.h vrpn_Auxiliary_Logger.h
	)

ELSE(VRPN_CLIENT_ONLY)
	ADD_LIBRARY(vrpn vrpn_Connection.C vrpn_Tracker.C vrpn_Button.C 
		vrpn_ForceDevice.C vrpn_Shared.C 
		vrpn_Analog.C vrpn_FileConnection.C 
		vrpn_FileController.C vrpn_Forwarder.C vrpn_Text.C 
		vrpn_ForwarderController.C vrpn_Serial.C vrpn_Dial.C 
		vrpn_SharedObject.C vrpn_BaseClass.C 
		vrpn_Sound.C vrpn_LamportClock.C vrpn_Mutex.C 
		vrpn_RedundantTransmission.C vrpn_Imager.C 
		vrpn_Analog_Output.C vrpn_Poser.C vrpn_Auxiliary_Logger.C

		vrpn_3Space.C 
		vrpn_Flock.C vrpn_Tracker_Fastrak.C vrpn_Dyna.C 
		vrpn_Flock_Parallel.C  vrpn_UNC_Joystick.C 
		vrpn_JoyFly.C vrpn_sgibox.C vrpn_CerealBox.C 
		vrpn_Tracker_AnalogFly.C vrpn_raw_sgibox.C vrpn_Magellan.C 
		vrpn_Analog_Radamec_SPI.C vrpn_ImmersionBox.C vrpn_Wanda.C 
		vrpn_Analog_5dt.C vrpn_Joylin.C vrpn_Tng3.C vrpn_Spaceball.C 
		vrpn_Tracker_isense.C vrpn_Zaber.C vrpn_nikon_controls.C 
		vrpn_GlobalHapticsOrb.C vrpn_Tracker_ButtonFly.C vrpn_ADBox.C 
		vrpn_VPJoystick.C vrpn_Tracker_Liberty.C vrpn_NationalInstruments.C 
		vrpn_Poser_Analog.C vrpn_Tracker_DTrack.C vrpn_Poser_Tek4662.C 
		vrpn_Tracker_Crossbow.C vrpn_Tracker_3DMouse.C 
		vrpn_Mouse.C vrpn_3DMicroscribe.C vrpn_5DT16.C 
		vrpn_ForceDeviceServer.C vrpn_Keyboard.C 
		vrpn_Analog_USDigital_A2.C vrpn_Button_NI_DIO24.C 
		vrpn_Tracker_PhaseSpace.C 
		vrpn_Atmel.C vrpn_inertiamouse.C vrpn_Event.C vrpn_Event_Analog.C 
		vrpn_Event_Mouse.C vrpn_Imager_Stream_Buffer.C 
		vrpn_HumanInterface.C vrpn_Xkeys.C vrpn_3DConnexion.C 
		vrpn_Tracker_MotionNode.C vrpn_Tracker_NDI_Polaris.C 
		vrpn_WiiMote.C vrpn_Tracker_Isotrak.C vrpn_Freespace.C
		vrpn_Button_USB.cpp vrpn_Joywin32.C
		vrpn_DirectXFFJoystick.C vrpn_DirectXRumblePad.C
		vrpn_FunctionGenerator.C vrpn_XInputGamepad.C

		vrpn_Configure.h vrpn_Types.h
		vrpn_Connection.h vrpn_Tracker.h vrpn_Button.h
		vrpn_Sound.h vrpn_ForceDevice.h vrpn_Shared.h
		vrpn_Analog.h vrpn_FileConnection.h
		vrpn_FileController.h vrpn_Forwarder.h vrpn_Text.h
		vrpn_ForwarderController.h vrpn_Serial.h vrpn_Dial.h
		vrpn_SharedObject.h vrpn_LamportClock.h vrpn_Mutex.h
		vrpn_BaseClass.h vrpn_Imager.h
		vrpn_Analog_Output.h vrpn_Poser.h vrpn_Auxiliary_Logger.h

		vrpn_3Space.h vrpn_Flock.h vrpn_Tracker_Fastrak.h vrpn_Dyna.h
		vrpn_Flock_Parallel.h vrpn_UNC_Joystick.h
		vrpn_JoyFly.h vrpn_sgibox.h vrpn_raw_sgibox.h
		vrpn_CerealBox.h vrpn_Tracker_AnalogFly.h vrpn_Magellan.h
		vrpn_Analog_Radamec_SPI.h vrpn_ImmersionBox.h vrpn_Wanda.h
		vrpn_Analog_5dt.h vrpn_Joylin.h vrpn_Tng3.h vrpn_Spaceball.h
		vrpn_Tracker_isense.h vrpn_Zaber.h vrpn_nikon_controls.h
		vrpn_GlobalHapticsOrb.h vrpn_Tracker_ButtonFly.h vrpn_ADBox.h
		vrpn_VPJoystick.h vrpn_Tracker_Liberty.h vrpn_NationalInstruments.h
		vrpn_Poser_Analog.h vrpn_Tracker_DTrack.h vrpn_Poser.h
		vrpn_Poser_Tek4662.h vrpn_Tracker_Crossbow.h vrpn_Tracker_3DMouse.h
		vrpn_Mouse.h vrpn_3DMicroscribe.h vrpn_5DT16.h
		vrpn_ForceDeviceServer.h vrpn_Keyboard.h
		vrpn_Analog_USDigital_A2.h vrpn_Button_NI_DIO24.h
		vrpn_Tracker_PhaseSpace.h vrpn_Atmel.h
		vrpn_inertiamouse.h vrpn_Event.h vrpn_Event_Analog.h
		vrpn_Event_Mouse.h vrpn_Imager_Stream_Buffer.h
		vrpn_HumanInterface.h vrpn_Xkeys.h vrpn_3DConnexion.h
		vrpn_Tracker_MotionNode.h vrpn_Tracker_NDI_Polaris.h
		vrpn_WiiMote.h vrpn_Tracker_Isotrak.h vrpn_Freespace.h
		vrpn_Button_USB.h vrpn_Joywin32.h vrpn_Freespace.h
		vrpn_DirectXFFJoystick.h vrpn_DirectXRumblePad.h
		vrpn_FunctionGenerator.h vrpn_RedundantTransmission.h
		vrpn_XInputGamepad.h
	)

ENDIF(VRPN_CLIENT_ONLY)

INSTALL(TARGETS vrpn ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

INSTALL(FILES
		vrpn_Configure.h vrpn_Types.h
		vrpn_Connection.h vrpn_Tracker.h vrpn_Button.h
		vrpn_Sound.h vrpn_ForceDevice.h vrpn_Shared.h
		vrpn_Analog.h vrpn_FileConnection.h
		vrpn_FileController.h vrpn_Forwarder.h vrpn_Text.h
		vrpn_ForwarderController.h vrpn_Serial.h vrpn_Dial.h
		vrpn_SharedObject.h vrpn_LamportClock.h vrpn_Mutex.h
		vrpn_BaseClass.h vrpn_Imager.h
		vrpn_Analog_Output.h vrpn_Poser.h vrpn_Auxiliary_Logger.h

		vrpn_3Space.h vrpn_Flock.h vrpn_Tracker_Fastrak.h vrpn_Dyna.h
		vrpn_Flock_Parallel.h vrpn_UNC_Joystick.h
		vrpn_JoyFly.h vrpn_sgibox.h vrpn_raw_sgibox.h
		vrpn_CerealBox.h vrpn_Tracker_AnalogFly.h vrpn_Magellan.h
		vrpn_Analog_Radamec_SPI.h vrpn_ImmersionBox.h vrpn_Wanda.h
		vrpn_Analog_5dt.h vrpn_Joylin.h vrpn_Tng3.h vrpn_Spaceball.h
		vrpn_Tracker_isense.h vrpn_Zaber.h vrpn_nikon_controls.h
		vrpn_GlobalHapticsOrb.C vrpn_Tracker_ButtonFly.h vrpn_ADBox.h
		vrpn_VPJoystick.h vrpn_Tracker_Liberty.h vrpn_NationalInstruments.h
		vrpn_Poser_Analog.h vrpn_Tracker_DTrack.h vrpn_Poser.h
		vrpn_Poser_Tek4662.h vrpn_Tracker_Crossbow.h vrpn_Tracker_3DMouse.h
		vrpn_Mouse.h vrpn_3DMicroscribe.h vrpn_5DT16.h
		vrpn_ForceDeviceServer.h vrpn_Keyboard.h
		vrpn_Analog_USDigital_A2.h vrpn_Button_NI_DIO24.h
		vrpn_Tracker_PhaseSpace.h vrpn_Atmel.h
		vrpn_inertiamouse.h vrpn_Event.h vrpn_Event_Analog.h
		vrpn_Event_Mouse.h vrpn_Imager_Stream_Buffer.h
		vrpn_HumanInterface.h vrpn_Xkeys.h vrpn_3DConnexion.h
		vrpn_Tracker_MotionNode.h vrpn_Tracker_NDI_Polaris.h
		vrpn_WiiMote.h vrpn_Tracker_Isotrak.h vrpn_Freespace.h
		vrpn_Button_USB.h vrpn_Joywin32.h vrpn_Freespace.h
		vrpn_DirectXFFJoystick.h vrpn_DirectXRumblePad.h
		vrpn_FunctionGenerator.h
		vrpn_RedundantTransmission.h vrpn_Tracker_Crossbow.h
		vrpn_XInputGamepad.h
	DESTINATION ${CMAKE_INSTALL_PREFIX}/include
) 

# Include directory needed by all of the files
INCLUDE_DIRECTORIES(${VRPN_SOURCE_DIR} ${VRPN_SOURCE_DIR}/atmellib ${QUATLIB_INCLUDE_DIR})

#-----------------------------------------------------------------------------
# Build the client applications if we've been asked to.

IF(BUILD_VRPN_CLIENTS)
	add_subdirectory (client_src)
ENDIF(BUILD_VRPN_CLIENTS)

#-----------------------------------------------------------------------------
# Build the atmellib library if we're building a server library

IF(NOT VRPN_CLIENT_ONLY)
	add_subdirectory (atmellib)
ENDIF(NOT VRPN_CLIENT_ONLY)

#-----------------------------------------------------------------------------
# Build the server applications if we've been asked to and we didn't build
# the library client-only.

IF(BUILD_VRPN_SERVERS AND NOT VRPN_CLIENT_ONLY)
	add_subdirectory (server_src)
ENDIF(BUILD_VRPN_SERVERS AND NOT VRPN_CLIENT_ONLY)

#-----------------------------------------------------------------------------
# Build the RPC generation if we've been asked to

IF(BUILD_TEST_RPC_GENERATION)
	add_subdirectory (util/gen_rpc)
ENDIF(BUILD_TEST_RPC_GENERATION)

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
# Applications that live in the main directory.
IF(BUILD_VRPN_SERVERS AND NOT VRPN_CLIENT_ONLY)
	CPP_APPLICATION( time_test tests )
ENDIF(BUILD_VRPN_SERVERS AND NOT VRPN_CLIENT_ONLY)
