/*
 * vrpn_Tracker_SpacePoint.cpp
 *
 *  Created on: Nov 22, 2010
 *      Author: janoc
 */

#include <stdio.h>                      // for fprintf, stderr
#include <string.h>                     // for memset

#include "vrpn_Connection.h"            // for vrpn_CONNECTION_LOW_LATENCY, etc
#include "vrpn_Tracker_SpacePoint.h"

const unsigned SPACEPOINT_VENDOR = 0x20ff;
const unsigned SPACEPOINT_PRODUCT = 0x0100;

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#ifdef VRPN_USE_HID
vrpn_Tracker_SpacePoint::vrpn_Tracker_SpacePoint(const char * name, vrpn_Connection * trackercon) :
                    vrpn_Tracker(name, trackercon), vrpn_Button(name, trackercon),
                    vrpn_HidInterface(new vrpn_HidProductAcceptor(SPACEPOINT_VENDOR, SPACEPOINT_PRODUCT),
                    SPACEPOINT_VENDOR, SPACEPOINT_PRODUCT)
{
    memset(d_quat, 0, 4 * sizeof(float));
    d_quat[3] = 1.0;

    vrpn_Button::num_buttons = 2;
    vrpn_gettimeofday(&_timestamp, NULL);
}

void vrpn_Tracker_SpacePoint::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
    /*
     * Horrible kludge - as we do not have a way to select the correct endpoint, we have to use a hack
     * to identify the correct device. In Windows the SpacePoint appears as 2 devices with same VID/PID, one for each endpoint
     * The correct device sends a report 15 bytes long. If we get a report of a different length, we try to open
     * the other device instead.
     */

    // test from the app note
    // the quaternion should be: 0.2474, -0.1697, -0.1713, 0.9384
    /*
    bytes = 15;
    vrpn_uint8 test_dta[] =
                    { 0x2f, 0x85, 0x23, 0x8a, 0x5b, 0x90, 0xac, 0x9f, 0x49, 0x6a, 0x12, 0x6a, 0x1e, 0xf8, 0xd0 };
    memcpy(buffer, test_dta, 15 * sizeof(vrpn_uint8));
    */

    if (bytes == 15) {
        vrpn_uint8 * bufptr = buffer + 6;

        for (int i = 0; i < 4; i++) {
            d_quat[i] = (vrpn_unbuffer_from_little_endian<vrpn_uint16>(bufptr) - 32768) / 32768.0;
        }

        buttons[0] = buffer[14] & 0x1;
        buttons[1] = buffer[14] & 0x2;

        // Find out what time we received the new information, then send any
        // changes to the client.
        vrpn_gettimeofday(&_timestamp, NULL);
        vrpn_Tracker::timestamp = _timestamp;
        vrpn_Button::timestamp = _timestamp;

        // send tracker orientation
        d_sensor = 0;
        memset(pos, 0, sizeof(vrpn_float64) * 3); // no position

        char msgbuf[1000];
        int len = vrpn_Tracker::encode_to(msgbuf);
        if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
            fprintf(stderr, "SpacePoint tracker: can't write message: tossing\n");
        }

        // send buttons
        vrpn_Button::report_changes();

    } else {
        // try the other iface
        // as we are keeping the first one open,
        // it will not enumerate and we get the next one. Horrible kludge :(
        reconnect();
    }
}

void vrpn_Tracker_SpacePoint::mainloop()
{
    if (connected()) {
        // device update.  This will call on_data_received() if we get something.
        update();

        // server update
        server_mainloop();
    }
}

#endif
