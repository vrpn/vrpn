#!/bin/sh -e
stylescript=$(cd $(dirname $0) && pwd)/formatFile.sh
filelist=$(cd $(dirname $0) && pwd)/sharedcode.txt
#stylescript="echo"

(
cd $(dirname $0) && cd ..

# This list is all the modules considered to be "common/shared" and thus
# normalized to a standard formatting
cat $filelist | while read module; do
    for ext in C h h.cmake_in; do
        if [ -f ${module}.${ext} ]; then
            "${stylescript}" ${module}.${ext}
        fi
    done
done
)

