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
#include <cmath>

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 vrpn_OSVR_VENDOR = 0x1532;
static const vrpn_uint16 vrpn_OSVR_HACKER_DEV_KIT_HMD = 0x0b00;

/// @todo Alternate, temporary VID/PID
static const vrpn_uint16 vrpn_OSVR_ALT_VENDOR = 0x03EB;
static const vrpn_uint16 vrpn_OSVR_ALT_HACKER_DEV_KIT_HMD = 0x2421;

// Every nth report, we'll force an analog report with the status data, even if
// no changes have occurred. 400 is the most common tracker rate (reports per
// second) so this is about once a second.
static const vrpn_uint16 vrpn_HDK_STATUS_STRIDE = 400;
static const vrpn_float64 vrpn_HDK_DT = 1.0 / 50;

static vrpn_HidAcceptor *makeHDKHidAcceptor()
{
  vrpn_HidAcceptor *ret;
  try {
    ret = new vrpn_HidBooleanOrAcceptor(
      new vrpn_HidProductAcceptor(vrpn_OSVR_VENDOR,
        vrpn_OSVR_HACKER_DEV_KIT_HMD),
      new vrpn_HidProductAcceptor(vrpn_OSVR_ALT_VENDOR,
        vrpn_OSVR_ALT_HACKER_DEV_KIT_HMD));
  } catch (...) { return NULL; }
  return ret;
}

// NOTE: Cannot use the vendor-and-product parameters in the
// vrpn_HidInterface because there are one of two possible
// vendor/product pairs.  The Acceptor will still correctly
// work, it will just do more work during the enumeration phase
// because it will have to check all devices in the system.
vrpn_Tracker_OSVRHackerDevKit::vrpn_Tracker_OSVRHackerDevKit(const char *name,
                                                             hid_device *dev,
                                                             vrpn_Connection *c)
    : vrpn_Tracker(name, c)
    , vrpn_Analog(name, c)
    , vrpn_HidInterface(makeHDKHidAcceptor(), dev)
    , _wasConnected(false)
    , _messageCount(0)
    , _reportVersion(0)
    , _knownVersion(true)
{
    shared_init();
}

vrpn_Tracker_OSVRHackerDevKit::vrpn_Tracker_OSVRHackerDevKit(const char *name,
                                                             vrpn_Connection *c)
    : vrpn_Tracker(name, c)
    , vrpn_Analog(name, c)
    , vrpn_HidInterface(makeHDKHidAcceptor())
    , _wasConnected(false)
    , _messageCount(0)
    , _reportVersion(0)
    , _knownVersion(true)
{
    shared_init();
}

void vrpn_Tracker_OSVRHackerDevKit::shared_init() {
    /// Tracker setup
    vrpn_Tracker::num_sensors = 1; // only orientation

    // Initialize the state
    std::memset(d_quat, 0, sizeof(d_quat));
    d_quat[Q_W] = 1.0;
    // Arbitrary dt that will be less than a full rotation.
    vel_quat_dt = vrpn_HDK_DT;

    /// Analog setup
    vrpn_Analog::num_channel = 2; // version and video input status

    // Set the timestamp
    vrpn_gettimeofday(&_timestamp, NULL);
}

vrpn_Tracker_OSVRHackerDevKit::~vrpn_Tracker_OSVRHackerDevKit()
{
  try {
    delete m_acceptor;
  } catch (...) {
    fprintf(stderr, "vrpn_Tracker_OSVRHackerDevKit::~vrpn_Tracker_OSVRHackerDevKit(): delete failed\n");
    return;
  }
}

