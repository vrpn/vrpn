if(NOT VRPN_CLIENT_LIBRARY)
	message(FATAL_ERROR
		"Cannot compile RPC test without either the client or full library.  Enable one and try again.")
endif()

#-----------------------------------------------------------------------------
# Include directories needed
include_directories(${CMAKE_CURRENT_BINARY_DIR}
	${VRPN_SOURCE_DIR}
	${QUATLIB_INCLUDE_DIR})

#-----------------------------------------------------------------------------
# Custom build steps to generate RPC code.
# Executable built from these.
set(SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/gen_vrpn_rpc.pl")

set(MESSAGE_DEF rpc_Test.vrpndef)

# Must copy the message def to the binary directory
add_custom_command(OUTPUT
	"${CMAKE_CURRENT_BINARY_DIR}/${MESSAGE_DEF}"
	COMMAND
	"${CMAKE_COMMAND}"
	-E
	copy_if_different
	"${CMAKE_CURRENT_SOURCE_DIR}/${MESSAGE_DEF}"
	"${CMAKE_CURRENT_BINARY_DIR}/${MESSAGE_DEF}"
	DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/${MESSAGE_DEF}"
	COMMENT
	"Copying message definition file ${MESSAGE_DEF}")

# This generates the source files
function(gen_vrpn_rpc OUTVAR OUT IN)
	add_custom_command(OUTPUT
		"${CMAKE_CURRENT_BINARY_DIR}/${OUT}"
		COMMAND
		${PERL}
		"${SCRIPT}"
		${ARGN}
		"${CMAKE_CURRENT_SOURCE_DIR}/${IN}"
		DEPENDS
		"${SCRIPT}"
		"${CMAKE_CURRENT_SOURCE_DIR}/${IN}"
		"${CMAKE_CURRENT_BINARY_DIR}/${MESSAGE_DEF}"
		WORKING_DIRECTORY
		"${CMAKE_CURRENT_BINARY_DIR}"
		COMMENT
		"Generating ${OUT} from ${IN}"
		VERBATIM)
	list(APPEND ${OUTVAR} "${CMAKE_CURRENT_BINARY_DIR}/${OUT}")
	set(${OUTVAR} ${${OUTVAR}} PARENT_SCOPE)
endfunction()

set(GENERATED_SOURCES)
gen_vrpn_rpc(GENERATED_SOURCES rpc_Test_Remote.C rpc_Test_Remote.Cdef)
gen_vrpn_rpc(GENERATED_SOURCES rpc_Test_Remote.h rpc_Test_Remote.hdef)
gen_vrpn_rpc(GENERATED_SOURCES rpc_Test.h rpc_Test.vrpndef -h)
gen_vrpn_rpc(GENERATED_SOURCES rpc_Test.C rpc_Test.vrpndef -C)

add_executable(test_gen_rpc main_test.C ${GENERATED_SOURCES})

target_link_libraries(test_gen_rpc ${VRPN_CLIENT_LIBRARY})

if(PTHREAD_LIBRARY)
	target_link_libraries(test_gen_rpc ${PTHREAD_LIBRARY})
endif()
