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
#include <QSharedPointer>

// Standard includes
// - none


namespace Ui {
	class Plot;
}

class QGraphicsScene;

class QuickChart : public QFrame {
		Q_OBJECT

	public:
		explicit QuickChart(QWidget *parent = 0);
		~QuickChart();
		void setMin(float v) {
			_min = v;
			updateViewFit();
		}
		float min() const {
			return _min;
		}
		void setMax(float v) {
			_max = v;
			updateViewFit();
		}
		float max() const {
			return _max;
		}
		void setSampleWidth(int w);

		void setLabel(QString const& l);
	public slots:
		void addSample(float sample);
		void updateViewFit();
	private:
		Ui::Plot *ui;
		int _x;
		float _last;
		float _min;
		float _max;
		int _sampleWidth;
		QSharedPointer<QGraphicsScene> _scene;

};
#endif // INCLUDED_QuickChart_h_GUID_bb3e58a9_c33a_4e30_82e5_c721df4f0df7
