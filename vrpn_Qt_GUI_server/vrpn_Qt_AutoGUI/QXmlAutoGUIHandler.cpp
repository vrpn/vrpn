/*=========================================================================

  Name:        QXmlAutoGUIHandler.cpp

  Author:      David Borland, EventLab

  Description: XML parser for QAutoGUI.

=========================================================================*/


#include "QXmlAutoGUIHandler.h"

#include "QAutoGUI.h"

#include <QCheckBox>
#include <QDebug>
#include <QDial>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>


///////////////////////////////////////////////////////////////////////////


QAnalogContainer::QAnalogContainer(QWidget* widget, const QString& text) {
    widget->setParent(this);

    // Create the label
    QLabel* label = new QLabel(text, this);

    // Use a horizontal layout
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(label);
    layout->addWidget(widget);

    // Add a numerical display for things that need it
    if (qobject_cast<QAbstractSlider*>(widget)) {
        QAbstractSlider* slider = qobject_cast<QAbstractSlider*>(widget);

        // Use a spin box without buttons
        QSpinBox* display = new QSpinBox(this);
        display->setName("vrpn_Qt_ignore");
        display->setButtonSymbols(QAbstractSpinBox::NoButtons);

        // Set to the same value and range as the widget
        display->setValue(slider->value());
        display->setRange(display->minimum(), display->maximum());

        // Hook them together
        connect(slider, SIGNAL(valueChanged(int)), display, SLOT(setValue(int)));
        connect(display, SIGNAL(valueChanged(int)), slider, SLOT(setValue(int)));

        // Add to the layout
        layout->addWidget(display);
    }
    
    layout->addStretch();

    setLayout(layout);
}


///////////////////////////////////////////////////////////////////////////


QXmlAutoGUIHandler::QXmlAutoGUIHandler(QAutoGUI* autoGui) : gui(autoGui) {
}


bool QXmlAutoGUIHandler::startElement(const QString& namespaceURI, const QString& localName, 
                                      const QString& qName, const QXmlAttributes& atts) {
    // Top-level tag, do nothing
    if (qName == "AUTOGUI") return true;


    // Add a column
    if (qName == "Column") {
        QString text;
        for (int i = 0; i < atts.count(); i++) {
            if (atts.qName(i) == "Text") {
                text = atts.value(i);
            }
            else {
                qWarning() << "Unsupported attribute: " << atts.qName(i);
            }
        }

        gui->AddColumn(text);

        return true;
    }


    // Add a horizontal line
    if (qName == "Line") {
        // Use a frame with some specific properties set to make it a line.  
        // This is how Qt Designer does it...
        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        gui->AddWidget(line);

        return true;
    }


    // Create the appropriate widget
    if (qName == "PushButton") {        
        // Create a push button
        QPushButton* widget = new QPushButton();

        ProcessPushButton(widget, atts);
    }
    else if (qName == "CheckBox") {
        // Create a check box
        QCheckBox* widget = new QCheckBox();

        ProcessCheckableButton(widget, atts);
    }
    else if (qName == "RadioButton") {
        // Create a check box
        QRadioButton* widget = new QRadioButton();

        ProcessCheckableButton(widget, atts);
    }
    else if (qName == "SpinBox") {
        // Create a spin box
        QSpinBox* widget = new QSpinBox();

        ProcessSpinBox(widget, atts);
    }    
    else if (qName == "DoubleSpinBox") {
        // Create a spin box
        QDoubleSpinBox* widget = new QDoubleSpinBox();

        ProcessDoubleSpinBox(widget, atts);
    }
    else if (qName == "Dial") {
        // Create a spin box
        QDial* widget = new QDial();
        widget->setNotchesVisible(true);

        ProcessSliderWidget(widget, atts);
    }
    else if (qName == "ScrollBar") {
        QScrollBar* widget = new QScrollBar(Qt::Horizontal);

        ProcessSliderWidget(widget, atts);
    }
    else if (qName == "Slider") {
        QSlider* widget = new QSlider(Qt::Horizontal);
        widget->setTickmarks(QSlider::Below);

        ProcessSliderWidget(widget, atts);
    }
    else {
        qWarning() << "Unsupported widget type: " << qName;
    }

    return true;
}

bool QXmlAutoGUIHandler::endDocument() {
    // Finish up the GUI
    gui->Finish();

    return true;
}

bool QXmlAutoGUIHandler::fatalError(const QXmlParseException& exception) {
    // Print a fatal error message
    qWarning()  << "Fatal error on line" << exception.lineNumber()
                << ", column" << exception.columnNumber() << ":"
                << exception.message();

    return false;
}


