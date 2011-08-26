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
#include <iostream>

QuickChart::QuickChart(QWidget * parent)
	: QFrame(parent)
	, ui(new Ui::Plot)
	, _x(0)
	, _last(0)
	, _min(0)
	, _max(255)
	, _sampleWidth(10)
	, _gotOne(false)
	, _scene(new QGraphicsScene) {
	ui->setupUi(this);
	ui->graphicsView->setScene(_scene.data());
	updateViewFit();
}

QuickChart::~QuickChart() {
	delete ui;
}

void QuickChart::addSample(float sample) {
	addSample(_x + 1, sample);
}

void QuickChart::addSample(float x, float sample) {
	if (_gotOne) {
		_scene->addLine(_x, _last, x, sample);
	}
	std::cout << x << ", " << sample << std::endl;
	_gotOne = true;
	_last = sample;
	_x = x;
}

void QuickChart::updateViewFit() {
	const QSize s = ui->graphicsView->size();
	const float h(s.height());
	const float w(s.width());
	std::cout << "Expecting input in [" << _min << ", " << _max << "]" << std::endl;
	std::cout << "Fitting " << _sampleWidth << " in the width available." << std::endl;
	std::cout << "size is " << h << " tall, " << w << " wide" << std::endl;
	const float xScale = w / _sampleWidth;
	const float yScale = h / (_max - _min);
	//std::cout << "Scale by " << xScale << ", " << yScale << std::endl;
	ui->graphicsView->setTransform(QTransform::fromScale(xScale, yScale).translate(-_min, 0));
	//ui->graphicsView->setTransform(QTransform::fromTranslate(-_min, 0).scale(xScale, yScale));
	//ui->graphicsView->fitInView(0, _min, _sampleWidth, _max);
}

void QuickChart::setSampleWidth(float w) {
	_sampleWidth = w;
	updateViewFit();
}

void QuickChart::setLabel(QString const& l) {
	ui->label->setText(l);
}
