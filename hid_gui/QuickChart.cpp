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
// - none

// Standard includes
// - none


QuickChart::QuickChart(QWidget * parent)
	: QFrame(parent)
	, ui(new Ui::Plot)
	, _min(0)
	, _max(255) {
	ui->setupUi(this);
}

QuickChart::~QuickChart() {
	delete ui;
}

void QuickChart::addSample(float /*sample*/) {

}
