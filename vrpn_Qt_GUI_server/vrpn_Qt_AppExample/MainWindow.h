/*=========================================================================

  Name:         MainWindow.h

  Author:       David Borland, EventLab

  Description:  Main window for vrpn_Qt_AppExample.

=========================================================================*/


#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>

#include "ui_MainWindow.h"


class MainWindow : public QMainWindow, private Ui_MainWindow {
    Q_OBJECT

public:
    // Constructor
    MainWindow(QWidget* parent = NULL);    
};

#endif
