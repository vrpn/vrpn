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
    ${DOS2UNIX} ${FILETOPROCESS} > /dev/null
}

AddDosEndlines() {
    StatusMessage "Adding DOS endlines with ${UNIX2DOS}"
    ${UNIX2DOS} ${FILETOPROCESS} > /dev/null
}

TrimTrailingWhitespace() {
    StatusMessage "Trimming trailing whitespace with sed"
    sed -i 's/[ \t]*$//' ${FILETOPROCESS}
}

LeadingFourSpacesToTabs() {
    # sed tip from http://kvz.io/blog/2011/03/31/spaces-vs-tabs/
    StatusMessage "Converting leading four-space groups into tabs with sed"
    sed -i -e ':repeat; s/^\(\t*\)    /\1\t/; t repeat' ${FILETOPROCESS}
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

# Excludes submodules directory
FindFilesNamed() {
    find * -path submodules -prune -o -name "$1" -print
}

# Excludes submodules directory
FindFilesNamedCaseInsensitive() {
    find * -path submodules -prune -o -iname "$1" -print
}

###
# Main script - process files!
###

# Open a subshell in which we'll change directories to VRPN root to process
# files relative to there.
(
    cd ${VRPN}

    # Clean up this script first, but only if we're not on Windows
    if uname | grep "_NT-" >/dev/null; then
        StatusMessage "Skipping script self-clean because Windows was detected"
    else
        StartProcessingFile ${SCRIPT}
        RemoveDosEndlines
        TrimTrailingWhitespace
        AddExecutablePrivilege
    fi

    # Git config file
    StartProcessingFile .gitmodules
    RemoveDosEndlines
    #TrimTrailingWhitespace  # Is this safe to do?
    RemoveExecutablePrivilege

    # Clean up all CMake files we control
    for fn in $(FindFilesNamed "CMakeLists.txt") *.cmake cmake/*.cmake submodules/*.cmake submodules/CMakeLists.txt; do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        LeadingFourSpacesToTabs
        TrimTrailingWhitespace
        RemoveExecutablePrivilege
    done

    # Clean up lots of files without trimming whitespace.
    (
        FindFilesNamed "Format"
        FindFilesNamed Doxyfile
        FindFilesNamed GNUmakefile
        FindFilesNamed Makefile.python
        FindFilesNamed package-list
        FindFilesNamed Readme
        FindFilesNamed set_instruments_for_sound_server
        FindFilesNamed \*.afm-plus
        FindFilesNamed \*.afm-plus_imd
        FindFilesNamed \*.c
        FindFilesNamed \*.C
        FindFilesNamed \*.Cdef
        FindFilesNamed \*.cfg
        FindFilesNamed \*.classpath
        FindFilesNamed \*.cmake_in
        FindFilesNamed \*.cpp
        FindFilesNamed \*.css
        FindFilesNamed \*.cvsignore
        FindFilesNamed \*.h
        FindFilesNamed \*.hdef
        FindFilesNamed \*.hpp
        FindFilesNamed \*.htm
        FindFilesNamed \*.html
        FindFilesNamed \*.i
        FindFilesNamed \*.imp
        FindFilesNamed \*.in
        FindFilesNamed \*.inf
        FindFilesNamed \*.java
        FindFilesNamed \*.jconf
        FindFilesNamed \*.mk
        FindFilesNamed \*.pl
        FindFilesNamed \*.prefs
        FindFilesNamed \*.project
        FindFilesNamed \*.properties
        FindFilesNamed \*.psf
        FindFilesNamed \*.py
        FindFilesNamed \*.qsp
        FindFilesNamed \*.rc
        FindFilesNamed \*.tcl
        FindFilesNamed \*.txt
        FindFilesNamed \*.ui
        FindFilesNamed \*.UNC
        FindFilesNamed \*.vmd
        FindFilesNamed \*.vrpndef
        FindFilesNamed \*.xml
        FindFilesNamedCaseInsensitive "ChangeLog"
        FindFilesNamedCaseInsensitive "Makefile"
        FindFilesNamedCaseInsensitive README\*
    ) | while read fn; do
        StartProcessingFile ${fn}
        RemoveDosEndlines
        RemoveExecutablePrivilege
    done

    #########################################################################
    # Clean up all .dsp and .dsw files. These should be DOS format when done.
    # Clean up all .sln and .vcproj files. These should be DOS format when done.
    # Clean up all .vcp and .vcw files. These should be DOS format when done.
    (
        FindFilesNamed \*.dsw
        FindFilesNamed \*.dsp
        FindFilesNamed \*.sln
        FindFilesNamed \*.vcproj
        FindFilesNamed \*.vcp
        FindFilesNamed \*.vcw
    ) | while read fn; do
        StartProcessingFile ${fn}
        AddDosEndlines
        RemoveExecutablePrivilege
    done

    # Server config file
    StartProcessingFile server_src/vrpn.cfg
    RemoveDosEndlines
    #TrimTrailingWhitespace  # Is this safe to do?
    RemoveExecutablePrivilege
    CheckForUncommentedLines
)

