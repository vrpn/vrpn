/*=========================================================================

  Name:         QXmlAutoGUIHandler.h

  Author:       David Borland, EventLab

  Description:  XML parser for QAutoGUI.

=========================================================================*/


#ifndef QXMLAUTOGUIHANDLER_H
#define QXMLAUTOGUIHANDLER_H


#include <QXmlStreamReader>
#include <QWidget>


class QAutoGUI;

class QAbstractButton;
class QAbstractSlider;
class QDoubleSpinBox;
class QSpinBox;


///////////////////////////////////////////////////////////////////////////


// Small container for Qt analog widgets that adds a label
class QAnalogContainer : public QWidget {
public:
    QAnalogContainer(QWidget* widget, const QString& text = QString());
};


///////////////////////////////////////////////////////////////////////////


// XML handler
class QXmlAutoGUIHandler {
public:
    // Constructor
    QXmlAutoGUIHandler(QAutoGUI* autoGui);

    // Parse from a QIODevice (e.g. QFile)
    bool parse(QIODevice* device);

protected:
    // Process individual element types
    void processElement(const QStringView& name, const QXmlStreamAttributes& atts);

    // Set up widgets from XML attributes
    void ProcessPushButton(QAbstractButton* button, const QXmlStreamAttributes& atts);
    void ProcessCheckableButton(QAbstractButton* button, const QXmlStreamAttributes& atts);
    void ProcessSliderWidget(QAbstractSlider* widget, const QXmlStreamAttributes& atts);
    void ProcessSpinBox(QSpinBox* widget, const QXmlStreamAttributes& atts);
    void ProcessDoubleSpinBox(QDoubleSpinBox* widget, const QXmlStreamAttributes& atts);

    // The QAutoGUI widget
    QAutoGUI* gui;
};


#endif
