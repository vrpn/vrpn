#!/bin/sh
set -e

###
# Auto-Configuration
###

# Find the VRPN source root relative to this script's known location.
VRPN=$(cd $(dirname $0) && cd .. && pwd)
# Relative path to script
SCRIPT="util/$(basename $0)"
# Verboseness - just use the environment
VERBOSE=${VERBOSE:-false}

# Pick the DOS endline remover
if ! DOS2UNIX=$(which dos2unix || which fromdos); then
    echo "Can't find dos2unix or fromdos! Must exit!" >&2
    exit 1
fi

# Check for sed
if ! which sed > /dev/null; then
    echo "Can't find 'sed'!" >&2
    exit 1
fi

# Check for find
if ! which find > /dev/null; then
    echo "Can't find 'find'!" >&2
    exit 1
fi

###
# Startup Message
###

echo
if ${VERBOSE}; then
    echo "VRPN Source Root: ${VRPN}"
    echo "Cleanup script:   ${SCRIPT}"
else
    echo "(Run with environment variable VERBOSE=true for full output)"
fi
echo


###
# Status message functions
###

# Internal use only - don't call from the main script
# Wrapper for echo that prefixes the filename, a colon, and a tab.
DisplayMessageInternal() {
    echo "${FILETOPROCESS}: $*"
}

# Display a message only seen in verbose mose
StatusMessage() {
    if ${VERBOSE}; then
        DisplayMessageInternal "$*"
    fi
}

# Display a message always seen, with extra indent
ResultMessage() {
    DisplayMessageInternal "    $*"
}

# Display a message always seen on standard error
ErrorMessage() {
    ResultMessage "$*" >&2
}

###
# Functions for file processing
#
# Must always start by calling StartProcessingFile with the filename.
# Then, any other function may be called: the cleaning functions do
# not need a filename passed to them because of StartProcessingFile.
###

# Must be called before each new file processing begins.
StartProcessingFile() {
    export FILETOPROCESS=$1
}

RemoveDosEndlines() {
    StatusMessage "Removing DOS endlines with ${DOS2UNIX}"
    ${DOS2UNIX} ${FILETOPROCESS}
}

TrimTrailingWhitespace() {
    StatusMessage "Trimming trailing whitespace with sed"
    sed -i 's/[ \t]*$//' ${FILETOPROCESS}
}

# Greps for lines that aren't all whitespace and don't start with # as a comment marker
# such as in vrpn.cfg
CheckForUncommentedLines() {
    StatusMessage "Checking for un-commented, non-empty lines"
    if grep --with-filename --line-number -v -E '^(#.*)?\s*$' ${FILETOPROCESS} >&2; then
        ErrorMessage "Oops - you have uncommented lines (see above) - don't commit this file!"
    else
        ResultMessage "No un-commented lines found - OK to commit this file!"
    fi
}

RemoveExecutablePrivilege() {
    StatusMessage "Ensuring file is not marked as executable"
    chmod a-x ${FILETOPROCESS}
}

AddExecutablePrivilege() {
    StatusMessage "Ensuring file is marked as executable"
    chmod a+x ${FILETOPROCESS}
}


###
# Main script - process files!
###

# Open a subshell in which we'll change directories to VRPN root to process
# files relative to there.
(
    cd ${VRPN}

    # Clean up this script first!
    StartProcessingFile ${SCRIPT}
    RemoveDosEndlines
    TrimTrailingWhitespace
    AddExecutablePrivilege

    # Server config file
    StartProcessingFile server_src/vrpn.cfg
    RemoveDosEndlines
    #TrimTrailingWhitespace  # Is this safe to do?
    CheckForUncommentedLines
    RemoveExecutablePrivilege

    # Clean up all CMake files we control
    for fn in $(find * -name "CMakeLists.txt") *.cmake cmake/*.cmake submodules/*.cmake; do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        TrimTrailingWhitespace
        RemoveExecutablePrivilege
    done
)

