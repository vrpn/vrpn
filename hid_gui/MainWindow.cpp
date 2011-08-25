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

// Library/third-party includes
#include <QTimer>

// Standard includes
// - none


MainWindow::MainWindow(vrpn_HidAcceptor * acceptor, QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, _device(new HIDDevice(acceptor, this))
	, _timer(new QTimer(this)) {
	ui->setupUi(this);
	connect(_device.data(), SIGNAL(message(QString const&)), ui->textLog, SLOT(append(QString const&)));
	connect(_device.data(), SIGNAL(inputReport(QByteArray)), this, SLOT(gotReport(QByteArray)));
	connect(_timer.data(), SIGNAL(timeout()), _device.data(), SLOT(do_update()));
	_timer->start(20); // every 20 ms
}

MainWindow::~MainWindow() {
	delete ui;
}


void MainWindow::gotReport(QByteArray buf) {
	ui->reportSizeLabel->setText(QString("%1 bytes").arg(buf.size()));

	ui->reportContents->setText(buf.toHex());
}
