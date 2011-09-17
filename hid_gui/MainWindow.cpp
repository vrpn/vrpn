/**
	@file
	@brief Implementation

	@date 2011

	@author
	Ryan Pavlik
	<rpavlik@iastate.edu> and <abiryan@ryand.net>
	http://academic.cleardefinition.com/
	Iowa State University Virtual Reality Applications Center
	Human-Computer Interaction Graduate Program
*/

//          Copyright Iowa State University 2011.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Internal Includes
#include "MainWindow.h"
#include "ui_mainwindow.h"
#include "HIDDevice.h"
#include "Inspector.h"
#include "QuickChart.h"

// Library/third-party includes
#include <QTimer>
#include <QInputDialog>

// Standard includes
#include <cmath>

MainWindow::MainWindow(vrpn_HidAcceptor * acceptor, QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, _device(new HIDDevice(acceptor))
	, _timer(new QTimer)
	, _singlebyteIntMenu(new QMenu)
	, _multibyteIntMenu(new QMenu)
	, _selectionLabel(new QLabel) {

	ui->setupUi(this);
	/// When the device has a message, add it to the log.
	connect(_device.data(), SIGNAL(message(QString const&)), ui->textLog, SLOT(append(QString const&)));

	/// When the device gets an input report, update our controls
	connect(_device.data(), SIGNAL(inputReport(QByteArray, qint64)), this, SLOT(gotReport(QByteArray)));

	/// Update the HID device every 20 ms
	connect(_timer.data(), SIGNAL(timeout()), _device.data(), SLOT(do_update()));
	_timer->start(20);

	/// Widget for showing what offset and length are selected
	statusBar()->addPermanentWidget(_selectionLabel);
	_selectionLabel->setText(QString("No selected bytes"));

	/// Create context menu for when a single byte is selected
	_singlebyteIntMenu->addAction(QString("Inspect as signed integer"), this, SLOT(addSignedLEInspector()));
	_singlebyteIntMenu->addAction(QString("Inspect as unsigned integer"), this, SLOT(addUnsignedLEInspector()));

	/// Create context menu for when 2, 4 bytes are selected
	QMenu * submenu = _multibyteIntMenu->addMenu(QString("Inspect as signed integer"));
	submenu->addAction(QString("Big endian"), this, SLOT(addSignedBEInspector()));
	submenu->addAction(QString("Little endian"), this, SLOT(addSignedLEInspector()));

	submenu = _multibyteIntMenu->addMenu(QString("Inspect as unsigned integer"));
	submenu->addAction(QString("Big endian"), this, SLOT(addUnsignedBEInspector()));
	submenu->addAction(QString("Little endian"), this, SLOT(addUnsignedLEInspector()));
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::gotReport(QByteArray buf) {
	/// Update display of report size
	ui->reportSizeLabel->setText(QString("%1 bytes").arg(buf.size()));

	/// Save selection range, if applicable
	int initialStart = -1;
	int initialLength = -1;
	if (ui->reportContents->hasSelectedText()) {
		initialStart = ui->reportContents->selectionStart();
		initialLength = ui->reportContents->selectedText().length();
	}

	/// Update text
	ui->reportContents->setText(buf.toHex());

	/// Restore selection range
	if (initialStart >= 0) {
		ui->reportContents->setSelection(initialStart, initialLength);
	}
}

void MainWindow::on_actionRemove_all_inspectors_triggered() {
	/// remove all inspectors
	_inspectors.clear();

	/// remove all charts
	QLayoutItem * child;
	while ((child = ui->chartBox->takeAt(0)) != NULL) {
		delete child->widget();
		delete child;
	}
}

void MainWindow::on_reportContents_selectionChanged() {
	if (ui->reportContents->hasSelectedText()) {
		const QPair<int, int> offsetLength = _getSelectionByteOffsetLength();
		const int byteOffset = offsetLength.first;
		const int byteLength = offsetLength.second;

		const int initialStart = ui->reportContents->selectionStart();
		const int initialLength = ui->reportContents->selectedText().length();

		/// Normalize selection if needed
		if (initialStart !=  byteOffset * 2 || initialLength !=  byteLength * 2) {
			ui->reportContents->setSelection(byteOffset * 2, byteLength * 2);
		}
		_selectionLabel->setText(QString("Offset %1, length %2").arg(byteOffset).arg(byteLength));
	} else {
		_selectionLabel->setText(QString("No selected bytes"));
	}
}

void MainWindow::on_reportContents_customContextMenuRequested(const QPoint & pos) {
	QPoint globalPos = ui->reportContents->mapToGlobal(pos);
	const QPair<int, int> offsetLength = _getSelectionByteOffsetLength();
	const int byteLength = offsetLength.second;
	switch (byteLength) {
		case -1:
			/// no selection, no menu.
			statusBar()->showMessage("No bytes selected to inspect.");
			break;
		case 1:
			/// Single byte
			_singlebyteIntMenu->popup(globalPos);
			break;
		case 2:
		case 4:
			/// Valid multibyte
			_multibyteIntMenu->popup(globalPos);
			break;
		default:
			/// Some other number of bytes - not handled
			statusBar()->showMessage("Can only inspect integers with powers of 2 lengths.");
			break;
	}
}

QPair<int, int> MainWindow::_getSelectionByteOffsetLength() const {
	if (ui->reportContents->hasSelectedText()) {
		int initialStart = ui->reportContents->selectionStart();
		int initialLength = ui->reportContents->selectedText().length();

		int endingCharacter =  initialStart + initialLength;

		/// get the initial byte
		int startingByte = initialStart / 2;
		int endingByte = (endingCharacter + 1) / 2;
		int byteLength = endingByte - startingByte;
		return QPair<int, int>(startingByte, byteLength);
	}
	/// Default case for when there's no selection
	return QPair<int, int>(-1, -1);
}

void MainWindow::_addInspector(bool signedVal, bool bigEndian) {
	const QPair<int, int> offsetLength = _getSelectionByteOffsetLength();
	const int byteOffset = offsetLength.first;
	const int byteLength = offsetLength.second;
	_addInspector(byteOffset, byteLength, signedVal, bigEndian);
}

void MainWindow::_addInspector(std::size_t size, bool signedVal, bool bigEndian) {
	bool ok;
	std::size_t offset = QInputDialog::getInt(this,
	                     QString("Starting index"),
	                     QString("What (0-based) index should we start at? Current report length is %1.")
	                     .arg(ui->reportSizeLabel->text()),
	                     0, 0, 255, 1, &ok);
	if (ok) {
		_addInspector(offset, size, signedVal, bigEndian);
	}
}


void MainWindow::_addInspector(std::size_t offset, std::size_t size, bool signedVal, bool bigEndian) {
	QuickChart * chart = new QuickChart(this);
	int range = std::pow(2.0, 8.0 * size);
	chart->setMin(signedVal ? - range / 2 : 0);
	chart->setMax(signedVal ? range / 2 : range);
	QString label = QString("Offset %1 as %2-endian %3 int %4")
	                .arg(offset)
	                .arg(bigEndian ? "big" : "little")
	                .arg(signedVal ? "signed" : "unsigned")
	                .arg(size * 8);
	chart->setLabel(label);
	ui->chartBox->addWidget(chart);

	InspectorPtr inspect(new Inspector(offset, size, signedVal, bigEndian));
	connect(_device.data(), SIGNAL(inputReport(QByteArray, qint64)), inspect.data(), SLOT(updatedData(QByteArray, qint64)));
	connect(inspect.data(), SIGNAL(newValue(float, float)), chart, SLOT(addSample(float, float)));
	_inspectors.push_back(inspect);
}
