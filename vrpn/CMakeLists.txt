cmake_minimum_required(VERSION 2.6)
project(VRPN)

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
list(APPEND CMAKE_MODULE_PATH ${VRPN_SOURCE_DIR}/cmake)
find_package(quatlib REQUIRED)	# Sets QUATLIB_INCLUDE_DIR and QUATLIB_LIBRARIES
find_package(Threads)
set(EXTRA_LIBS ${QUATLIB_LIBRARIES})

if(NOT WIN32)
	list(APPEND EXTRA_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

if(APPLE)
	find_library(MACOSX_COREFOUNDATION_LIBRARY CoreFoundation)
	find_library(MACOSX_IOKIT_LIBRARY IOKit)
	list(APPEND EXTRA_LIBS ${MACOSX_COREFOUNDATION_LIBRARY} ${MACOSX_IOKIT_LIBRARY})
	mark_as_advanced(MACOSX_COREFOUNDATION_LIBRARY MACOSX_IOKIT_LIBRARY)
endif()

#-----------------------------------------------------------------------------
# configure a header file to pass some of the CMake settings
# to the source code
configure_file(
  "${PROJECT_SOURCE_DIR}/vrpn_Configure.h.cmake_in"
  "${PROJECT_BINARY_DIR}/vrpn_Configure.h"
)
include_directories(${PROJECT_BINARY_DIR})


# Configuration options controlling what gets included in the build.
option(VRPN_USE_PHANTOM_SERVER "Enable to include SensAble Phantom code in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_DIRECTINPUT "Enable to include Microsoft DirectInput code in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_DIRECTSHOW "Enable to include Microsoft DirectShow code in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_INCLUDE_TIMECODE_SERVER "Enable to include Adrienne time-code server code in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_INCLUDE_INTERSENSE "Enable to include InterSense native library code in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_NATIONAL_INSTRUMENTS "Enable to include National Instruments (old library) in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_NATIONAL_INSTRUMENTS_MX "Enable to include National Instruments (new library) in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_USDIGITAL "Enable to include US Digital SEI/A2 library in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_MICROSCRIBE "Enable to include MicroScribe3D library in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_INCLUDE_PHASESPACE "Enable to include PhaseSpace library in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_SHARED_LIBRARY "Enable to use DLLs on Windows (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_GPM_MOUSE "Enable to include GPM Linux mouse interface in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_MOTIONNODE "Enable to include GLI Interactive LLC MotionNode library in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_WIIUSE "Enable to include WiiUse library in build (see vrpn_Configure.h for more info)" OFF)
option(VRPN_USE_FREESPACE "Enable to include Hillcrest Labs' Freespace devices in build (see vrpn_Configure.h for more info)" OFF)

#option(vrpn_USE_MPI "Enable to include MPI in build (see vrpn_Configure.h for more info)" OFF)

#-----------------------------------------------------------------------------
# Options that control what gets built and how.

# We can build two configurations (passing defs to the compile lines) - which do we want?
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
	vrpn_Connection.C
	vrpn_Tracker.C
	vrpn_Button.C
	vrpn_ForceDevice.C
	vrpn_Shared.C
	vrpn_Analog.C
	vrpn_FileConnection.C
	vrpn_FileController.C
	vrpn_Forwarder.C
	vrpn_Text.C
	vrpn_ForwarderController.C
	vrpn_Serial.C
	vrpn_Dial.C
	vrpn_SharedObject.C
	vrpn_BaseClass.C
	vrpn_Sound.C
	vrpn_LamportClock.C
	vrpn_Mutex.C
	vrpn_RedundantTransmission.C
	vrpn_Imager.C
	vrpn_Analog_Output.C
	vrpn_Poser.C
	vrpn_Auxiliary_Logger.C
)

set(VRPN_CLIENT_PUBLIC_HEADERS
	vrpn_Connection.h
	vrpn_Tracker.h
	vrpn_Button.h
	vrpn_Sound.h
	vrpn_ForceDevice.h
	vrpn_Shared.h
	vrpn_Analog.h
	vrpn_FileConnection.h
	vrpn_FileController.h
	vrpn_Forwarder.h
	vrpn_Text.h
	vrpn_ForwarderController.h
	vrpn_Serial.h
	vrpn_Dial.h
	vrpn_SharedObject.h
	vrpn_LamportClock.h
	vrpn_Mutex.h
	vrpn_BaseClass.h
	vrpn_Imager.h
	vrpn_Analog_Output.h
	vrpn_Poser.h
	vrpn_Auxiliary_Logger.h
)

set(VRPN_SERVER_SOURCES
	${VRPN_CLIENT_SOURCES}
	vrpn_3Space.C
	vrpn_Flock.C
	vrpn_Tracker_Fastrak.C
	vrpn_Dyna.C
	vrpn_Flock_Parallel.C
	vrpn_UNC_Joystick.C
	vrpn_JoyFly.C
	vrpn_sgibox.C
	vrpn_CerealBox.C
	vrpn_Tracker_AnalogFly.C
	vrpn_raw_sgibox.C
	vrpn_Magellan.C
	vrpn_Analog_Radamec_SPI.C
	vrpn_ImmersionBox.C
	vrpn_Wanda.C
	vrpn_Analog_5dt.C
	vrpn_Joylin.C
	vrpn_Tng3.C
	vrpn_Spaceball.C
	vrpn_Tracker_isense.C
	vrpn_Zaber.C
	vrpn_nikon_controls.C
	vrpn_GlobalHapticsOrb.C
	vrpn_Tracker_ButtonFly.C
	vrpn_ADBox.C
	vrpn_VPJoystick.C
	vrpn_Tracker_Liberty.C
	vrpn_NationalInstruments.C
	vrpn_Poser_Analog.C
	vrpn_Tracker_DTrack.C
	vrpn_Poser_Tek4662.C
	vrpn_Tracker_Crossbow.C
	vrpn_Tracker_3DMouse.C
	vrpn_Mouse.C
	vrpn_3DMicroscribe.C
	vrpn_5DT16.C
	vrpn_ForceDeviceServer.C
	vrpn_Keyboard.C
	vrpn_Analog_USDigital_A2.C
	vrpn_Button_NI_DIO24.C
	vrpn_Tracker_PhaseSpace.C
	vrpn_Atmel.C
	vrpn_inertiamouse.C
	vrpn_Event.C
	vrpn_Event_Analog.C
	vrpn_Event_Mouse.C
	vrpn_Imager_Stream_Buffer.C
	vrpn_HumanInterface.C
	vrpn_Xkeys.C
	vrpn_3DConnexion.C
	vrpn_Tracker_MotionNode.C
	vrpn_Tracker_NDI_Polaris.C
	vrpn_WiiMote.C
	vrpn_Tracker_Isotrak.C
	vrpn_Freespace.C
	vrpn_Button_USB.cpp
	vrpn_Joywin32.C
	vrpn_DirectXFFJoystick.C
	vrpn_DirectXRumblePad.C
	vrpn_FunctionGenerator.C
	vrpn_XInputGamepad.C
)

set(VRPN_SERVER_PUBLIC_HEADERS
	${VRPN_CLIENT_PUBLIC_HEADERS}
	vrpn_3Space.h
	vrpn_Flock.h
	vrpn_Tracker_Fastrak.h
	vrpn_Dyna.h
	vrpn_Flock_Parallel.h
	vrpn_UNC_Joystick.h
	vrpn_JoyFly.h
	vrpn_sgibox.h
	vrpn_raw_sgibox.h
	vrpn_CerealBox.h
	vrpn_Tracker_AnalogFly.h
	vrpn_Magellan.h
	vrpn_Analog_Radamec_SPI.h
	vrpn_ImmersionBox.h
	vrpn_Wanda.h
	vrpn_Analog_5dt.h
	vrpn_Joylin.h
	vrpn_Tng3.h
	vrpn_Spaceball.h
	vrpn_Tracker_isense.h
	vrpn_Zaber.h
	vrpn_nikon_controls.h
	vrpn_GlobalHapticsOrb.h
	vrpn_Tracker_ButtonFly.h
	vrpn_ADBox.h
	vrpn_VPJoystick.h
	vrpn_Tracker_Liberty.h
	vrpn_NationalInstruments.h
	vrpn_Poser_Analog.h
	vrpn_Tracker_DTrack.h
	vrpn_Poser.h
	vrpn_Poser_Tek4662.h
	vrpn_Tracker_Crossbow.h
	vrpn_Tracker_3DMouse.h
	vrpn_Mouse.h
	vrpn_3DMicroscribe.h
	vrpn_5DT16.h
	vrpn_ForceDeviceServer.h
	vrpn_Keyboard.h
	vrpn_Analog_USDigital_A2.h
	vrpn_Button_NI_DIO24.h
	vrpn_Tracker_PhaseSpace.h
	vrpn_Atmel.h
	vrpn_inertiamouse.h
	vrpn_Event.h
	vrpn_Event_Analog.h
	vrpn_Event_Mouse.h
	vrpn_Imager_Stream_Buffer.h
	vrpn_HumanInterface.h
	vrpn_Xkeys.h
	vrpn_3DConnexion.h
	vrpn_Tracker_MotionNode.h
	vrpn_Tracker_NDI_Polaris.h
	vrpn_WiiMote.h
	vrpn_Tracker_Isotrak.h
	vrpn_Freespace.h
	vrpn_Button_USB.h
	vrpn_Joywin32.h
	vrpn_Freespace.h
	vrpn_DirectXFFJoystick.h
	vrpn_DirectXRumblePad.h
	vrpn_FunctionGenerator.h
	vrpn_RedundantTransmission.h
	vrpn_XInputGamepad.h
)

set(VRPN_SERVER_LIBRARY NO)
set(VRPN_CLIENT_LIBRARY NO)

if(VRPN_BUILD_SERVER_LIBRARY)
	set(VRPN_SERVER_LIBRARY vrpnserver)
	set(VRPN_CLIENT_LIBRARY vrpnserver) # If we build the client, this gets overridden.

	add_library(vrpnserver ${VRPN_SERVER_SOURCES} ${VRPN_SERVER_PUBLIC_HEADERS})
	target_link_libraries(vrpnserver ${QUATLIB_LIBRARIES} ${EXTRA_LIBS})
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
	set(VRPN_CLIENT_LIBRARY vrpn)
	add_library(vrpn ${VRPN_CLIENT_SOURCES} ${VRPN_CLIENT_PUBLIC_HEADERS})
	target_link_libraries(vrpn ${QUATLIB_LIBRARIES} ${EXTRA_LIBS})

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

if(NOT VRPN_BUILD_LIBRARY_CLIENT_ONLY)
	add_subdirectory (atmellib)
endif()

#-----------------------------------------------------------------------------
# Build the server applications if we've been asked to and we didn't build
# the library client-only.

if(VRPN_BUILD_SERVERS AND NOT VRPN_BUILD_LIBRARY_CLIENT_ONLY)
	add_subdirectory (server_src)
endif()

#-----------------------------------------------------------------------------
# Build the RPC generation if we've been asked to

if(VRPN_BUILD_TEST_RPC_GENERATION)
	add_subdirectory (util/gen_rpc)
endif()

#-----------------------------------------------------------------------------
# Applications that live in the main directory.
if(BUILD_VRPN_SERVERS AND NOT VRPN_BUILD_LIBRARY_CLIENT_ONLY)
	foreach(SOURCE time_test.cpp)
		get_filename_component(APP ${SOURCE} NAME_WE)
		add_executable(${APP} ${SOURCE})
		target_link_libraries(${APP} vrpn)
		set_target_properties(${APP} PROPERTIES SOLUTION_FOLDER tests)
		install(TARGETS ${APP}
			RUNTIME DESTINATION bin)
	endforeach()
endif()
