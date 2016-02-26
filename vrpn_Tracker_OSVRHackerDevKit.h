/** @file	vrpn_Tracker_OSVRHackerDevKit.h
	@brief	header for OSVR Hacker Dev Kit

	@date	2014

	@author
	Kevin M. Godby
	<kevin@godby.org>
*/

#ifndef VRPN_TRACKER_OSVR_HACKER_DEV_KIT_H_
#define VRPN_TRACKER_OSVR_HACKER_DEV_KIT_H_

#include <cstddef> // for size_t
#include <string>  // for string

#include "vrpn_Tracker.h"        // for vrpn_Tracker
#include "vrpn_Analog.h"         // for vrpn_Analog
#include "vrpn_Configure.h"      // for VRPN_API, VRPN_USE_HID
#include "vrpn_Connection.h"     // for vrpn_Connection (ptr only), etc
#include "vrpn_HumanInterface.h" // for vrpn_HIDDEVINFO, etc
#include "vrpn_Shared.h"         // for timeval
#include "vrpn_Types.h"          // for vrpn_uint16, vrpn_uint32, etc

#if defined(VRPN_USE_HID)

/** @brief OSVR Hacker Dev Kit HMD
 * The official name of the Razer/Sensics HMD (until they change it again...) is
 * “OSVR Hacker Dev Kit.”
 *
 * The devkit will have a built-in head tracker.
 *
 *   VID 0x1532
 *   PID 0x0B00
 *
 * The head tracker shows as a generic HID device
 *
 * Protocol for it is as follows (in byte offsets):
 *
 *   0:
 *      Bits 0-3: Version number, currently 1, 2, or 3
 *      Upper nibble holds additional data for v3+:
 *      Bit 4: 1 if video detected, 0 if not.
 *      Bit 5: 1 if portrait, 0 if landscape
 *
 *   1: message sequence number (8 bit)
 *
 *   2: Unit quaternion i component LSB
 *   3: Unit quaternion i component MSB
 *
 *   4: Unit quaternion j component LSB
 *   5: Unit quaternion j component MSB
 *
 *   6: Unit quaternion k component LSB
 *   7: Unit quaternion k component MSB
 *
 *   8: Unit quaternion real component LSB
 *   9: Unit quaternion real component MSB
 *
 *   10: Instantaneous angular velocity about X LSB
 *   11: Instantaneous angular velocity about X MSB
 *
 *   12: Instantaneous angular velocity about Y LSB
 *   13: Instantaneous angular velocity about Y MSB
 *
 *   14: Instantaneous angular velocity about Z LSB
 *   15: Instantaneous angular velocity about Z MSB
 *
 * Each quaternion is presented as signed, 16-bit fixed point, 2’s complement
 * number with a Q point of 14. The components of angular velocity are signed,
 * 16-bit fixed point, 2's complement with Q of 9. Reports are either 32 (in old
 * firmware, early v1 reports) or 16 bytes (most firmware, both v1 and v2) long,
 * and only v2+ reports contain angular velocity data.
 *
 * v3+ reports also contain additional data in the upper nibble of the version
 * number.
 *
 * Report version number is exposed as Analog channel 0.
 * Analog channel 1 should be interpreted as follows:
 *    0: Status unknown - reports are < v3 or no connection.
 *    1: No video input.
 *    2: Portrait video input detected.
 *    3: Landscape video input detected.
 */
class VRPN_API vrpn_Tracker_OSVRHackerDevKit : public vrpn_Tracker,
                                               public vrpn_Analog,
                                               protected vrpn_HidInterface {
public:
    /**
     * @brief Constructor.
     *
     * @param name Name of tracker.
     * @param dev Optional Already-opened HIDAPI device for the tracker.
     * @param c Optional vrpn_Connection.
     */
    vrpn_Tracker_OSVRHackerDevKit(const char *name, hid_device *dev = NULL,
                                  vrpn_Connection *c = NULL);

    /**
     * @overload
     */
    vrpn_Tracker_OSVRHackerDevKit(const char *name, vrpn_Connection *c);

    /**
     * @brief Destructor.
     */
    virtual ~vrpn_Tracker_OSVRHackerDevKit();

    /**
     * @brief Standard VRPN mainloop method.
     */
    virtual void mainloop();

    enum Status {
        STATUS_UNKNOWN = 0,
        STATUS_NO_VIDEO_INPUT = 1,
        STATUS_PORTRAIT_VIDEO_INPUT = 2,
        STATUS_LANDSCAPE_VIDEO_INPUT = 3
    };

protected:
    /// Extracts the sensor values from each report.
    void on_data_received(std::size_t bytes, vrpn_uint8 *buffer);
    void shared_init();

    /// Timestamp updated during mainloop()
    struct timeval _timestamp;

    /// Flag indicating whether we were connected last time through the
    /// mainloop.
    /// Used to send a "normal"-severity message when we connect with info on
    /// the device and to handle re-connecting after a USB disconnect.
    bool _wasConnected;

    /// Used to forcibly send the analog update every so often
    vrpn_uint16 _messageCount;

    vrpn_uint8 _reportVersion;
    bool _knownVersion;
};

#endif // VRPN_USE_HID

#endif // VRPN_TRACKER_OSVR_HACKER_DEV_KIT_H_
