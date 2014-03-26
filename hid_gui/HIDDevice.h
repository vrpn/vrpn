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
#include "vrpn_Shared.h"

// Library/third-party includes
#include <QObject>
#include <QByteArray>
#include <QString>

// Standard includes
// - none

class vrpn_HidAcceptor;

/// Qt wrapper for a debugging HidInterface.
class HIDDevice: public QObject {
		Q_OBJECT
	public:
		explicit HIDDevice(vrpn_HidAcceptor * acceptor, QObject * parent = NULL);
		~HIDDevice();
	signals:
		void inputReport(QByteArray buffer, qint64 timestamp);
		void message(QString const& msg);

	public slots:
		void do_update();

	protected:
		bool _connected;
		class VRPNDevice;
		friend class HIDDevice::VRPNDevice;
		void send_data_signal(size_t bytes, const char * buffer);
		void send_message_signal(QString const& msg);
		VRPNDevice * _device;
		qint64 _startingTimestamp;

};

