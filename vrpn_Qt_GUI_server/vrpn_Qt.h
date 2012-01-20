/*=============================================================================================

  Name:         vrpn_Qt.h

  Author:       David Borland, EventLab

  Description:  The vrpn_Qt class.  
  
                This class is designed to enable remote control of an application using VRPN 
                for communication.  
               
                All widgets derived from QAbstractButton are supported with a VRPN button 
                device, and all widgets derived from QAbstractSlider, along with 
                QDoubleSpinBox and QSpinBox, are supported as channels of a VRPN analog device.

                Widgets can be added individually using AddWidget(), or all children of a 
                widget can be added using AddChildren.
               
=============================================================================================*/


#ifndef VRPN_QT_H
#define VRPN_QT_H


#include <QObject>

#include <vrpn_Analog.h>
#include <vrpn_Button.h>

class QAbstractButton;
class QWidget;


class vrpn_Qt : public QObject, public vrpn_Button_Filter, public vrpn_Analog {
    Q_OBJECT

public:
    //
    // Constructor
    //
    // name:        VRPN device name
    //
    // c:           VRPN connection to use
    //
    // parent:      Qt parent object
    //
    // updateRate:  Interval of QTimer used to call mainloop(), in milliseconds
    //
    vrpn_Qt(const char* name, vrpn_Connection* c, QObject* parent = NULL, int updateRate = 10);

    // Add widget
    void AddWidget(QWidget* widget);

    // Add all child widgets of this widget
    void AddChildren(QWidget* widget);

    // Print widget information
    void PrintWidgets();

    // Resets the "last" values to force report_changes() to send a report of the current state.
    // Used to send send current state to newly-connected clients.
    void ResetLast();

public slots:
    // Widget changes
    void ButtonChanged();
    void AnalogChanged();

    // Instantiate VRPN pure virtual method
    virtual void mainloop();

protected:
    // Holds widgets for all button types
    QList<QAbstractButton*> buttonWidgets;

    // Holds widgets for all analog types
    QList<QWidget*> analogWidgets;
};


#endif
