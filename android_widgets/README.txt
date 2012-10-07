This project is totally distinct from java_vrpn and the Vrpn server running on Android.

The Vrpn widgets for Android provide a set of Android Widgets that are bound to a Vrpn server. On the Android device, an application can be written mainly with Android XML layout, almost without Java coding. The documentation of the widgets is in android_widgets/vrpn_library/doc/

The Android application will send updates as UDP packets with a JSON payload to the vrpn_Tracker_JsonNet device. You need a VRPN server build with VRPN_USE_JSONNET and a vrpn.cfg file that declares a vrpn_Tracker_JsonNet device.

To build such a server you need JSONCPP. See README.jsoncpp for instructions about obtaining and building JSONCPP first, then use CMAKE to generate the VRPN project with VRPN_USE_JSONNET. You will need to define the JSONCPP_ROOT_DIR CMake variable.

Note : 
	the JSONNET server was built and tested only on Windows 7 32 bits, MSVC 2008

Philippe Crassous / Ensam ParisTech - Institut Image / 2011
p.crassous@free.fr

