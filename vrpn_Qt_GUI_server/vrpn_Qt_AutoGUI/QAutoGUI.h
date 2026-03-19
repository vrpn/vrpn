/*=========================================================================

  Name:         QAutoGUI.h

  Author:       David Borland, EventLab

  Description:  Widget for generating a simple Qt GUI.  Widgets can be added
                manually, or via an XML description.

                Widgets currently supported:
                    
                    QPushButton
                    QCheckBox
                    QRadioButton

                    QSpinBox
                    QDoubleSpinBox
                    QDial
                    QScrollBar
                    QSlider

                Widgets are added to the current column with AddWidget(), 
                and columns can be added with AddColumn().

=========================================================================*/


#ifndef QAUTOGUI_H
#define QAUTOGUI_H


#include <QWidget>

class QBoxLayout;

class QAutoGUI : public QWidget {
    Q_OBJECT

public:
    // Constructor
    QAutoGUI(QWidget* parent = NULL);

    // Add widgets described in the given XML file
    bool ParseXML(const QString& fileName);

    // Add an individual widget
    void AddWidget(QWidget* widget);

    // Add a column
    void AddColumn(const QString& text = NULL);

    // Finish building the GUI
    void Finish();

protected:
    QBoxLayout* horizontal;
    QBoxLayout* vertical;
};


#endif
