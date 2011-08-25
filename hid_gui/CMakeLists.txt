
if(NOT VRPN_BUILD_SERVER_LIBRARY OR NOT VRPN_USE_HID)
	message(FATAL_ERROR
		"Cannot compile HID gui without the full (server) library and HID support.  Enable it and try again.")
endif()

set(VRPN_SERVER_LIBRARY vrpnserver)

if(NOT WIN32)
	set(VRPN_ATMEL_LIBRARY vrpn_atmel)
endif()


find_package(Qt4)
if(QT_FOUND)
	include(${QT_USE_FILE})
	
endif()

