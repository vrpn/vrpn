#!/bin/sh
set -e

# Find the VRPN source root relative to this script's known location.
VRPN=$(cd $(dirname $0) && cd .. && pwd)
SCRIPT="util/$(basename $0)"
echo
echo "VRPN Source Root:\t${VRPN}"
echo "Cleanup script:\t${SCRIPT}"
echo


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

SUCCEEDED=true
FILETOPROCESS=
PROCESSED=

###
# Functions for re-use when processing more files before commit
#
# Seems silly to have them when we just process a few fukes, but ideally other
# files would also get the "star treatment" of this verification/cleanup pass.
# I'm just not sure which ones. Anyway, it makes it easier to read.
#   -- Ryan Pavlik, Oct 2012
###

StartProcessingFile() {
    FILETOPROCESS=$1
    echo
}

StatusMessage() {
    echo "${FILETOPROCESS}:\t$*"
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
    if grep -v -E '^(#.*)?\s*$' ${FILETOPROCESS}; then
        StatusMessage "\tOops - you have uncommented lines (see above) - don't commit this file!"
        SUCCEEDED=false
    else
        StatusMessage "\tNo un-commented lines found - OK to commit this file!"
    fi

}

# Open a subshell in which we'll change directories to VRPN root to process
# files relative to there.
(
    cd ${VRPN}

    # Clean up this script first!
    StartProcessingFile ${SCRIPT}
    RemoveDosEndlines
    TrimTrailingWhitespace

    StartProcessingFile server_src/vrpn.cfg
    RemoveDosEndlines
    #TrimTrailingWhitespace
    CheckForUncommentedLines

    # Clean up all CMake files
    for fn in $(find * -name "CMakeLists.txt") *.cmake cmake/*.cmake submodules/*.cmake; do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        TrimTrailingWhitespace
    done
)

echo

if ${SUCCEEDED}; then
    echo "Cleanups complete and all checks OK!"
else
    echo "One or more errors/warnings - see above!"
    exit 1
fi
