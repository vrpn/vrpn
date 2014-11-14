#!/bin/sh
# Be sure to export CLASSPATH to include where vrpn.jar is, as well as
# and VRPN_JAVA_LIBDIR to indicate where libvrpn_java.so is.
(
  cd $(dirname $0)
  export CLASSPATH=$CLASSPATH:$(cd test && pwd)
  (cd test && ls *Test.class |sed s/.class//) | xargs -n 1 ./runtest.sh
)
