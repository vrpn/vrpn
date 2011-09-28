#!/bin/csh
javac vrpn/*.java

javah -jni -classpath . vrpn.FunctionGeneratorRemote
javah -jni -classpath . vrpn.AnalogOutputRemote
javah -jni -classpath . vrpn.AnalogRemote
javah -jni -classpath . vrpn.AuxiliaryLoggerRemote
javah -jni -classpath . vrpn.ButtonRemote
javah -jni -classpath . vrpn.ForceDeviceRemote
javah -jni -classpath . vrpn.PoserRemote
javah -jni -classpath . vrpn.TextSender
javah -jni -classpath . vrpn.TextReceiver
javah -jni -classpath . vrpn.TrackerRemote
javah -jni -classpath . vrpn.VRPNDevice

jar -cvf vrpn.jar vrpn/
