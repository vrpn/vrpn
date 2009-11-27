CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
# 
# Make this compile the Atmel Library and put it in the right place.

if (NOT WIN32)
	ADD_LIBRARY(vrpn_atmel
		vrpn_atmellib_errno.h vrpn_atmellib.h vrpn_atmellib_helper.h

		vrpn_atmellib_helper.C vrpn_atmellib_iobasic.C
		vrpn_atmellib_openclose.C vrpn_atmellib_register.C
		vrpn_atmellib_tester.C
	)
	INSTALL(TARGETS vrpn_atmel ARCHIVE DESTINATION {$CMAKE_INSTALL_PREFIX}/lib)

endif (NOT WIN32)

