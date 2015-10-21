/** @file	vrpn_Oculus.C
@brief	Drivers for various Oculus devices.  Initially, just the DK2.

@date	2015

@author
Russell M. Taylor II
<russ@reliasolve.com>
*/

// Based on the OSVR hacker dev kit driver by Kevin Godby.
// Based on Oliver Kreylos' OculusRiftHIDReports.cpp, used with permission.

#include "vrpn_Oculus.h"

#include "vrpn_BaseClass.h"  // for ::vrpn_TEXT_NORMAL, etc

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 OCULUS_VENDOR = 0x2833;
static const vrpn_uint16 DK2_PRODUCT = 0x0021;

vrpn_Oculus_DK2::vrpn_Oculus_DK2(const char *name, vrpn_Connection *c,
  double keepAliveSeconds)
    : vrpn_Analog(name, c)
    , vrpn_HidInterface(m_filter =
      new vrpn_HidProductAcceptor(OCULUS_VENDOR, DK2_PRODUCT))
{
  d_keepAliveSeconds = keepAliveSeconds;

  vrpn_Analog::num_channel = 6;
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));

  // Set the timestamp
  vrpn_gettimeofday(&d_timestamp, NULL);
  d_lastKeepAlive = d_timestamp;
}

vrpn_Oculus_DK2::~vrpn_Oculus_DK2()
{
  delete m_acceptor;
}

void vrpn_Oculus_DK2::on_data_received(std::size_t bytes,
                                                     vrpn_uint8 *buffer)
{
    if (bytes != 32 && bytes != 16) {
      send_text_message("Unexpected report length", d_timestamp,
        vrpn_TEXT_WARNING);
        return;
    }

    // @todo Parse the data and fill in the structures.
}

void vrpn_Oculus_DK2::mainloop()
{
    vrpn_gettimeofday(&d_timestamp, NULL);

    // See if it has been long enough to send another keep-alive to
    // the LEDs.
    if (vrpn_TimevalDurationSeconds(d_timestamp, d_lastKeepAlive) >=
        d_keepAliveSeconds) {
      writeKeepAlive();
      d_lastKeepAlive = d_timestamp;
    }

    update();
    server_mainloop();
}

// Thank you to Oliver Kreylos for the info needed to write this function.
// It is based on his OculusRiftHIDReports.cpp, used with permission.
void vrpn_Oculus_DK2::writeLEDControl(
  bool enable
  , vrpn_uint16 exposureLength
  , vrpn_uint16 frameInterval
  , vrpn_uint16 vSyncOffset
  , vrpn_uint8 dutyCycle
  , vrpn_uint8 pattern
  , bool autoIncrement
  , bool useCarrier
  , bool syncInput
  , bool vSyncLock
  , bool customPattern
  , vrpn_uint16 commandId)
{
  // Buffer to store our report in.
  vrpn_uint8 pktBuffer[13];

  /* Pack the packet buffer, using little-endian packing: */
  vrpn_uint8 *bufptr = pktBuffer;
  vrpn_int32 buflen = sizeof(pktBuffer);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, vrpn_uint8(0x0cU));
  vrpn_buffer_to_little_endian(&bufptr, &buflen, commandId);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, pattern);
  vrpn_uint8 flags = 0x00U;
  if (enable) {
    flags |= 0x01U;
  }
  if (autoIncrement) {
    flags |= 0x02U;
  }
  if (useCarrier) {
    flags |= 0x04U;
  }
  if (syncInput) {
    flags |= 0x08U;
  }
  if (vSyncLock) {
    flags |= 0x10U;
  }
  if (customPattern) {
    flags |= 0x20U;
  }
  vrpn_buffer_to_little_endian(&bufptr, &buflen, flags);
  vrpn_buffer_to_little_endian(&bufptr, &buflen,
    vrpn_uint8(0x0cU)); // Reserved byte
  vrpn_buffer_to_little_endian(&bufptr, &buflen, exposureLength);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, frameInterval);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, vSyncOffset);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, dutyCycle);

  /* Write the LED control feature report: */
  send_feature_report(sizeof(pktBuffer), pktBuffer);
}

// Thank you to Oliver Kreylos for the info needed to write this function.
// It is based on his OculusRiftHIDReports.cpp, used with permission.
void vrpn_Oculus_DK2::writeKeepAlive(
  bool keepLEDs
  , vrpn_uint16 interval
  , vrpn_uint16 commandId)
{
  // Buffer to store our report in.
  vrpn_uint8 pktBuffer[6];

  /* Pack the packet buffer, using little-endian packing: */
  vrpn_uint8 *bufptr = pktBuffer;
  vrpn_int32 buflen = sizeof(pktBuffer);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, vrpn_uint8(0x11U));
  vrpn_buffer_to_little_endian(&bufptr, &buflen, commandId);
  vrpn_uint8 flags = keepLEDs ? 0x0bU : 0x01U;
  vrpn_buffer_to_little_endian(&bufptr, &buflen, flags);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, interval);

  /* Write the LED control feature report: */
  send_feature_report(sizeof(pktBuffer), pktBuffer);
}


#endif