void QXmlAutoGUIHandler::ProcessPushButton(QAbstractButton* widget, const QXmlAttributes& atts) {
    // Check attributes
    for (int i = 0; i < atts.count(); i++) {
        if (atts.qName(i) == "Name") {
            widget->setName(atts.value(i));
        }
        else if (atts.qName(i) == "Text") {
            widget->setText(atts.value(i));
        }
        else {
            qWarning() << "Unsupported attribute: " << atts.qName(i);
        }
    }

    // Add the widget
    gui->AddWidget(widget);
}
void QXmlAutoGUIHandler::ProcessCheckableButton(QAbstractButton* widget, const QXmlAttributes& atts) {
    // Check attributes
    for (int i = 0; i < atts.count(); i++) {
        if (atts.qName(i) == "Name") {
            widget->setName(atts.value(i));
        }
        else if (atts.qName(i) == "Text") {
            widget->setText(atts.value(i));
        }            
        else if (atts.qName(i) == "Checked") {
            widget->setChecked(atts.value(i).toInt());
        }
        else {
            qWarning() << "Unsupported attribute: " << atts.qName(i);
        }
    }

    // Add the widget
    gui->AddWidget(widget);
}

void QXmlAutoGUIHandler::ProcessSliderWidget(QAbstractSlider* widget, const QXmlAttributes& atts) {
    // Set this so the scrollbar can expand
    widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    // Check attributes
    QString text;
    for (int i = 0; i < atts.count(); i++) {
        if (atts.qName(i) == "Name") {
            widget->setName(atts.value(i));
        }
        else if (atts.qName(i) == "Text") {
            text = atts.value(i);
        }            
        else if (atts.qName(i) == "Min") {
            widget->setMinimum(atts.value(i).toInt());
        }            
        else if (atts.qName(i) == "Max") {
            widget->setMaximum(atts.value(i).toInt());
        }
        else if (atts.qName(i) == "Value") {
            widget->setValue(atts.value(i).toInt());
        }
        else if (atts.qName(i) == "Step") {
            widget->setSingleStep(atts.value(i).toInt());
        }
        else {
            qWarning() << "Unsupported attribute: " << atts.qName(i);
        }
    }

    // Add to a container and add the widget
    QAnalogContainer* container = new QAnalogContainer(widget, text);
    gui->AddWidget(container);
}

void QXmlAutoGUIHandler::ProcessSpinBox(QSpinBox* widget, const QXmlAttributes& atts)  {
    // Check attributes
    QString text;
    for (int i = 0; i < atts.count(); i++) {
        if (atts.qName(i) == "Name") {
            widget->setName(atts.value(i));
        }
        else if (atts.qName(i) == "Text") {
            text = atts.value(i);
        }              
        else if (atts.qName(i) == "Min") {
            widget->setMinimum(atts.value(i).toInt());
        }            
        else if (atts.qName(i) == "Max") {
            widget->setMaximum(atts.value(i).toInt());
        }
        else if (atts.qName(i) == "Value") {
            widget->setValue(atts.value(i).toInt());
        }
        else if (atts.qName(i) == "Step") {
            widget->setSingleStep(atts.value(i).toInt());
        }
        else {
            qWarning() << "Unsupported attribute: " << atts.qName(i);
        }
    }

    // Add to a container and add the widget
    QAnalogContainer* container = new QAnalogContainer(widget, text);
    gui->AddWidget(container);
}

void QXmlAutoGUIHandler::ProcessDoubleSpinBox(QDoubleSpinBox* widget, const QXmlAttributes& atts)  {
    // Check attributes
    QString text;
    for (int i = 0; i < atts.count(); i++) {
        if (atts.qName(i) == "Name") {
            widget->setName(atts.value(i));
        }
        else if (atts.qName(i) == "Text") {
            text = atts.value(i);
        }           
        else if (atts.qName(i) == "Min") {
            widget->setMinimum(atts.value(i).toDouble());
        }            
        else if (atts.qName(i) == "Max") {
            widget->setMaximum(atts.value(i).toDouble());
        }
        else if (atts.qName(i) == "Value") {
            widget->setValue(atts.value(i).toDouble());
        }
        else if (atts.qName(i) == "Step") {
            widget->setSingleStep(atts.value(i).toDouble());
        }
        else {
            qWarning() << "Unsupported attribute: " << atts.qName(i);
        }
    }

    // Add to a container and add the widget
    QAnalogContainer* container = new QAnalogContainer(widget, text);
    gui->AddWidget(container);
}