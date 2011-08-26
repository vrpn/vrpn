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
#include "HIDDevice.h"
#include "vrpn_HumanInterface.h"

// Library/third-party includes
#include <QDateTime>

// Standard includes
#include <iostream>

class HIDDevice::VRPNDevice : public vrpn_HidInterface {
		HIDDevice * _container;

	public:
		VRPNDevice(vrpn_HidAcceptor * acceptor, HIDDevice * container)
			: vrpn_HidInterface(acceptor)
			, _container(container)
		{}
		void handshake() {
			_container->send_message_signal(QString("in handshake()"));
			if (connected()) {
				_container->send_message_signal(QString("Starting the handshake"));
				int bytes;

				vrpn_uint8 buf[91] = {0};
				buf[0] = 0;
				bytes = get_feature_report(13, buf);
				_container->send_message_signal(QString("12-byte feature request 0: expected -1, got %1").arg(bytes));

				buf[0] = 0;
				bytes = get_feature_report(91, buf);
				_container->send_message_signal(QString("90-byte feature request 0: expected -90, got %1").arg(bytes));
				static const vrpn_uint8 HYDRA_FEATURE_REPORT[] = {
				  0x00, // first byte must be report type
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x06, 0x00
				};
				static const int HYDRA_FEATURE_REPORT_LEN = 91;

				/// Prompt to start streaming motion data
				send_feature_report(HYDRA_FEATURE_REPORT_LEN, HYDRA_FEATURE_REPORT);

				buf[0] = 0;
				bytes = get_feature_report(91, buf);
				_container->send_message_signal(QString("90-byte feature request 0: expected -90, got %1").arg(bytes));
			}
		}
	protected:
		void on_data_received(size_t bytes, vrpn_uint8 *buffer) {
			_container->send_data_signal(bytes, reinterpret_cast<char *>(buffer));
		}

};

HIDDevice::HIDDevice(vrpn_HidAcceptor * acceptor, QObject * parent)
	: QObject(parent)
	, _connected(false)
	, _device(new VRPNDevice(acceptor, this))
	, _startingTimestamp(-1) {}

HIDDevice::~HIDDevice() {
	delete _device;
	_device = NULL;
}

void HIDDevice::do_update() {
	bool wasConnected = _connected;
	_connected = _device->connected();

	if (_connected && !wasConnected) {
		emit message("Connected to device!");
		_device->handshake();
	} else if (!_connected && wasConnected) {
		emit message("Lost connection to device!");
	}
	_device->update();
}



void HIDDevice::send_data_signal(size_t bytes, const char * buffer) {
	qint64 current = QDateTime::currentMSecsSinceEpoch();
	if (_startingTimestamp < 0) {
		_startingTimestamp = current;
	}
	emit inputReport(QByteArray(buffer, bytes), current - _startingTimestamp);
}

void HIDDevice::send_message_signal(QString const& msg) {
	emit message(msg);
}
