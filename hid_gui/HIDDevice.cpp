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
// - none
class HIDDevice::VRPNDevice : public vrpn_HidInterface {
		HIDDevice * _container;

	public:
		VRPNDevice(vrpn_HidAcceptor * acceptor, HIDDevice * container)
			: vrpn_HidInterface(acceptor)
			, _container(container)
		{}
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
