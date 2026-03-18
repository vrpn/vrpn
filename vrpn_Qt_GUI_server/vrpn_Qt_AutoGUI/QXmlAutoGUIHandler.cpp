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
        display->setObjectName("vrpn_Qt_ignore");
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


bool QXmlAutoGUIHandler::parse(QIODevice* device) {
    QXmlStreamReader xml(device);

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QStringView name = xml.name();

            // Top-level tag, do nothing
            if (name == u"AUTOGUI") continue;

            processElement(name, xml.attributes());
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML parse error at line" << xml.lineNumber()
                    << ", column" << xml.columnNumber() << ":"
                    << xml.errorString();
        return false;
    }

    // Finish up the GUI
    gui->Finish();

    return true;
}


void QXmlAutoGUIHandler::processElement(const QStringView& qName, const QXmlStreamAttributes& atts) {
    // Add a column
    if (qName == u"Column") {
        QString text;
        for (const auto& att : atts) {
            if (att.name() == u"Text") {
                text = att.value().toString();
            }
            else {
                qWarning() << "Unsupported attribute: " << att.name();
            }
        }

        gui->AddColumn(text);
        return;
    }


    // Add a horizontal line
    if (qName == u"Line") {
        // Use a frame with some specific properties set to make it a line.
        // This is how Qt Designer does it...
        QFrame* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        gui->AddWidget(line);
        return;
    }


    // Create the appropriate widget
    if (qName == u"PushButton") {
        // Create a push button
        QPushButton* widget = new QPushButton();

        ProcessPushButton(widget, atts);
    }
    else if (qName == u"CheckBox") {
        // Create a check box
        QCheckBox* widget = new QCheckBox();

        ProcessCheckableButton(widget, atts);
    }
    else if (qName == u"RadioButton") {
        // Create a radio button
        QRadioButton* widget = new QRadioButton();

        ProcessCheckableButton(widget, atts);
    }
    else if (qName == u"SpinBox") {
        // Create a spin box
        QSpinBox* widget = new QSpinBox();

        ProcessSpinBox(widget, atts);
    }
    else if (qName == u"DoubleSpinBox") {
        // Create a double spin box
        QDoubleSpinBox* widget = new QDoubleSpinBox();

        ProcessDoubleSpinBox(widget, atts);
    }
    else if (qName == u"Dial") {
        // Create a dial
        QDial* widget = new QDial();
        widget->setNotchesVisible(true);

        ProcessSliderWidget(widget, atts);
    }
    else if (qName == u"ScrollBar") {
        QScrollBar* widget = new QScrollBar(Qt::Horizontal);

        ProcessSliderWidget(widget, atts);
    }
    else if (qName == u"Slider") {
        QSlider* widget = new QSlider(Qt::Horizontal);
        widget->setTickPosition(QSlider::TicksBelow);

        ProcessSliderWidget(widget, atts);
    }
    else {
        qWarning() << "Unsupported widget type: " << qName;
    }
}


void QXmlAutoGUIHandler::ProcessPushButton(QAbstractButton* widget, const QXmlStreamAttributes& atts) {
    // Check attributes
    for (const auto& att : atts) {
        if (att.name() == u"Name") {
            widget->setObjectName(att.value().toString());
        }
        else if (att.name() == u"Text") {
            widget->setText(att.value().toString());
        }
        else {
            qWarning() << "Unsupported attribute: " << att.name();
        }
    }

    // Add the widget
    gui->AddWidget(widget);
}

void QXmlAutoGUIHandler::ProcessCheckableButton(QAbstractButton* widget, const QXmlStreamAttributes& atts) {
    // Check attributes
    for (const auto& att : atts) {
        if (att.name() == u"Name") {
            widget->setObjectName(att.value().toString());
        }
        else if (att.name() == u"Text") {
            widget->setText(att.value().toString());
        }
        else if (att.name() == u"Checked") {
            widget->setChecked(att.value().toInt());
        }
        else {
            qWarning() << "Unsupported attribute: " << att.name();
        }
    }

    // Add the widget
    gui->AddWidget(widget);
}

void QXmlAutoGUIHandler::ProcessSliderWidget(QAbstractSlider* widget, const QXmlStreamAttributes& atts) {
    // Set this so the scrollbar can expand
    widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    // Check attributes
    QString text;
    for (const auto& att : atts) {
        if (att.name() == u"Name") {
            widget->setObjectName(att.value().toString());
        }
        else if (att.name() == u"Text") {
            text = att.value().toString();
        }
        else if (att.name() == u"Min") {
            widget->setMinimum(att.value().toInt());
        }
        else if (att.name() == u"Max") {
            widget->setMaximum(att.value().toInt());
        }
        else if (att.name() == u"Value") {
            widget->setValue(att.value().toInt());
        }
        else if (att.name() == u"Step") {
            widget->setSingleStep(att.value().toInt());
        }
        else {
            qWarning() << "Unsupported attribute: " << att.name();
        }
    }

    // Add to a container and add the widget
    QAnalogContainer* container = new QAnalogContainer(widget, text);
    gui->AddWidget(container);
}

void QXmlAutoGUIHandler::ProcessSpinBox(QSpinBox* widget, const QXmlStreamAttributes& atts)  {
    // Check attributes
    QString text;
    for (const auto& att : atts) {
        if (att.name() == u"Name") {
            widget->setObjectName(att.value().toString());
        }
        else if (att.name() == u"Text") {
            text = att.value().toString();
        }
        else if (att.name() == u"Min") {
            widget->setMinimum(att.value().toInt());
        }
        else if (att.name() == u"Max") {
            widget->setMaximum(att.value().toInt());
        }
        else if (att.name() == u"Value") {
            widget->setValue(att.value().toInt());
        }
        else if (att.name() == u"Step") {
            widget->setSingleStep(att.value().toInt());
        }
        else {
            qWarning() << "Unsupported attribute: " << att.name();
        }
    }

    // Add to a container and add the widget
    QAnalogContainer* container = new QAnalogContainer(widget, text);
    gui->AddWidget(container);
}

void QXmlAutoGUIHandler::ProcessDoubleSpinBox(QDoubleSpinBox* widget, const QXmlStreamAttributes& atts)  {
    // Check attributes
    QString text;
    for (const auto& att : atts) {
        if (att.name() == u"Name") {
            widget->setObjectName(att.value().toString());
        }
        else if (att.name() == u"Text") {
            text = att.value().toString();
        }
        else if (att.name() == u"Min") {
            widget->setMinimum(att.value().toDouble());
        }
        else if (att.name() == u"Max") {
            widget->setMaximum(att.value().toDouble());
        }
        else if (att.name() == u"Value") {
            widget->setValue(att.value().toDouble());
        }
        else if (att.name() == u"Step") {
            widget->setSingleStep(att.value().toDouble());
        }
        else {
            qWarning() << "Unsupported attribute: " << att.name();
        }
    }

    // Add to a container and add the widget
    QAnalogContainer* container = new QAnalogContainer(widget, text);
    gui->AddWidget(container);
}
