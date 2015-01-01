/** @file	vrpn_Tracker_OSVRHackerDevKit.C
    @brief	Implemendation of the OSVR Hacker Dev Kit HMD tracker.

    @date	2014

    @author
    Kevin M. Godby
    <kevin@godby.org>
*/

#include "vrpn_Tracker_OSVRHackerDevKit.h"

#include "vrpn_SendTextMessageStreamProxy.h"
#include "vrpn_BaseClass.h"  // for ::vrpn_TEXT_NORMAL, etc
#include "vrpn_FixedPoint.h" // for vrpn::FixedPoint
#include <quat.h>            // for Q_W, Q_X, etc.

#include <cstring>   // for memset
#include <stdexcept> // for logic_error

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 vrpn_OSVR_VENDOR = 0x1532;
static const vrpn_uint16 vrpn_OSVR_HACKER_DEV_KIT_HMD = 0x0b00;

/// @todo Alternate, temporary VID/PID
static const vrpn_uint16 vrpn_OSVR_ALT_VENDOR = 0x03EB;
static const vrpn_uint16 vrpn_OSVR_ALT_HACKER_DEV_KIT_HMD = 0x2421;

vrpn_Tracker_OSVRHackerDevKit::vrpn_Tracker_OSVRHackerDevKit(const char *name,
                                                             vrpn_Connection *c)
    : vrpn_Tracker(name, c)
    , vrpn_HidInterface(new vrpn_HidBooleanOrAcceptor(
          new vrpn_HidProductAcceptor(vrpn_OSVR_VENDOR,
                                      vrpn_OSVR_HACKER_DEV_KIT_HMD),
          new vrpn_HidProductAcceptor(vrpn_OSVR_ALT_VENDOR,
                                      vrpn_OSVR_ALT_HACKER_DEV_KIT_HMD)))
    , _wasConnected(false)
{
    vrpn_Tracker::num_sensors = 1; // only orientation

    // Initialize the state
    std::memset(d_quat, 0, sizeof(d_quat));
    d_quat[Q_W] = 1.0;

    // Set the timestamp
    vrpn_gettimeofday(&_timestamp, NULL);
}

vrpn_Tracker_OSVRHackerDevKit::~vrpn_Tracker_OSVRHackerDevKit()
{
    delete _acceptor;
}

void vrpn_Tracker_OSVRHackerDevKit::on_data_received(std::size_t bytes,
                                                     vrpn_uint8 *buffer)
{
    if (bytes != 32) {
        send_text_message(vrpn_TEXT_WARNING)
            << "Received a report " << bytes
            << " in length, but expected it to be 32 bytes. Discarding.";
        return;
    }

    vrpn_uint8 version = vrpn_unbuffer_from_little_endian<vrpn_uint8>(buffer);
    /// @todo Verify that version is what we expect.
    vrpn_uint8 msg_seq = vrpn_unbuffer_from_little_endian<vrpn_uint8>(buffer);

    // Signed, 16-bit, fixed-point numbers in Q1.14 format.
    typedef vrpn::FixedPoint<1, 14> FixedPointValue;
    d_quat[Q_X] =
        FixedPointValue(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer))
            .get<vrpn_float64>();
    d_quat[Q_Y] =
        FixedPointValue(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer))
            .get<vrpn_float64>();
    d_quat[Q_Z] =
        FixedPointValue(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer))
            .get<vrpn_float64>();
    d_quat[Q_W] =
        FixedPointValue(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer))
            .get<vrpn_float64>();

    vrpn_Tracker::timestamp = _timestamp;
    char msgbuf[512];
    int len = vrpn_Tracker::encode_to(msgbuf);
    if (d_connection->pack_message(len, _timestamp, position_m_id, d_sender_id,
                                   msgbuf, vrpn_CONNECTION_LOW_LATENCY)) {
        fprintf(
            stderr,
            "vrpn_Tracker_OSVRHackerDevKit: cannot write message: tossing\n");
    }
}

void vrpn_Tracker_OSVRHackerDevKit::mainloop()
{
    vrpn_gettimeofday(&_timestamp, NULL);

    update();

    if (connected() && !_wasConnected) {
        send_text_message("Successfully connected to OSVR Hacker Dev Kit HMD.",
                          _timestamp, vrpn_TEXT_NORMAL);
    }
    _wasConnected = connected();

    server_mainloop();
}

#endif
