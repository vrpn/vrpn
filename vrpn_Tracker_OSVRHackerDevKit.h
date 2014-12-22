/** @file	vrpn_Tracker_OSVRHackerDevKit.h
	@brief	header for OSVR Hacker Dev Kit

	@date	2014

	@author
	Kevin M. Godby
	<kevin@godby.org>
*/

#ifndef VRPN_TRACKER_OSVR_HACKER_DEV_KIT_H_
#define VRPN_TRACKER_OSVR_HACKER_DEV_KIT_H_

#include <cstddef>                      // for size_t
#include <string>                       // for string

#include "vrpn_Tracker.h"               // for vrpn_Tracker
#include "vrpn_Configure.h"             // for VRPN_API, VRPN_USE_HID
#include "vrpn_Connection.h"            // for vrpn_Connection (ptr only), etc
#include "vrpn_HumanInterface.h"        // for vrpn_HIDDEVINFO, etc
#include "vrpn_Shared.h"                // for timeval
#include "vrpn_Types.h"                 // for vrpn_uint16, vrpn_uint32, etc

#if defined(VRPN_USE_HID)

/** @brief OSVR Hacker Dev Kit HMD
 * The official name of the Razer/Sensics HMD (until they change it again…) is “OSVR Hacker Dev Kit.”
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
 *   0: Version number, currently 1
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
 *   10-31: reserved for future use
 *
 * Each quaternion is presented as signed, 16-bit fixed point, 2’s complement
 * number with a Q point of 14.
 */
class VRPN_API vrpn_Tracker_OSVRHackerDevKit : public vrpn_Tracker, protected vrpn_HidInterface {
public:
    /**
     * @brief Constructor.
     *
     * @param filter HID acceptor.
     * @param name Name of tracker.
     * @param c Optional vrpn_Connection.
     */
    vrpn_Tracker_OSVRHackerDevKit(const char *name, vrpn_Connection *c = NULL);

    /**
     * @brief Destructor.
     */
    virtual ~vrpn_Tracker_OSVRHackerDevKit();

    /**
     * @brief Standard VRPN mainloop method.
     */
    virtual void mainloop();

protected:

    /// Extracts the sensor values from each report.
    void on_data_received(std::size_t bytes, vrpn_uint8 *buffer);

    /// Timestamp updated during mainloop()
    struct timeval _timestamp;

    /// Flag indicating whether we were connected last time through the mainloop.
    /// Used to send a "normal"-severity message when we connect with info on the
    /// device.
    bool _wasConnected;
};

#endif // VRPN_USE_HID

#endif // VRPN_TRACKER_OSVR_HACKER_DEV_KIT_H_

