CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#-----------------------------------------------------------------------------
# Include directories needed
INCLUDE_DIRECTORIES(
	${VRPN_SOURCE_DIR}
	${QUATLIB_INCLUDE_DIR}
	${VRPN_SOURCE_DIR}/server_src
	${VRPN_SOURCE_DIR}/server_src/timecode_generator_server
	${VRPN_SOURCE_DIR}/server_src/ghostEffects
	${HDAPI_INCLUDE_DIR} ${HDAPI_HDU_INCLUDE_DIR}
)

if (WIN32)
	set (PERL C:/cygwin/bin/perl.exe)
else (WIN32)
	set (PTHREAD_LIBRARY pthread)
	set (PERL perl)
endif (WIN32)

#-----------------------------------------------------------------------------
# Custom build steps to generate RPC code.
# Executable built from these.
add_custom_command (OUTPUT "${PROJECT_SOURCE_DIR}/util/gen_rpc/rpc_Test_Remote.C"
	COMMAND ${PERL} gen_vrpn_rpc.pl rpc_Test_Remote.Cdef
	WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/util/gen_rpc" )
add_custom_command (OUTPUT "${PROJECT_SOURCE_DIR}/util/gen_rpc/rpc_Test_Remote.h"
	COMMAND ${PERL} gen_vrpn_rpc.pl rpc_Test_Remote.hdef
	WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/util/gen_rpc" )

SET(TARGET_NAME test_gen_rpc)
ADD_EXECUTABLE(test_gen_rpc main_test.C rpc_Test.C rpc_Test_Remote.C rpc_Test_Remote.h)
TARGET_LINK_LIBRARIES(test_gen_rpc
	vrpn
	${PTHREAD_LIBRARY}
)
