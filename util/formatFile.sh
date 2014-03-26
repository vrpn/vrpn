#!/bin/sh -e
config="$(cd $(dirname $0) && pwd)/astyle.rc"

#echo -n "$@: "
astyle -n --options=${config} "$@"
