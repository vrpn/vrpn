#!/bin/sh -e
stylescript=$(cd $(dirname $0) && pwd)/formatFile.sh
#stylescript="echo"

(
cd $(dirname $0) && cd ..

# This list is all the modules considered to be "common/shared" and thus
# normalized to a standard formatting
while read module; do
    for ext in C h h.cmake_in; do
        if [ -f ${module}.${ext} ]; then
            ${stylescript} ${module}.${ext}
        fi
    done
done <<EOS
vrpn_Analog
vrpn_Analog_Output
vrpn_BaseClass
vrpn_Button
vrpn_Configure
vrpn_Connection
vrpn_Dial
vrpn_FileConnection
vrpn_FileController
vrpn_ForceDevice
vrpn_ForceDeviceServer
vrpn_Forwarder
vrpn_ForwarderController
vrpn_HumanInterface
vrpn_MainloopContainer
vrpn_MainloopObject
vrpn_MessageMacros
vrpn_Mutex
vrpn_Poser
vrpn_Poser_Analog
vrpn_RedundantTransmission
vrpn_SendTextMessageStreamProxy
vrpn_Serial
vrpn_SerialPort
vrpn_Shared
vrpn_Sound
vrpn_Text
vrpn_Tracker
vrpn_Types
server_src/vrpn
server_src/vrpn_Generic_server_object

EOS
)

