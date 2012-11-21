set(ATMELLIB_PUBLIC_HEADERS
	vrpn_atmellib_errno.h
	vrpn_atmellib.h)

set(ATMELLIB_SOURCES
	vrpn_atmellib_helper.h
	vrpn_atmellib_helper.C
	vrpn_atmellib_iobasic.C
	vrpn_atmellib_openclose.C
	vrpn_atmellib_register.C
	vrpn_atmellib_tester.C)

add_library(vrpn_atmel ${ATMELLIB_PUBLIC_HEADERS} ${ATMELLIB_SOURCES})
set_property(TARGET
	vrpn_atmel
	PROPERTY
	PUBLIC_HEADER
	${ATMELLIB_PUBLIC_HEADERS})
set_property(TARGET
	vrpn_atmel
	PROPERTY
	FOLDER
	"Library")

install(TARGETS
	vrpn_atmel
	ARCHIVE
	DESTINATION
	lib
	COMPONENT
	serversdk
	PUBLIC_HEADER
	DESTINATION
	include
	COMPONENT
	serversdk)
