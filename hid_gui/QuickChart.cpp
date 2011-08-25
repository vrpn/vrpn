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
#include "QuickChart.h"
#include "ui_plot.h"

// Library/third-party includes
#include <QGraphicsScene>

// Standard includes
// - none


QuickChart::QuickChart(QWidget * parent)
	: QFrame(parent)
	, ui(new Ui::Plot)
	, _x(0)
	, _last(0)
	, _min(0)
	, _max(255)
	, _sampleWidth(200)
	, _scene(new QGraphicsScene) {
	ui->setupUi(this);
	ui->graphicsView->setScene(_scene.data());
}

QuickChart::~QuickChart() {
	delete ui;
}

void QuickChart::addSample(float sample) {
	if (_x != 0) {
		_scene->addLine(_x, _last, _x + 1, sample);
	}
	_last = sample;
	_x++;
}

void QuickChart::updateViewFit() {
	ui->graphicsView->fitInView(0, _min, _sampleWidth, _max);
}

void QuickChart::setSampleWidth(int w) {
	_sampleWidth = w;
	updateViewFit();
}

void QuickChart::setLabel(QString const& l) {
	ui->label->setText(l);
}
