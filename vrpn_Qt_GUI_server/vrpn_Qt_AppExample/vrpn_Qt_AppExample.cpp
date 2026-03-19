/*=========================================================================

  Name:         vrpn_Qt_AppExample.cpp

  Author:       David Borland, EventLab

  Description:  Contains the main function for vrpn_Qt_AppExample. 

                This is an example of using a GUI created with Qt
                Designer with vrpn_Qt.

=========================================================================*/


#include <QApplication>

#include "vrpn_Qt.h"
#include "MainWindow.h"

#include <QDebug>
#include <QString>


int main(int argc, char** argv) {
    // Initialize Qt
    QApplication app(argc, argv);


    // Set default server name and port
    char* name = "qt";
    int port = vrpn_DEFAULT_LISTEN_PORT_NO;
    
    // Parse the command line to override defaults
    for (int i = 1; i < argc; i++) {        
        if (QString(argv[i]) == "-serverName") {            
            name = argv[++i];
        }
        else if (QString(argv[i]) == "-vrpnPort") {            
            port = QString(argv[++i]).toInt();
        }
        else {
            qDebug() << "Error parsing command line.\n"
                << "Usage: vrpn_Qt_AppExample.exe [-serverName name] [-vrpnPort portNumber]";
        }
    }


    // Set up VRPN
    vrpn_Connection* connection = vrpn_create_server_connection(port);
    vrpn_Qt vq(name, connection);

    // Create the window
    MainWindow mainWindow;

    // Add widgets to the vrpn_Qt
    vq.AddChildren(&mainWindow);
    vq.PrintWidgets();

    // Show the window
    mainWindow.show();

    return app.exec();
}

