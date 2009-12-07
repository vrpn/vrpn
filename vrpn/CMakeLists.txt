cmake_minimum_required(VERSION 2.6)
project(VRPN)

#-----------------------------------------------------------------------------
# XXX Things to make better.
#
# Make Quatlib and VRPN into packages as described by Cory
# Move DirectInput libraries out of vrpn_Configure.h and into the Cmake realm
# Move the shared-library code over to CMake's normal definition
# Move the version number into Cmake.
# Make a CPack installer.

#-----------------------------------------------------------------------------
# Compiler flags we got from Hans
if(WIN32) # MS-Windows, both 32 and 64 bits
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /fp:fast")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FIXED:NO")
endif()

#-----------------------------------------------------------------------------
# Libraries we need to do our thing.

###
# Local CMake Modules
###
list(APPEND CMAKE_MODULE_PATH ${VRPN_SOURCE_DIR}/cmake)
if("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" VERSION_LESS "2.8.0")
	# Use backported modules if we're earlier than the cmake version above
	list(APPEND CMAKE_MODULE_PATH ${VRPN_SOURCE_DIR}/cmake/cmake-2.8.0-features)
endif()

###
# Quatlib
###

# If we haven't been called by a parent CMakeLists.txt but there's a
# quat/ dir next to us, make it part of our build anyway
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../quat AND NOT TARGET quat)
	message(STATUS "Found quatlib source in ../quat - adding to build")
	add_subdirectory(../quat # source directory
					quatlib/) # relative path for build output
endif()

