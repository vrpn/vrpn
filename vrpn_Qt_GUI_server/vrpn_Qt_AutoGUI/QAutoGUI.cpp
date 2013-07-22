/*=========================================================================

  Name:         QAutoGUI.cpp

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


#include "QAutoGUI.h"

#include "QXmlAutoGUIHandler.h"

#include <QDebug>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QXmlDefaultHandler>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QVBoxLayout>


QAutoGUI::QAutoGUI(QWidget* parent) : QWidget(parent) {
    // Create the main layout for the widget
    horizontal = new QHBoxLayout(this);
    setLayout(horizontal);

    vertical = NULL;
}


bool QAutoGUI::ParseXML(const QString& fileName) {
    // Try to open the file
    QFile file(fileName);
    if (!file.exists()) {
        qDebug() << "QAutoGUI::ParseXML() : Error, vrpn_Qt_AutoGUI configuration file" << fileName
                 << "does not exist.";

        return false;
    }

    // Set up XML parsing
    QXmlAutoGUIHandler handler(this);

    QXmlSimpleReader xmlReader;
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    
    // Parse the GUI description
    QXmlInputSource source(file);
    xmlReader.parse(source);

    return true;
}

void QAutoGUI::AddWidget(QWidget* widget) {
    // Check if there is a column yet
    if (vertical == NULL) {
        AddColumn();
    }

    // Show the widget
    widget->setParent(this);
    vertical->addWidget(widget);
}

void QAutoGUI::AddColumn(const QString& title) {
    // If there is already a column, add a spacer at the end
    if (vertical) {
        QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);    
        vertical->addItem(spacer);
    }

    // Add a new column
    QGroupBox* groupBox = new QGroupBox(title, this);
    vertical = new QVBoxLayout(groupBox);
    groupBox->setLayout(vertical);

    horizontal->addWidget(groupBox);
}

void QAutoGUI::Finish() {
    // If there is a column, add a spacer at the end
    if (vertical) {
        QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);    
        vertical->addItem(spacer);
    }
}