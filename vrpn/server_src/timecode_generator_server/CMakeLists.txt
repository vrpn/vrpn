CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Include directory needed by all of the files
INCLUDE_DIRECTORIES(${VRPN_SOURCE_DIR})

ADD_LIBRARY (vrpn_timecode_generator
	vrpn_timecode_generator.cpp vrpn_timecode_generator.h
)
INSTALL (TARGETS vrpn_timecode_generator
	ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
INSTALL(FILES
	vrpn_timecode_generator.h
	DESTINATION ${CMAKE_INSTALL_PREFIX}/include	
)
