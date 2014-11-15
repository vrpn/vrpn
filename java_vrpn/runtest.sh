#!/bin/sh
# Be sure to export CLASSPATH to include where vrpn.jar is, as well as
# and VRPN_JAVA_LIBDIR to indicate where libvrpn_java.so is.
echo -------------------------
echo $1
(
  cd test && \
  timeout -s9 5s java -Djava.library.path=${VRPN_JAVA_LIBDIR} $1
  ret=$?
  if [ ! $ret -eq 124 ]; then
    echo "Exited with code $ret"
    exit $ret
  fi
)
