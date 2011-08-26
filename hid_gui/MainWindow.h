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
#ifndef INCLUDED_MainWindow_h_GUID_2f127696_8780_4ce5_86c9_46f4df7fe245
#define INCLUDED_MainWindow_h_GUID_2f127696_8780_4ce5_86c9_46f4df7fe245

// Internal Includes
// - none

// Library/third-party includes
#include <QMainWindow>
#include <QSharedPointer>

// Standard includes
// - none


namespace Ui {
	class MainWindow;
}
class QTimer;
class QLabel;

class HIDDevice;
class vrpn_HidAcceptor;
class Inspector;

class MainWindow : public QMainWindow {
		Q_OBJECT

	public:
		explicit MainWindow(vrpn_HidAcceptor * acceptor, QWidget *parent = 0);
		~MainWindow();

	public slots:
		void gotReport(QByteArray buf);

		void on_actionInt8_2_triggered() {
			_addInspector(1, true, false);
		}

		void on_actionUint8_2_triggered() {
			_addInspector(1, false, false);
		}

		void on_actionInt16_LE_triggered() {
			_addInspector(2, true, false);
		}
		void on_actionInt16_BE_triggered() {
			_addInspector(2, true, true);
		}


		void on_actionUint16_LE_triggered() {
			_addInspector(2, false, false);
		}
		void on_actionUint16_BE_triggered() {
			_addInspector(2, false, true);
		}

		void on_reportContents_selectionChanged();
	private:
		typedef QSharedPointer<Inspector> InspectorPtr;
		Ui::MainWindow *ui;
		QSharedPointer<HIDDevice> _device;
		QSharedPointer<QTimer> _timer;
		QSharedPointer<Inspector> _inspector;
		void _addInspector(std::size_t size, bool signedVal, bool bigEndian);
		std::vector<InspectorPtr> _inspectors;
		QLabel * _selectionLabel;

};
#endif // INCLUDED_MainWindow_h_GUID_2f127696_8780_4ce5_86c9_46f4df7fe245
