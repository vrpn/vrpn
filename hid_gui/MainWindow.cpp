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
#include <iostream>

MainWindow::MainWindow(vrpn_HidAcceptor * acceptor, QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, _device(new HIDDevice(acceptor))
	, _timer(new QTimer)
	, _selectionLabel(new QLabel) {
	ui->setupUi(this);
	connect(_device.data(), SIGNAL(message(QString const&)), ui->textLog, SLOT(append(QString const&)));
	connect(_device.data(), SIGNAL(inputReport(QByteArray)), this, SLOT(gotReport(QByteArray)));
	connect(_timer.data(), SIGNAL(timeout()), _device.data(), SLOT(do_update()));
	_timer->start(20); // every 20 ms


	statusBar()->addWidget(_selectionLabel);
	_selectionLabel->setText(QString("No selected bytes"));
	/*
		QuickChart * chart = new QuickChart(this);

		chart->setMin(0);
		chart->setMax(256 * 256);

		ui->chartBox->addWidget(chart);
		connect(_inspector.data(), SIGNAL(newValue(float)), chart, SLOT(addSample(float)));
		connect(_device.data(), SIGNAL(inputReport(QByteArray)), _inspector.data(), SLOT(updatedData(QByteArray)));
		*/
}

MainWindow::~MainWindow() {
	delete ui;
}


void MainWindow::gotReport(QByteArray buf) {
	ui->reportSizeLabel->setText(QString("%1 bytes").arg(buf.size()));
	int initialStart = -1;
	int initialLength = -1;
	if (ui->reportContents->hasSelectedText()) {
		initialStart = ui->reportContents->selectionStart();
		initialLength = ui->reportContents->selectedText().length();
	}
	ui->reportContents->setText(buf.toHex());
	if (initialStart >= 0) {
		ui->reportContents->setSelection(initialStart, initialLength);
	}
}

void MainWindow::on_reportContents_selectionChanged() {

	if (ui->reportContents->hasSelectedText()) {
		int initialStart = ui->reportContents->selectionStart();
		int initialLength = ui->reportContents->selectedText().length();

		//std::cout << initialStart << ", " << initialLength << std::endl;
		int endingCharacter =  initialStart + initialLength;
		/// get the initial byte
		int startingByte = initialStart / 2;
		int endingByte = (endingCharacter + 1) / 2;
		int byteLength = endingByte - startingByte;
		/// Normalize selection if needed
		if (initialStart != startingByte * 2 || initialLength != byteLength * 2) {
			ui->reportContents->setSelection(startingByte * 2, byteLength * 2);
		}
		_selectionLabel->setText(QString("Offset %1, length %2").arg(startingByte).arg(byteLength));
	} else {
		_selectionLabel->setText(QString("No selected bytes"));
	}
}

void MainWindow::_addInspector(std::size_t size, bool signedVal, bool bigEndian) {
	bool ok;
	std::size_t start = QInputDialog::getInt(this, QString("Starting index"), QString("What (0-based) index should we start at? Current report length is %1.").arg(ui->reportSizeLabel->text()), 0, 0, 255, 1, &ok);
	if (ok) {
		QuickChart * chart = new QuickChart(this);
		int range = std::pow(2, 8 * size);
		chart->setMin(signedVal ? - range / 2 : 0);
		chart->setMax(signedVal ? range / 2 : range);
		QString label = QString("Offset %1 as %2-endian %3 int %4")
		                .arg(start)
		                .arg(bigEndian ? "big" : "little")
		                .arg(signedVal ? "signed" : "unsigned")
		                .arg(size * 8);
		chart->setLabel(label);
		ui->chartBox->addWidget(chart);

		InspectorPtr inspect(new Inspector(start, size, signedVal, bigEndian));
		connect(_device.data(), SIGNAL(inputReport(QByteArray)), inspect.data(), SLOT(updatedData(QByteArray)));
		connect(inspect.data(), SIGNAL(newValue(float)), chart, SLOT(addSample(float)));
		_inspectors.push_back(inspect);
	}
}
