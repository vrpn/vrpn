#!/usr/bin/csh
javac *.java
/bin/cp -f *.java vrpn/
/bin/mv -f *.class vrpn/

javah -jni -classpath . vrpn.TrackerRemote
javah -jni -classpath . vrpn.AnalogRemote
javah -jni -classpath . vrpn.AnalogOutputRemote
javah -jni -classpath . vrpn.ButtonRemote
javah -jni -classpath . vrpn.ForceDeviceRemote
javah -jni -classpath . vrpn.TempImagerRemote

jar -cvf vrpn.jar vrpn/
