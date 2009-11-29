cmake_minimum_required(VERSION 2.6)

#-----------------------------------------------------------------------------
# XXX Things to make better.
#
# Make this compile the Atmel Library and put it in the right place.

if(NOT WIN32)
	add_library(vrpn_atmel
		vrpn_atmellib_errno.h
		vrpn_atmellib.h
		vrpn_atmellib_helper.h

		vrpn_atmellib_helper.C
		vrpn_atmellib_iobasic.C
		vrpn_atmellib_openclose.C
		vrpn_atmellib_register.C
		vrpn_atmellib_tester.C
	)
	install(TARGETS vrpn_atmel ARCHIVE DESTINATION lib)

endif()

