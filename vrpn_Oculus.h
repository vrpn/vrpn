/** @file	vrpn_Oculus.h
	@brief	Header for various Oculus devices.  Initially, just the DK2.

	@date	2015

	@author
	Russell M. Taylor II
	<russ@reliasolve.com>
*/

// Based on the OSVR hacker dev kit driver by Kevin Godby.
// Based on Oliver Kreylos' OculusRiftHIDReports.cpp.

#pragma once

#include "vrpn_HumanInterface.h"
#include "vrpn_Analog.h"

#if defined(VRPN_USE_HID)

/// @brief Oculus Rift DK2 head-mounted display (inertial measurement unit only)
class VRPN_API vrpn_Oculus_DK2 : public vrpn_Analog, protected vrpn_HidInterface {
public:
    /**
     * @brief Constructor.
     *
     * @param name Name of this device.
     * @param c Optional vrpn_Connection pointer to use as our connection.
     * @param keepAliveSeconds How often to re-trigger the LED flashing
     */
    vrpn_Oculus_DK2(const char *name, vrpn_Connection *c = NULL,
      double keepAliveSeconds = 9.0);

    /**
     * @brief Destructor.
     */
    virtual ~vrpn_Oculus_DK2();

    virtual void mainloop();

protected:
  vrpn_HidAcceptor *m_filter;

  /// Extracts the sensor values from each report.
  void on_data_received(std::size_t bytes, vrpn_uint8 *buffer);

  /// Timestamp updated during mainloop()
  struct timeval d_timestamp;

  /// How often to send the keepAlive message to trigger the LEDs
  double d_keepAliveSeconds;
  struct timeval d_lastKeepAlive;

  // Send an LED control feature report. The enable flag tells
  // whether to turn on the LEDs (true) or not.
  void
    writeLEDControl(bool enable = true, vrpn_uint16 exposureLength = 350,
      vrpn_uint16 frameInterval = 16666
      , vrpn_uint16 vSyncOffset = 0
      , vrpn_uint8 dutyCycle = 127 //< 255 = 100% brightness
      , vrpn_uint8 pattern = 1
      , bool autoIncrement = true
      , bool useCarrier = true
      , bool syncInput = false
      , bool vSyncLock = false
      , bool customPattern = false
      , vrpn_uint16 commandId = 0 //< Should always be zero
      );

  // Send a KeepAlive feature report to the DK2.  This needs to be sent
  // every keepAliveSeconds to keep the LEDs going.
  void writeKeepAlive(
    bool keepLEDs = true //< Keep LEDs going, or only IMU?
    , vrpn_uint16 interval = 10000 //< KeepAlive time in milliseconds
    , vrpn_uint16 commandId = 0 //< Should always be zero
    );
};

#endif // VRPN_USE_HID
