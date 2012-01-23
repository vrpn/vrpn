This tool was submitted by David Borland.  It lets you create a Qt-based GUI whose widgets control the values for the buttons and analogs from a VRPN server.  

There are two ways to use it:  
The first is to create a Qt GUI application using standard tools (i.e. Qt Designer).  An example of this method is found in the vrpn_Qt_AppExample folder.
The second is to use the vrpn_Qt_AutoGUI tool found in the vrpn_Qt_AutoGUI folder.  It is configurable with an XML file, so that you can generate custom GUI controls for your VR application quickly, and edit them easily.

You create the makefiles to compile the code in this directory using CMake.  The current build has been tested only on MingGW on Windows; it was developed for Visual Studio.

The VRPN library must first have been made and installed (this will also build and install the quat library).  These should then be found by CMake when you configure it.

You will probably need to point the configuration at the QMake executable that you want to use.

The executables in the bin/ directory under the build (for MinGW, Release/ or Debug/ for Visual Studio) are ready to use.

You will either need to have all of the Qt DLLs in your path or copy them into the executable directory so that you can run the program.

For the vrpn_Qt_AutoGUI, you also need to have a vrpn_Qt_GUI.xml file in the same directory as the executable; it tells what GUI elements should be created.  You can find an example configuration file in the vrpn_Qt_GUI subdirectory (this should be copied automatically to the bin directory).  You can use the -xmlFile command-line argument to select a different file.

The device name to connect to if you are running on the same host is "qt@localhost".  This device has 5 buttons (0 through 4) and 8 analogs (0 through 7).  The console window will show you which GUI components are mapped to which channels (this mapping is done in the XML file).  The example XML file has Button0-Button4 and Analog0-Analog7 (but analogs 3, 5, and 7 are not used).  You can use the -serverName command-line argument to change the name of the local server.

