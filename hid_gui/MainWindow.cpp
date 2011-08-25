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
	, _timer(new QTimer) {
	ui->setupUi(this);
	connect(_device.data(), SIGNAL(message(QString const&)), ui->textLog, SLOT(append(QString const&)));
	connect(_device.data(), SIGNAL(inputReport(QByteArray)), this, SLOT(gotReport(QByteArray)));
	connect(_timer.data(), SIGNAL(timeout()), _device.data(), SLOT(do_update()));
	_timer->start(20); // every 20 ms

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

	ui->reportContents->setText(buf.toHex());
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
