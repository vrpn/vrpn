/*=========================================================================

  Name:         MainWindow.cpp

  Author:       David Borland, EventLab

  Description:  Main window for vrpn_Qt_AppExample. 

=========================================================================*/


#include "MainWindow.h"


// Constructor
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    // Create the GUI from the Qt Designer file
    setupUi(this);
}