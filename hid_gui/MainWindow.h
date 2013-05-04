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
#include <QMainWindow>
#include <QSharedPointer>
#include <QPair>

// Standard includes
// - none


namespace Ui {
	class MainWindow;
}
class QTimer;
class QLabel;
class QMenu;

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

		void addSignedLEInspector() {
			_addInspector(true, false);
		}
		void addUnsignedLEInspector() {
			_addInspector(false, false);
		}

		void addSignedBEInspector() {
			_addInspector(true, true);
		}
		void addUnsignedBEInspector() {
			_addInspector(false, true);
		}

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

		void on_actionRemove_all_inspectors_triggered();

		void on_reportContents_selectionChanged();

		void on_reportContents_customContextMenuRequested(const QPoint & pos);

	private:
		/// @brief return the offset and the length, in bytes, of the selection,
		/// or -1, -1 if no selection.
		QPair<int, int> _getSelectionByteOffsetLength() const;

		/// @brief Helper function to add an inspector from the selection
		void _addInspector(bool signedVal, bool bigEndian);

		/// @brief Helper function to add an inspector from the menu, prompting for offset
		void _addInspector(std::size_t size, bool signedVal, bool bigEndian);

		/// @brief Helper function called by the other overloads adding an inspector once
		/// we know all the parameters
		void _addInspector(std::size_t offset, std::size_t size, bool signedVal, bool bigEndian);

		typedef QSharedPointer<Inspector> InspectorPtr;
		Ui::MainWindow *ui;
		QSharedPointer<HIDDevice> _device;
		QSharedPointer<QTimer> _timer;
		std::vector<InspectorPtr> _inspectors;
		QSharedPointer<QMenu> _singlebyteIntMenu;
		QSharedPointer<QMenu> _multibyteIntMenu;

		/// ownership transferred to status bar
		QLabel * _selectionLabel;

};

