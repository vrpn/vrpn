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

# Pick the DOS endline adder
if ! UNIX2DOS=$(which unix2dos || which todos); then
    echo "Can't find unix2dos or todos! Must exit!" >&2
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
    ${DOS2UNIX} -q ${FILETOPROCESS}
}

AddDosEndlines() {
    StatusMessage "Adding DOS endlines with ${UNIX2DOS}"
    ${UNIX2DOS} -q ${FILETOPROCESS}
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

    # Git config file
    StartProcessingFile .gitmodules
    RemoveDosEndlines
    #TrimTrailingWhitespace  # Is this safe to do?
    RemoveExecutablePrivilege

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

    # Clean up all ChangeLog files.  Don't trim whitespace from these.
    for fn in $(find . -name "ChangeLog"); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .in and Format files.  Don't trim whitespace from these.
    for fn in $(find . -name \*.in) $(find . -name "Format"); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all Makefile and README files.  Don't trim whitespace from these.
    for fn in $(find . -name "Makefile") $(find . -name README\*); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .C and .cpp files.  Don't trim whitespace from these.
    for fn in $(find . -name \*.C) $(find . -name \*.cpp); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all and .hpp files.  Don't trim whitespace from these.
    for fn in $(find . -name \*.hpp); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .c and .h files.  Don't trim whitespace from these.
    for fn in $(find . -name \*.c) $(find . -name \*.h); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .classpath and .xml files.  Don't trim whitespace from these.
    for fn in $(find . -name \*.classpath) $(find . -name \*.xml); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .project and .css files.  Don't trim whitespace from these.
    for fn in $(find . -name \*.project) $(find . -name \*.css); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .properties and .cfg files. Don't trim whitespace from these.
    for fn in $(find . -name \*.properties) $(find . -name \*.cfg); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .java and .html files. Don't trim whitespace from these.
    for fn in $(find . -name \*.java) $(find . -name \*.html); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .htm and .txt files. Don't trim whitespace from these.
    for fn in $(find . -name \*.htm) $(find . -name \*.txt); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .psf and .vmd files. Don't trim whitespace from these.
    for fn in $(find . -name \*.psf) $(find . -name \*.vmd); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .inf and .ui files. Don't trim whitespace from these.
    for fn in $(find . -name \*.inf) $(find . -name \*.ui); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .cvsignore and package-list files. Don't trim whitespace from these.
    for fn in $(find . -name \*.cvsignore) $(find . -name package-list); do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    #########################################################################
    # Clean up all .dsp and .dsw files. These should be DOS format when done.
    for fn in $(find . -name \*.dsw) $(find . -name \*.dsp); do
        StartProcessingFile ${fn}
        AddDosEndlines
        RemoveExecutablePrivilege
    done

    # Clean up all .sln and .vcproj files. These should be DOS format when done.
    for fn in $(find . -name \*.sln) $(find . -name \*.vcproj); do
        StartProcessingFile ${fn}
        AddDosEndlines
        RemoveExecutablePrivilege
    done

)

