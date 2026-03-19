/*=========================================================================

  Name:         vrpn_Qt_GUI.cpp

  Author:       David Borland, EventLab

  Description:  Contains the main function for vrpn_Qt_GUI. 

                This is an example of generating a GUI from a simple XML
                file using QAutoGUI.

=========================================================================*/


#include <QApplication>

#include "vrpn_Qt.h"
#include "QAutoGUI.h"

#include <QDebug>
#include <QString>


int main(int argc, char** argv) {
    // Initialize Qt
    QApplication app(argc, argv);


    // Set default xml file, server name, and port
    QString xmlFile("vrpn_Qt_AutoGUI.xml");
    char* name = "qt";
    int port = vrpn_DEFAULT_LISTEN_PORT_NO;
    
    // Parse the command line to override defaults
    for (int i = 1; i < argc; i++) {
        if (QString(argv[i]) == "-xmlFile") {
            xmlFile = argv[++i];
        }
        else if (QString(argv[i]) == "-serverName") {            
            name = argv[++i];
        }
        else if (QString(argv[i]) == "-vrpnPort") {            
            port = QString(argv[++i]).toInt();
        }
        else {
            qDebug() << "Error parsing command line.\n"
                << "Usage: vrpn_Qt_GUI.exe [-xmlFile fileName] [-vrpnPort portNumber]";
        }
    }


    // Set up VRPN
    vrpn_Connection* connection = vrpn_create_server_connection(port);
    vrpn_Qt vq(name, connection);

    // Create the GUI
    QAutoGUI gui;
    gui.ParseXML(xmlFile);

    // Add widgets to the vrpn_Qt
    vq.AddChildren(&gui);
    vq.PrintWidgets();
    
    // Show the widget
    gui.show();

    return app.exec();
}

