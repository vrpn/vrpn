# XXX TODO:
# Correctly configure so that aec_nttc.lib is linked and AECiNTTC.H is found
# and setting VRPN_INCLUDE_TIMECODE_SERVER

set(TIMECODE_GENSERV_SOURCES
	vrpn_timecode_generator.cpp)

set(TIMECODE_GENSERV_PUBLIC_HEADERS
	vrpn_timecode_generator.h)

add_library(vrpn_timecode_generator ${TIMECODE_GENSERV_SOURCES} ${TIMECODE_GENSERV_PUBLIC_HEADERS})
set_property(TARGET vrpn_timecode_generator PROPERTY
		PUBLIC_HEADER ${TIMECODE_GENSERV_PUBLIC_HEADERS})

install(TARGETS vrpn_timecode_generator
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION include)