find_package(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES

###
# Threading
###
find_package(Threads)

###
# Mac HID
###
if(APPLE AND VRPN_BUILD_SERVER_LIBRARY)
	find_package(MacHID REQUIRED)
	if(MACHID_FOUND)
		list(APPEND EXTRA_LIBS ${MACHID_LIBRARIES})
		add_definitions(${MACHID_DEFINITIONS})
	endif()
endif()

###
# MPI
###
find_package(MPI)
if(MPI_FOUND AND NOT DEFINED vrpn_USE_MPI)
	option(vrpn_USE_MPI "Build with MPI support" ON)
endif()

###
# OpenHaptics HDAPI/HLAPI
###
find_package(OpenHaptics)
if(OPENHAPTICS_FOUND AND NOT DEFINED VRPN_USE_PHANTOM_SERVER)
	option(VRPN_USE_PHANTOM_SERVER "Build with SensAble Phantom support" ON)
endif()

###
# WiiUse
###
find_package(WiiUse)
if(WIIUSE_FOUND AND VRPN_GPL_SERVER AND NOT DEFINED VRPN_USE_WIIUSE)
	option(VRPN_USE_WIIUSE "Build with WiiUse library support - makes servers GPL!" ON)
endif()


#-----------------------------------------------------------------------------
# Set up build for requested libraries.

###
# Quatlib
###
list(APPEND EXTRA_LIBS ${QUATLIB_LIBRARIES})

###
# Threading (not on win32?)
###
if(NOT WIN32)
	list(APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

###
# Mac HID
###
if(MACHID_FOUND)
	list(APPEND SERVER_LIBS ${MACHID_LIBRARIES})
	add_definitions(${MACHID_DEFINITIONS})
endif()

###
# MPI
###
if(vrpn_USE_MPI)
	if(NOT MPI_FOUND)
		message(FATAL_ERROR "ERROR: vrpn_USE_MPI set but MPI not found!")
	endif()
	add_definitions(${MPI_COMPILE_FLAGS})
	include_directories(${MPI_INCLUDE_PATH})
	list(APPEND EXTRA_LIBS ${MPI_LIBRARIES})
endif()

###
# OpenHaptics HDAPI/HLAPI
###
if(VRPN_USE_PHANTOM_SERVER)
	if(NOT OPENHAPTICS_FOUND)
		message(FATAL_ERROR "ERROR: VRPN_USE_PHANTOM_SERVER set but HDAPI/HLAPI not found!")
	endif()
	if(VRPN_BUILD_SERVER_LIBRARY AND VRPN_GPL_SERVER)
		message(SEND_ERROR "ERROR: Building with VRPN_USE_PHANTOM_SERVER and VRPN_GPL_SERVER may produce a legally non-distributable binary!")
	endif()
	include_directories(${OPENHAPTICS_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${OPENHAPTICS_LIBRARIES})
endif()

###
# WiiUse
###
if(VRPN_USE_WIIUSE)
	if(NOT WIIUSE_FOUND)
		message(FATAL_ERROR "ERROR: VRPN_USE_WIIUSE set but WIIUSE not found!")
	endif()
	if(VRPN_BUILD_SERVER_LIBRARY AND NOT VRPN_GPL_SERVER)
		message(FATAL_ERROR "ERROR: VRPN_USE_WIIUSE produces a GPL server library - set VRPN_GPL_SERVER to acknowledge this and build.")
	endif()
	include_directories(${WIIUSE_INCLUDE_DIRS})
	list(APPEND SERVER_EXTRA_LIBS ${WIIUSE_LIBRARIES})
endif()

#-----------------------------------------------------------------------------
# configure a header file to pass some of the CMake settings
# to the source code
configure_file(
  "${PROJECT_SOURCE_DIR}/vrpn_Configure.h.cmake_in"
  "${PROJECT_BINARY_DIR}/vrpn_Configure.h"
)

# Force use of our CMake-processed configuration header before the stock one.
if(CMAKE_COMPILER_IS_GNUCXX)
	add_definitions("-include \"${PROJECT_BINARY_DIR}/vrpn_Configure.h\"")
elseif(MSVC)
	add_definitions("/FI \"${PROJECT_BINARY_DIR}/vrpn_Configure.h\"")
endif()

# Configuration options controlling what gets included in the build.
option(VRPN_USE_PHANTOM_SERVER "Build with SensAble Phantom support" OFF)
option(VRPN_USE_DIRECTINPUT "Build with Microsoft DirectInput support" OFF)
option(VRPN_USE_DIRECTSHOW "Build with Microsoft DirectShow support" OFF)
option(VRPN_INCLUDE_TIMECODE_SERVER "Build with Adrienne time-code server support" OFF)
option(VRPN_INCLUDE_INTERSENSE "Build with InterSense native library support" OFF)
option(VRPN_USE_NATIONAL_INSTRUMENTS "Build with National Instruments (old library) support" OFF)
option(VRPN_USE_NATIONAL_INSTRUMENTS_MX "Build with National Instruments (new library) support" OFF)
option(VRPN_USE_NIDAQ "Build with NIDAQ support" OFF)
option(VRPN_USE_USDIGITAL "Build with US Digital SEI/A2 library support" OFF)
option(VRPN_USE_MICROSCRIBE "Build with MicroScribe3D library support" OFF)
option(VRPN_INCLUDE_PHASESPACE "Build with PhaseSpace library support" OFF)
option(VRPN_USE_SHARED_LIBRARY "Enable to use DLLs on Windows (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_GPM_MOUSE "Build with GPM Linux mouse interface support" OFF)
option(VRPN_USE_MOTIONNODE "Build with GLI Interactive LLC MotionNode library support" OFF)
option(VRPN_USE_WIIUSE "Build with WiiUse library support" OFF)
option(VRPN_USE_FREESPACE "Build with Hillcrest Labs' Freespace devices support" OFF)

#option(vrpn_USE_MPI "Build with MPI support" OFF)

#-----------------------------------------------------------------------------
# Options that control what gets built and how.

# We can build two configurations (passing defs to the compile lines) - which ones do we want?
# Note that we can build both now, if desired!
option(VRPN_BUILD_CLIENT_LIBRARY "Build the vrpn library including only client code" ON)
option(VRPN_BUILD_SERVER_LIBRARY "Build the vrpnserver library including client and server code" ON)

# Build various applications if we want them.
option(VRPN_BUILD_CLIENTS "Build VRPN client apps and tests" ON)
option(VRPN_BUILD_SERVERS "Build VRPN servers" ON)
option(VRPN_BUILD_TEST_RPC_GENERATION "Build VRPN RPC generation" OFF)
if(WIN32)
	option(VRPN_BUILD_DIRECTSHOW_VIDEO_SERVER "Enable to build DirectShow Video Server (Windows)" OFF)
endif()

#-----------------------------------------------------------------------------
# Build the library itself and declare what bits need to be installed

# Note: configure is here so we don't distribute it as a public header.
set(VRPN_CLIENT_SOURCES
	"${PROJECT_BINARY_DIR}/vrpn_Configure.h"
	vrpn_Analog.C
	vrpn_Analog_Output.C
	vrpn_Auxiliary_Logger.C
	vrpn_BaseClass.C
	vrpn_Button.C
	vrpn_Connection.C
	vrpn_Dial.C
	vrpn_FileConnection.C
	vrpn_FileController.C
	vrpn_ForceDevice.C
	vrpn_Forwarder.C
	vrpn_ForwarderController.C
	vrpn_Imager.C
	vrpn_LamportClock.C
	vrpn_Mutex.C
	vrpn_Poser.C
	vrpn_RedundantTransmission.C
	vrpn_Serial.C
	vrpn_Shared.C
	vrpn_SharedObject.C
	vrpn_Sound.C
	vrpn_Text.C
	vrpn_Tracker.C
	vrpn_TypeDispatcher.C
)

set(VRPN_CLIENT_PUBLIC_HEADERS
	vrpn_Analog.h
	vrpn_Analog_Output.h
	vrpn_Auxiliary_Logger.h
	vrpn_BaseClass.h
	vrpn_Button.h
	vrpn_Connection.h
	vrpn_ConnectionTypes.h
	vrpn_Dial.h
	vrpn_FileConnection.h
	vrpn_FileController.h
	vrpn_ForceDevice.h
	vrpn_ForwarderController.h
	vrpn_Forwarder.h
	vrpn_Imager.h
	vrpn_LamportClock.h
	vrpn_Log.h
	vrpn_Mutex.h
	vrpn_Serial.h
	vrpn_Shared.h
	vrpn_SharedObject.h
	vrpn_Sound.h
	vrpn_Text.h
	vrpn_Tracker.h
	vrpn_TypeDispatcher.h
	vrpn_Types.h
)

set(VRPN_SERVER_SOURCES
	${VRPN_CLIENT_SOURCES}
	vrpn_3DConnexion.C
	vrpn_3DMicroscribe.C
	vrpn_3Space.C
	vrpn_5DT16.C
	vrpn_ADBox.C
	vrpn_Analog_5dt.C
	vrpn_Analog_Radamec_SPI.C
	vrpn_Analog_USDigital_A2.C
	vrpn_Atmel.C
	vrpn_Button_NI_DIO24.C
	vrpn_Button_USB.cpp
	vrpn_CerealBox.C
	vrpn_DirectXFFJoystick.C
	vrpn_DirectXRumblePad.C
	vrpn_Dyna.C
	vrpn_Event_Analog.C
	vrpn_Event.C
	vrpn_Event_Mouse.C
	vrpn_Flock.C
	vrpn_Flock_Parallel.C
	vrpn_ForceDeviceServer.C
	vrpn_Freespace.C
	vrpn_FunctionGenerator.C
	vrpn_GlobalHapticsOrb.C
	vrpn_HumanInterface.C
	vrpn_Imager_Stream_Buffer.C
	vrpn_ImmersionBox.C
	vrpn_inertiamouse.C
	vrpn_JoyFly.C
	vrpn_Joylin.C
	vrpn_Joywin32.C
	vrpn_Keyboard.C
	vrpn_Magellan.C
	vrpn_Mouse.C
	vrpn_NationalInstruments.C
	vrpn_Nidaq.C
	vrpn_nikon_controls.C
	vrpn_Poser_Analog.C
	vrpn_Poser_Tek4662.C
	vrpn_raw_sgibox.C
	vrpn_sgibox.C
	vrpn_Spaceball.C
	vrpn_Tng3.C
	vrpn_Tracker_3DMouse.C
	vrpn_Tracker_AnalogFly.C
	vrpn_Tracker_ButtonFly.C
	vrpn_Tracker_Crossbow.C
	vrpn_Tracker_DTrack.C
	vrpn_Tracker_Fastrak.C
	vrpn_Tracker_isense.C
	vrpn_Tracker_Isotrak.C
	vrpn_Tracker_Liberty.C
	vrpn_Tracker_MotionNode.C
	vrpn_Tracker_NDI_Polaris.C
	vrpn_Tracker_PhaseSpace.C
	vrpn_UNC_Joystick.C
	vrpn_VPJoystick.C
	vrpn_Wanda.C
	vrpn_WiiMote.C
	vrpn_XInputGamepad.C
	vrpn_Xkeys.C
	vrpn_Zaber.C
)

set(VRPN_SERVER_PUBLIC_HEADERS
	${VRPN_CLIENT_PUBLIC_HEADERS}
	vrpn_3DConnexion.h
	vrpn_3DMicroscribe.h
	vrpn_3Space.h
	vrpn_5DT16.h
	vrpn_ADBox.h
	vrpn_Analog_5dt.h
	vrpn_Analog_Radamec_SPI.h
	vrpn_Analog_USDigital_A2.h
	vrpn_Atmel.h
	vrpn_Button_NI_DIO24.h
	vrpn_Button_USB.h
	vrpn_CerealBox.h
	vrpn_DirectXFFJoystick.h
	vrpn_DirectXRumblePad.h
	vrpn_Dyna.h
	vrpn_Event_Analog.h
	vrpn_Event.h
	vrpn_Event_Mouse.h
	vrpn_Flock.h
	vrpn_Flock_Parallel.h
	vrpn_ForceDeviceServer.h
	vrpn_Freespace.h
	vrpn_FunctionGenerator.h
	vrpn_GlobalHapticsOrb.h
	vrpn_HashST.h
	vrpn_HumanInterface.h
	vrpn_Imager_Stream_Buffer.h
	vrpn_ImmersionBox.h
	vrpn_inertiamouse.h
	vrpn_JoyFly.h
	vrpn_Joylin.h
	vrpn_Joywin32.h
	vrpn_Keyboard.h
	vrpn_Magellan.h
	vrpn_Mouse.h
	vrpn_NationalInstruments.h
	vrpn_Nidaq.h
	vrpn_nikon_controls.h
	vrpn_Poser_Analog.h
	vrpn_Poser.h
	vrpn_Poser_Tek4662.h
	vrpn_raw_sgibox.h
	vrpn_RedundantTransmission.h
	vrpn_sgibox.h
	vrpn_Spaceball.h
	vrpn_Tng3.h
	vrpn_Tracker_3DMouse.h
	vrpn_Tracker_AnalogFly.h
	vrpn_Tracker_ButtonFly.h
	vrpn_Tracker_Crossbow.h
	vrpn_Tracker_DTrack.h
	vrpn_Tracker_Fastrak.h
	vrpn_Tracker_isense.h
	vrpn_Tracker_Isotrak.h
	vrpn_Tracker_Liberty.h
	vrpn_Tracker_MotionNode.h
	vrpn_Tracker_NDI_Polaris.h
	vrpn_Tracker_PhaseSpace.h
	vrpn_UNC_Joystick.h
	vrpn_VPJoystick.h
	vrpn_Wanda.h
	vrpn_WiiMote.h
	vrpn_XInputGamepad.h
	vrpn_Xkeys.h
	vrpn_Zaber.h
)

if(VRPN_BUILD_SERVER_LIBRARY)

	add_library(vrpnserver ${VRPN_SERVER_SOURCES} ${VRPN_SERVER_PUBLIC_HEADERS})
	target_link_libraries(vrpnserver ${EXTRA_LIBS} ${SERVER_LIBS})
	if(VRPN_USE_GPM_MOUSE)
		# XXX use a find_package here
		target_link_libraries(vrpnserver gpm)
	endif()
	set_property(TARGET vrpnserver PROPERTY PUBLIC_HEADER ${VRPN_SERVER_PUBLIC_HEADERS})

	install(TARGETS vrpnserver
		ARCHIVE DESTINATION lib
		PUBLIC_HEADER DESTINATION include)
endif()



if(VRPN_BUILD_CLIENT_LIBRARY)
	add_library(vrpn ${VRPN_CLIENT_SOURCES} ${VRPN_CLIENT_PUBLIC_HEADERS})
	target_link_libraries(vrpn ${EXTRA_LIBS})

	if(NOT VRPN_BUILD_SERVER_LIBRARY)
		# Only install these headers if we aren't building a server library.
		set_property(TARGET vrpn PROPERTY PUBLIC_HEADER ${VRPN_CLIENT_PUBLIC_HEADERS})
	endif()

	set_property(TARGET vrpn PROPERTY COMPILE_DEFINITIONS "VRPN_CLIENT_ONLY")

	install(TARGETS vrpn
		ARCHIVE DESTINATION lib
		PUBLIC_HEADER DESTINATION include)
endif()


# Include directory needed by all of the files
include_directories(${VRPN_SOURCE_DIR} ${VRPN_SOURCE_DIR}/atmellib ${QUATLIB_INCLUDE_DIR})

add_subdirectory (client_src)

#-----------------------------------------------------------------------------
# Build the atmellib library if we're building a server library

if(VRPN_BUILD_SERVER_LIBRARY)
	add_subdirectory (atmellib)
endif()

#-----------------------------------------------------------------------------
# Build the server applications if we've been asked to and we didn't build
# the library client-only.

if(VRPN_BUILD_SERVERS AND VRPN_BUILD_SERVER_LIBRARY)
	add_subdirectory (server_src)
endif()

#-----------------------------------------------------------------------------
# Build the RPC generation if we've been asked to

if(VRPN_BUILD_TEST_RPC_GENERATION)
	add_subdirectory (util/gen_rpc)
endif()

#-----------------------------------------------------------------------------
# Applications that live in the main directory.
if(BUILD_VRPN_SERVERS AND VRPN_BUILD_SERVER_LIBRARY)
	foreach(SOURCE time_test.cpp)
		get_filename_component(APP ${SOURCE} NAME_WE)
		add_executable(${APP} ${SOURCE})
		target_link_libraries(${APP} vrpn)
		set_target_properties(${APP} PROPERTIES SOLUTION_FOLDER tests)
		install(TARGETS ${APP}
			RUNTIME DESTINATION bin)
	endforeach()
endif()
