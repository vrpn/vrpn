cmake_minimum_required(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
#
# Move applications into subprojects in visual studio
# XXX tracker_to_poser
# XXX daq_server
# Fix the ones that are commented out so that they compile and run?

if(NOT VRPN_BUILD_SERVER_LIBRARY OR NOT VRPN_USE_HID)
	message(FATAL_ERROR
		"Cannot compile HID gui without the full (server) library and HID support.  Enable it and try again.")
endif()

set(VRPN_SERVER_LIBRARY vrpnserver)

if(NOT WIN32)
	set(VRPN_ATMEL_LIBRARY vrpn_atmel)
endif()


find_package(Qt4)
if(${QT_FOUND})
	include(${QT_USE_FILE})
	
endif()

