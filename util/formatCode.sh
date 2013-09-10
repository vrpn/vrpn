#!/bin/sh -ex
stylescript=$(cd $(dirname $0) && pwd)/formatFile.sh
(
cd $(dirname $0) && cd ..
# Find all source files, excluding the submodules directory.
find . \( \
    -path ./submodules -o \
    -path ./cmake \
    \)  -prune -o \( \
    -name "*.cpp" -o \
    -name "*.h" -o \
    -name "*.C" -o \
    -name "*.c" \) -print0 | xargs -n 1 -0 ${stylescript}
)