void vrpn_Tracker_OSVRHackerDevKit::on_data_received(std::size_t bytes,
                                                     vrpn_uint8 *buffer)
{
    if (bytes != 32 && bytes != 16) {
        send_text_message(vrpn_TEXT_WARNING)
            << "Received a report " << bytes
            << " in length, but expected it to be 32 or 16 bytes. Discarding. "
               "(May indicate issues with HID!)";
        return;
    }

    vrpn_uint8 firstByte = vrpn_unbuffer_from_little_endian<vrpn_uint8>(buffer);

    vrpn_uint8 version = vrpn_uint8(0x0f) & firstByte;
    _reportVersion = version;

    switch (version) {
    case 1:
        if (bytes != 32 && bytes != 16) {
            send_text_message(vrpn_TEXT_WARNING)
                << "Received a v1 report " << bytes
                << " in length, but expected it to be 32 or 16 bytes. "
                   "Discarding. "
                   "(May indicate issues with HID!)";
            return;
        }
        break;
    case 2:
        if (bytes != 16) {
            send_text_message(vrpn_TEXT_WARNING)
                << "Received a v2 report " << bytes
                << " in length, but expected it to be 16 bytes. Discarding. "
                   "(May indicate issues with HID!)";
            return;
        }
        break;

    case 3:
        /// @todo once this report format is finalized, tighten up the
        /// requirements.
        if (bytes < 16) {
            send_text_message(vrpn_TEXT_WARNING)
                << "Received a v3 report " << bytes
                << " in length, but expected it to be at least 16 bytes. "
                   "Discarding. "
                   "(May indicate issues with HID!)";
            return;
        }
        break;
    default:
        /// Highlight that we don't know this report version well...
        _knownVersion = false;
        /// Do a minimal check of it.
        if (bytes < 16) {
            send_text_message(vrpn_TEXT_WARNING)
                << "Received a report claiming to be version " << int(version)
                << " that was " << bytes << " in length, but expected it to be "
                                            "at least 16 bytes. Discarding. "
                                            "(May indicate issues with HID!)";
            return;
        }
        break;
    }

    // Report version as analog channel 0.
    channel[0] = version;

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
    {
        char msgbuf[512];
        int len = vrpn_Tracker::encode_to(msgbuf);
        if (d_connection->pack_message(len, _timestamp, position_m_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
            fprintf(stderr, "vrpn_Tracker_OSVRHackerDevKit: cannot write "
                            "message: tossing\n");
        }
    }
    if (version >= 2) {
        // We've got angular velocity in this message too
        // Given XYZ radians per second velocity.
        // Signed Q6.9
        typedef vrpn::FixedPoint<6, 9> VelFixedPoint;
        q_vec_type angVel;
        angVel[0] =
            VelFixedPoint(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer))
                .get<vrpn_float64>();
        angVel[1] =
            VelFixedPoint(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer))
                .get<vrpn_float64>();
        angVel[2] =
            VelFixedPoint(vrpn_unbuffer_from_little_endian<vrpn_int16>(buffer))
                .get<vrpn_float64>();

        //==================================================================
        // Determine the rotational velocity, which is
        // measured in the rotated coordinate system.  We need to rotate the
        // difference Euler angles back to the canonical orientation, apply
        // the change, and then rotate back to change our coordinates.
        // Be sure to scale by the time value vrpn_HDK_DT.
        q_type forward, inverse;
        q_copy(forward, d_quat);
        q_invert(inverse, forward);
        // Remember that Euler angles in Quatlib have rotation around Z in
        // the first term.  Compute the time-scaled delta transform in
        // canonical space.
        q_type delta;
        {
            delta[Q_W] = 0;
            delta[Q_X] = angVel[Q_X] * vrpn_HDK_DT * 0.5;
            delta[Q_Y] = angVel[Q_Y] * vrpn_HDK_DT * 0.5;
            delta[Q_Z] = angVel[Q_Z] * vrpn_HDK_DT * 0.5;
            q_exp(delta, delta);
            q_normalize(delta, delta);
        }
        // Bring the delta back into canonical space
        q_type canonical;
        q_mult(canonical, delta, inverse);
        q_mult(vel_quat, forward, canonical);

        // Send the rotational velocity information.
        // The dt value was set once, in the constructor.
        char msgbuf[512];
        int len = vrpn_Tracker::encode_vel_to(msgbuf);
        if (d_connection->pack_message(len, _timestamp, velocity_m_id,
                                       d_sender_id, msgbuf,
                                       vrpn_CONNECTION_LOW_LATENCY)) {
            fprintf(stderr, "vrpn_Tracker_OSVRHackerDevKit: cannot write "
                            "message: tossing\n");
        }
    }
    if (version < 3) {
        // No status info hidden in the first byte.
        channel[1] = STATUS_UNKNOWN;
    }
    else {
        // v3+: We've got status info in the upper nibble of the first byte.
        bool gotVideo = (firstByte & (0x01 << 4)) != 0;    // got video?
        bool gotPortrait = (firstByte & (0x01 << 5)) != 0; // portrait mode?
        if (!gotVideo) {
            channel[1] = STATUS_NO_VIDEO_INPUT;
        }
        else {
            if (gotPortrait) {
                channel[1] = STATUS_PORTRAIT_VIDEO_INPUT;
            }
            else {
                channel[1] = STATUS_LANDSCAPE_VIDEO_INPUT;
            }
        }
    }

    if (_messageCount == 0) {
        // When _messageCount overflows, send a report whether or not there was
        // a change.
        vrpn_Analog::report();
    }
    else {
        // otherwise just report if we have a change.
        vrpn_Analog::report_changes();
    };
    _messageCount = (_messageCount + 1) % vrpn_HDK_STATUS_STRIDE;
}

void vrpn_Tracker_OSVRHackerDevKit::mainloop()
{
    vrpn_gettimeofday(&_timestamp, NULL);

    update();

    if (connected() && !_wasConnected) {
        send_text_message(vrpn_TEXT_NORMAL)
            << "Successfully connected to OSVR Hacker Dev Kit HMD, receiving "
               "version "
            << int(_reportVersion) << " reports.";

        if (!_knownVersion) {

            send_text_message(vrpn_TEXT_WARNING)
                << "Connected to OSVR Hacker Dev Kit HMD, receiving "
                   "version "
                << int(_reportVersion)
                << " reports, newer than what is specifically recognized. You "
                   "may want to update your server to best make use of this "
                   "new report format.";
        }
    }
    if (!connected()) {
        channel[0] = 0;
        channel[1] = STATUS_UNKNOWN;
        vrpn_Analog::report_changes();
    }

    _wasConnected = connected();

    if (!_wasConnected) {
        m_acceptor->reset();
        reconnect();
    }

    server_mainloop();
}

#endif
