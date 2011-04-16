This tool was submitted by David Borland.  It lets you create a Qt-based GUI whose widgets control the values for the buttons and analogs from a VRPN server.  It is configurable with an XML file, so that you can generate custom GUI controls for your VR application.

You create the makefiles to compile the code in this directory using CMake.  The current build has been tested only on MingGW on Windows; it was developed for Visual Studio.

The VRPN library must first have been made and installed (this will also build and install the quat library).  These should then be found by CMake when you configure it.

You will probably need to point the configuration at the QMake executable that you want to use.

It will report an error at the end of the build that it cannot copy the file to an external directory; don't worry, this is not needed.  The vrpn_Qt_GUI.exe in the bin/ directory under the build (for MinGW, Release/ or Debug/ for Visual Studio) is ready to use.

You will either need to have all of the Qt DLLs in your path or copy them into the executable directory so that you can run the program.

You also need to have a vrpn_Qt_GUI.xml file in the same directory as the executable; it tells what GUI elements should be created.  You can find an example configuration file in the vrpn_Qt_GUI subdirectory.  You can use the -xmlFile command-line argument to select a different file.

The device name to connect to if you are running on the same host is "qt@localhost".  This device has 5 buttons (0 through 4) and 8 analogs (0 through 7).  The console window will show you which GUI components are mapped to which channels (this mapping is done in the XML file).  The example XML file has Button0-Button4 and Analog0-Analog7 (but analogs 3, 5, and 7 are not used).  You can use the -serverName command-line argument to change the name of the local server.

