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
// - none

// Standard includes
// - none


MainWindow::MainWindow(vrpn_HidAcceptor * acceptor, QWidget * parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, _device(new HIDDevice(acceptor, this)) {
	ui->setupUi(this);
	/*
	connect(ui->connect, SIGNAL(clicked()), _wand.data(), SLOT(connect()));
	connect(ui->disconnect, SIGNAL(clicked()), _wand.data(), SLOT(disconnect()));

	connect(_wand.data(), SIGNAL(startingConnectionAttempt()), this, SLOT(disableAllDuringConnectionAttempt()));
	connect(_wand.data(), SIGNAL(connected()), this, SLOT(updateButtons()));
	connect(_wand.data(), SIGNAL(connected()), this, SLOT(connectedWiimote()));
	connect(_wand.data(), SIGNAL(disconnected()), this, SLOT(updateButtons()));
	connect(_wand.data(), SIGNAL(disconnected()), this, SLOT(handleDisconnect()));
	connect(_wand.data(), SIGNAL(connectionFailed(QString)), this, SLOT(handleMessages(QString)));
	connect(_wand.data(), SIGNAL(statusUpdate(QString)), this, SLOT(handleMessages(QString)));



	connect(_wand.data(), SIGNAL(batteryUpdate(float)), _wmPanel.data(), SLOT(setBattery(float)));
	connect(_wand.data(), SIGNAL(disconnected()), _wmPanel.data(), SLOT(disconnected()));
	connect(_wand.data(), SIGNAL(wiimoteNumber(int)), _wmPanel.data(), SLOT(wiimoteNumber(int)));
	connect(_wand.data(), SIGNAL(buttonUpdate(int, bool)), _wmPanel.data(), SLOT(buttonUpdate(int, bool)));
	*/
}

MainWindow::~MainWindow() {
	delete ui;
}
