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

		/// set minimum y value
		void setMin(float v) {
			_min = v;
			updateViewFit();
		}

		/// get minimum y value
		float getMin(void) const {
			return _min;
		}

		/// set maximum y value
		void setMax(float v) {
			_max = v;
			updateViewFit();
		}

		/// get maximum y value
		float getMax(void) const {
			return _max;
		}

		/// set width of x values we should fit in the control at once
		void setSampleWidth(float w);

		/// Set the text of the label
		void setLabel(QString const& l);

	public slots:
		/// Add a sample, with x defaulted to 1 + previous x
		void addSample(float sample);
		/// Add a sample specifying both x and the sample (y)
		void addSample(float x, float sample);
		void updateViewFit();
		void setSceneRect();

	private:
		Ui::Plot *ui;
		float _x;
		float _last;
		float _min;
		float _max;
		float _sampleWidth;
		bool _gotOne;
		QSharedPointer<QGraphicsScene> _scene;

};

