This directory contains an example of a stand-alone VRPN server that can
be compiled separately from VRPN.  To use, install VRPN on your system
and then copy the contents of this directory to another location.  Use
CMake to construct a build environment and then compile the resulting
executable.

To make this work for a particular device, replace the @todo sections in
the code with your device-specific code.

This example is meant to run on a stand-alone embedded server that has
a specific device.  To make a driver for a device that can be used on the
same system along with others, follow the directions at:
https://github.com/vrpn/vrpn/wiki/Writing-a-device-driver

