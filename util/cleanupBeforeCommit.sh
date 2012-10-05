#!/bin/sh
set -e

# Find the VRPN source root relative to this script's known location.
VRPN=$(cd $(dirname $0) && cd .. && pwd)
echo
echo "VRPN Source Root: ${VRPN}"
echo


# Pick the DOS endline remover
if ! DOS2UNIX=$(which dos2unix || which fromdos); then
    echo "Can't find dos2unix or fromdos! Must exit!" >&2
    exit 1
fi

# Check for sed
if ! which sed > /dev/null; then
    echo "Can't find sed!" >&2
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
    PROCESSED="${PROCESSED} ${1}"
    echo
    echo "Processing file: ${FILETOPROCESS}"
}

RemoveDosEndlines() {
    echo "\tRemoving DOS endlines with ${DOS2UNIX}"
    ${DOS2UNIX} ${FILETOPROCESS}
}

TrimTrailingWhitespace() {
    echo "\tTrimming trailing whitespace with sed"
    sed -i 's/[ \t]*$//' ${FILETOPROCESS}
}

# Greps for lines that aren't all whitespace and don't start with # as a comment marker
# such as in vrpn.cfg
CheckForUncommentedLines() {
    echo "\tChecking for un-commented, non-empty lines"
    if grep -v -E '^(#.*)?\s*$' ${FILETOPROCESS}; then
        echo "\t\tOops - you have uncommented lines (see above) - don't commit ${FILETOPROCESS}"
        SUCCEEDED=false
    else
        echo "\t\tNo un-commented lines found - OK to commit ${FILETOPROCESS}!"
    fi

}

# Open a subshell in which we'll change directories to VRPN root to process
# files relative to there.
(
    cd ${VRPN}

    StartProcessingFile server_src/vrpn.cfg
    RemoveDosEndlines
    #TrimTrailingWhitespace
    CheckForUncommentedLines
)

echo

echo "Processed:"
for fn in ${PROCESSED}; do
    echo "\t${fn}"
done

if ${SUCCEEDED}; then
    echo "Cleanups complete and all checks OK!"
else
    echo "One or more errors/warnings - see above!"
    exit 1
fi
