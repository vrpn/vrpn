/** @file
	@brief Header

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

#pragma once
#ifndef INCLUDED_QuickChart_h_GUID_bb3e58a9_c33a_4e30_82e5_c721df4f0df7
#define INCLUDED_QuickChart_h_GUID_bb3e58a9_c33a_4e30_82e5_c721df4f0df7

// Internal Includes
// - none

// Library/third-party includes
#include <QFrame>

// Standard includes
// - none


namespace Ui {
	class Plot;
}

class QuickChart : public QFrame {
		Q_OBJECT

	public:
		explicit QuickChart(QWidget *parent = 0);
		~QuickChart();
		void setMin(float v) {
			_min = v;
		}
		float min() const {
			return _min;
		}
		void setMax(float v) {
			_max = v;
		}
		float max() const {
			return _max;
		}
	public slots:
		void addSample(float sample);
	private:
		Ui::Plot *ui;
		float _min;
		float _max;

};
#endif // INCLUDED_QuickChart_h_GUID_bb3e58a9_c33a_4e30_82e5_c721df4f0df7
