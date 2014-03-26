/*=========================================================================

  Name:         QXmlAutoGUIHandler.h

  Author:       David Borland, EventLab

  Description:  XML parser for QAutoGUI.

=========================================================================*/


#ifndef QXMLAUTOGUIHANDLER_H
#define QXMLAUTOGUIHANDLER_H


#include <QXmlDefaultHandler>
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
    QAnalogContainer(QWidget* widget, const QString& text = NULL);
};


///////////////////////////////////////////////////////////////////////////


// XML handler
class QXmlAutoGUIHandler : public QXmlDefaultHandler {
public:
    // Constructor
    QXmlAutoGUIHandler(QAutoGUI* autoGui);

    // Callbacks for XML parsing
    virtual bool startElement(const QString& namespaceURI, const QString& localName, 
                              const QString& qName, const QXmlAttributes& atts);
    virtual bool endDocument();
    virtual bool fatalError(const QXmlParseException& exception);

protected:
    // Set up widgets from XML attributes
    void ProcessPushButton(QAbstractButton* button, const QXmlAttributes& atts);
    void ProcessCheckableButton(QAbstractButton* button, const QXmlAttributes& atts);
    void ProcessSliderWidget(QAbstractSlider* widget, const QXmlAttributes& atts);
    void ProcessSpinBox(QSpinBox* widget, const QXmlAttributes& atts);
    void ProcessDoubleSpinBox(QDoubleSpinBox* widget, const QXmlAttributes& atts);

    // The QAutoGUI widget
    QAutoGUI* gui;
};


#endif