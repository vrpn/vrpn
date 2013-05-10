###
# Get the version number by parsing vrpn_Connection.C
###
include(WarningDev.cmake)
set(_vrpn_version_file "${CMAKE_CURRENT_SOURCE_DIR}/vrpn_Connection.C")
if(EXISTS "${_vrpn_version_file}")
	file(READ "${_vrpn_version_file}" _vrpn_version_contents)
endif()
set(_vrpn_version_regex "vrpn: ver. ([0-9]+).([0-9]+)")
string(REGEX
	MATCH
	"${_vrpn_version_regex}"
	_dummy
	"${_vrpn_version_contents}")

if(CMAKE_MATCH_1)
	set(CPACK_PACKAGE_VERSION_MAJOR "${CMAKE_MATCH_1}")
else()
	set(CPACK_PACKAGE_VERSION_MAJOR "07")
	warning_dev("Could not parse major version from vrpn_Connection.C")
endif()

if(CMAKE_MATCH_2)
	set(CPACK_PACKAGE_VERSION_MINOR "${CMAKE_MATCH_2}")
else()
	set(CPACK_PACKAGE_VERSION_MAJOR "26")
	warning_dev("Could not parse minor version from vrpn_Connection.C")
endif()

set(CPACK_PACKAGE_VERSION
	"${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
set(CONFIG_VERSION "${CPACK_PACKAGE_VERSION}")
set(BRIEF_VERSION "${CPACK_PACKAGE_VERSION}")

include(GetGitRevisionDescription)
git_get_exact_tag(GIT_EXACT_TAG --tags --match version_*)
if(GIT_EXACT_TAG)
	# Extract simple version number from description to verify it
	string(REPLACE "version_" "" GIT_LAST_TAGGED_VER "${GIT_EXACT_TAG}")
	string(REGEX
		REPLACE
		"-.*"
		""
		GIT_LAST_TAGGED_VER
		"${GIT_LAST_TAGGED_VER}")
	if(GIT_LAST_TAGGED_VER VERSION_EQUAL CPACK_PACKAGE_VERSION)
		# OK, they match.  All done!
		return()
	endif()

	message("WARNING: Git reports that the current source code is tagged '${GIT_EXACT_TAG}',
	but the version extracted from the source code (${CPACK_PACKAGE_VERSION}) differs!
	You may need to fix the source code's version and/or update the tag!

	The version extracted from the source code will be trusted for now,
	but please fix this issue!\n")

else()
	# Not an exact tag, let's try a description
	git_describe(GIT_REVISION_DESCRIPTION --tags --match version_*)
	if(GIT_REVISION_DESCRIPTION)
		# Extract simple version number from description
		string(REPLACE
			"version_"
			""
			GIT_LAST_TAGGED_VER
			"${GIT_REVISION_DESCRIPTION}")
		set(GIT_REDUCED_REVISION_DESCRIPTION "${GIT_LAST_TAGGED_VER}")
		string(REGEX
			REPLACE
			"-.*"
			""
			GIT_LAST_TAGGED_VER
			"${GIT_LAST_TAGGED_VER}")

		if(GIT_LAST_TAGGED_VER VERSION_LESS CPACK_PACKAGE_VERSION)
			# Prerelease
			message(STATUS
				"Git's description of the current revision indicates this is a prerelease of ${CPACK_PACKAGE_VERSION}: ${GIT_REVISION_DESCRIPTION}\n")
			set(CPACK_PACKAGE_VERSION_PATCH
				"0~prerelease-git-${GIT_REDUCED_REVISION_DESCRIPTION}")
			set(CONFIG_VERSION "pre-${CONFIG_VERSION}")
		else()
			# OK, this is a followup version
			# TODO verify assumption
			message(STATUS
				"Git's description of the current revision indicates this is a patched version of ${CPACK_PACKAGE_VERSION}: ${GIT_REVISION_DESCRIPTION}\n")
			set(CONFIG_VERSION "post-${CONFIG_VERSION}")
			set(CPACK_PACKAGE_VERSION_PATCH "0-git-${GIT_REDUCED_REVISION_DESCRIPTION}")
		endif()
		set(CPACK_PACKAGE_VERSION
			"${CPACK_PACKAGE_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
	endif()
endif()

