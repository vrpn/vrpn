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
#include "vrpn_Tracker_RazerHydra.h"
#include "quat.h"

// Library/third-party includes
// - none

// Standard includes
// - none

#ifdef VRPN_USE_HID

const unsigned int HYDRA_VENDOR = 0x1532;
const unsigned int HYDRA_PRODUCT = 0x0300;
const unsigned int HYDRA_INTERFACE = 0x0;

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

vrpn_Tracker_RazerHydra::vrpn_Tracker_RazerHydra(const char * name, vrpn_Connection * trackercon)
	: vrpn_Tracker(name, trackercon)
	, vrpn_HidInterface(new vrpn_HidBooleanAndAcceptor(
	                        new vrpn_HidInterfaceNumberAcceptor(HYDRA_INTERFACE),
	                        new vrpn_HidProductAcceptor(HYDRA_VENDOR, HYDRA_PRODUCT)))
	, _name(name)
	, _con(trackercon) {

	memset(d_quat, 0, 4 * sizeof(float));
	d_quat[3] = 1.0;

}

void vrpn_Tracker_RazerHydra::on_data_received(size_t bytes, vrpn_uint8 *buffer) {
	fprintf(stderr, "vrpn_Tracker_RazerHydra: got %d bytes\n", bytes);
}

void vrpn_Tracker_RazerHydra::mainloop() {
	if (connected()) {
		// device update
		update();

		// server update
		server_mainloop();

		vrpn_Tracker::timestamp = _timestamp;

		// send tracker orientation
		d_sensor = 0;
		memset(pos, 0, sizeof(vrpn_float64) * 3); // no position

		char msgbuf[1000];
		int len = vrpn_Tracker::encode_to(msgbuf);
		if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr, "vrpn_Tracker_RazerHydra: can't write message: tossing\n");
		}
	}
}

void vrpn_Tracker_RazerHydra::reconnect() {
	vrpn_HidInterface::reconnect();
	if (connected()) {
		/// Prompt to start streaming motion data
		send_feature_report(HYDRA_FEATURE_REPORT_LEN, HYDRA_FEATURE_REPORT);

		vrpn_uint8 buf[91] = {0};
		buf[0] = 0;
		int bytes = get_feature_report(91, buf);
		fprintf(stderr, "vrpn_Tracker_RazerHydra: feature report: read %d bytes\n", bytes);
	}
}


#endif // VRPN_USE_HID
