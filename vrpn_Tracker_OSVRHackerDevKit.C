/** @file	vrpn_Tracker_OSVRHackerDevKit.C
	@brief	Implemendation of the OSVR Hacker Dev Kit HMD tracker.

	@date	2011

	@author
	Kevin M. Godby
	<kevin@godby.org>
*/

#include <string.h>                     // for memset
#include <iostream>                     // for operator<<, ostringstream, etc
#include <sstream>
#include <stdexcept>                    // for logic_error

#include "vrpn_Tracker_OSVRHackerDevKit.h"
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_NORMAL, etc
#include "vrpn_FixedPoint.h"            // for vrpn_fixed_to_float

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 vrpn_OSVR_VENDOR = 0x1532;
static const vrpn_uint16 vrpn_OSVR_HACKER_DEV_KIT_HMD = 0x0b00;

vrpn_Tracker_OSVRHackerDevKit::vrpn_Tracker_OSVRHackerDevKit(vrpn_HidAcceptor *filter, const char *name, vrpn_Connection *c) :
    vrpn_Tracker(name, c),
    vrpn_HidInterface(filter),
    _wasConnected(false)
{
        vrpn_Tracker::num_channel = 1; // FIXME

        // Initialize the state of all the analogs
        memset(channel, 0, sizeof(channel));
        memset(last, 0, sizeof(last));

        // Set the timestamp
        vrpn_gettimeofday(&_timestamp, NULL);
    }

vrpn_Tracker_OSVRHackerDevKit::~vrpn_Tracker_OSVRHackerDevKit()
{
    delete _acceptor;
}

void vrpn_Tracker_OSVRHackerDevKit::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
    if (bytes != 32) {
        std::ostringstream ss;
        ss << "Received a too-short report: " << bytes;
        struct timeval ts;
        vrpn_gettimeofday(&ts, NULL);
        send_text_message(ss.str().c_str(), ts, vrpn_TEXT_WARNING);
        return;
    }

    vrpn_uint8 version = vrpn_unbuffer_from_little_endian<vrpn_uint8>(buffer);
    vrpn_uint8 msg_seq = vrpn_unbuffer_from_little_endian<vrpn_uint8>(buffer);

    // signed, 16-bit, fixed-point number with Q = 14.
    d_quat[Q_W] = vrpn_fixed_to_float<double>(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer), 1, 14);
    d_quat[Q_X] = vrpn_fixed_to_float<double>(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer), 1, 14);
    d_quat[Q_Y] = vrpn_fixed_to_float<double>(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer), 1, 14);
    d_quat[Q_Z] = vrpn_fixed_to_float<double>(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer), 1, 14);

    report_changes();
}

void vrpn_Tracker_OSVRHackerDevKit::mainloop()
{
    vrpn_gettimeofday(&_timestamp, NULL);

    update();

    if (connected() && !_wasConnected) {
        send_text_message("Successfully connected to OSVR Hacker Dev Kit HMD.", _timestamp, vrpn_TEXT_NORMAL);
    }
    _wasConnected = connected();

    server_mainloop();
}

void vrpn_Tracker_OSVRHackerDevKit::report_changes(vrpn_uint32 class_of_service)
{
    vrpn_Tracker::timestamp = _timestamp;

    vrpn_Tracker::report_changes(class_of_service);
}

void vrpn_Tracker_OSVRHackerDevKit::report(vrpn_uint32 class_of_service)
{
    vrpn_Tracker::timestamp = _timestamp;

    vrpn_Tracker::report(class_of_service);
}

#endif

