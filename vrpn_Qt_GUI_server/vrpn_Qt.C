/*=============================================================================================

  Name:         vrpn_Qt.C

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


#include "vrpn_Qt.h"

#include <QTimer>

// Buttons
#include <QAbstractButton>

// Analogs
#include <QAbstractSlider>
#include <QDoubleSpinBox>
#include <QSpinBox> 


// Callback for new connections
int VRPN_CALLBACK handle_gotConnection(void* userData, vrpn_HANDLERPARAM) {
    vrpn_Qt* server = static_cast<vrpn_Qt*>(userData);

    // Reset "last" values so report_changes() will send a report
    server->ResetLast();

    return 0;
}


vrpn_Qt::vrpn_Qt(const char* name, vrpn_Connection* c, QObject* parent, int updateRate) 
: QObject(parent), vrpn_Button_Filter(name, c), vrpn_Analog(name, c) {
    // Create a timer to call mainloop
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(mainloop()));
    timer->start(updateRate);

    // Create a callback for new connections
    vrpn_int32 gotConnection = c->register_message_type(vrpn_got_connection);
    c->register_handler(gotConnection, handle_gotConnection, this);
}


void vrpn_Qt::AddWidget(QWidget* widget) {
    // Ignore widgets called vrpn_Qt_ignore
    if (widget->objectName() == "vrpn_Qt_ignore") {
        return;
    }


    // Check if widget is derived from abstract button
    if (qobject_cast<QAbstractButton*>(widget)) {
        QAbstractButton* button = qobject_cast<QAbstractButton*>(widget);

        connect(button, SIGNAL(pressed()), this, SLOT(ButtonChanged()));
        connect(button, SIGNAL(released()), this, SLOT(ButtonChanged()));

        buttonWidgets.append(button);
        buttons[num_buttons] = button->isChecked();
        num_buttons++;
    }
    // Check if widget is derived from abstract slider
    else if (qobject_cast<QAbstractSlider*>(widget)) {
        QAbstractSlider* slider = qobject_cast<QAbstractSlider*>(widget);

        connect(slider, SIGNAL(valueChanged(int)), this, SLOT(AnalogChanged()));

        analogWidgets.append(slider);
        channel[num_channel] = slider->value();
        num_channel++;
    }
    // Check for double spin box
    else if (qobject_cast<QDoubleSpinBox*>(widget)) {
        QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget);

        connect(doubleSpinBox, SIGNAL(valueChanged(double)), this, SLOT(AnalogChanged()));

        analogWidgets.append(doubleSpinBox);
        channel[num_channel] = doubleSpinBox->value();
        num_channel++;
    }    
    // Check for spin box
    else if (qobject_cast<QSpinBox*>(widget)) {
        QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget);

        connect(spinBox, SIGNAL(valueChanged(int)), this, SLOT(AnalogChanged()));

        analogWidgets.append(spinBox);
        channel[num_channel] = spinBox->value();
        num_channel++;
    }


    // Sanity check
    if (buttonWidgets.size() != num_buttons) {
        fprintf(stderr, "Warning: number of Qt buttons and VRPN buttons differ!\n");
    }

    // Sanity check
    if (analogWidgets.size() != num_channel) {
        fprintf(stderr, "Warning: number of Qt analog widgets and VRPN analog channels differ!\n");
    }
}

void vrpn_Qt::AddChildren(QWidget* widget) {
    // Try to add each child of the input widget
    QList<QWidget*> list = widget->findChildren<QWidget*>();
    foreach(QWidget* w, list) {
        AddWidget(w);
    }
}


void vrpn_Qt::PrintWidgets() {
    // Print buttons
    for (int i = 0; i < buttonWidgets.size(); i++) {
        printf("Button %d: %s\n", i, buttonWidgets[i]->objectName().toLatin1().constData());
    }

    printf("\n");

    // Print analogs
    for (int i = 0; i < analogWidgets.size(); i++) {
        printf("Analog %d: %s\n", i, analogWidgets[i]->objectName().toLatin1().constData());
    }
}


void vrpn_Qt::ResetLast() {
    for (int i = 0; i < num_buttons; i++) {
        lastbuttons[i] = !buttons[i];
    }
    for (int i = 0; i < num_channel; i++) {
        last[i] = channel[i] - 1.0;
    }
}


void vrpn_Qt::ButtonChanged() {
    // Update all VRPN buttons from widgets because we don't know what button this is
    for (int i = 0; i < num_buttons; i++) {
        buttons[i] = buttonWidgets[i]->isChecked() || buttonWidgets[i]->isDown();
    }
}

void vrpn_Qt::AnalogChanged() {
    // Update all VRPN analog channels from widgets because we don't know what spin box this is
    for (int i = 0; i < num_channel; i++) {
        // Cast to the correct widget type and set the value
        if (qobject_cast<QAbstractSlider*>(analogWidgets[i])) {
            QAbstractSlider* slider = qobject_cast<QAbstractSlider*>(analogWidgets[i]);
            channel[i] = slider->value();
        }
        else if (qobject_cast<QDoubleSpinBox*>(analogWidgets[i])) {
            QDoubleSpinBox* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(analogWidgets[i]);
            channel[i] = doubleSpinBox->value();
        }     
        else if (qobject_cast<QSpinBox*>(analogWidgets[i])) {
            QSpinBox* spinBox = qobject_cast<QSpinBox*>(analogWidgets[i]);
            channel[i] = spinBox->value();
        }
    }
}


void vrpn_Qt::mainloop() {
    // Call the connection mainloop
    d_connection->mainloop();

    // Call the generic server mainloop
    server_mainloop();

    // Get a time stamp
    struct timeval current_time;
    vrpn_gettimeofday(&current_time, NULL);

    // Set the time stamp for each device type
    vrpn_Button_Filter::timestamp.tv_sec = current_time.tv_sec;
    vrpn_Button_Filter::timestamp.tv_usec = current_time.tv_usec;
    vrpn_Analog::timestamp.tv_sec = current_time.tv_sec;
    vrpn_Analog::timestamp.tv_usec = current_time.tv_usec;

    // Send updates
   vrpn_Button_Filter::report_changes();
   vrpn_Analog::report_changes();
}
