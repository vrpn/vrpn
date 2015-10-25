/** @file	vrpn_Oculus.C
@brief	Drivers for various Oculus devices.  Initially, just the DK2.

@date	2015

@author
Russell M. Taylor II
<russ@reliasolve.com>
*/

// Based on the OSVR hacker dev kit driver by Kevin Godby.
// Based on Oliver Kreylos' OculusRiftHIDReports.cpp and OculusRift.cpp.

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

  vrpn_Analog::num_channel = 11;
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

// Copied from Oliver Kreylos' OculusRift.cpp.
// Unpacks a 3D vector from 8 bytes
inline void unpackVector(const vrpn_uint8 raw[8], int vector[3])
{
  // @todo Make this also work on big-endian architectures.
  union // Helper union to assemble 3 or 4 bytes into a signed 32-bit integer
  {
    vrpn_uint8 b[4];
    vrpn_int32 i;
  } p;
  struct // Helper structure to sign-extend a 21-bit signed integer value
  {
    signed int si : 21;
  } s;

  /* Assemble the vector's x component: */
  p.b[0] = raw[2];
  p.b[1] = raw[1];
  p.b[2] = raw[0];
  // p.b[3]=0U; // Not needed because it's masked out below anyway
  vector[0] = s.si = (p.i >> 3) & 0x001fffff;

  /* Assemble the vector's y component: */
  p.b[0] = raw[5];
  p.b[1] = raw[4];
  p.b[2] = raw[3];
  p.b[3] = raw[2];
  vector[1] = s.si = (p.i >> 6) & 0x001fffff;

  /* Assemble the vector's z component: */
  p.b[0] = raw[7];
  p.b[1] = raw[6];
  p.b[2] = raw[5];
  // p.b[3]=0U; // Not needed because it's masked out below anyway
  vector[2] = s.si = (p.i >> 1) & 0x001fffff;
}

// Thank you to Oliver Kreylos for the info needed to write this function.
// It is based on his OculusRiftHIDReports.cpp and OculusRift.cpp.
void vrpn_Oculus_DK2::on_data_received(std::size_t bytes,
  vrpn_uint8 *buffer)
{
  /* We're getting 64-byte messages of type 11 when the example code
     seems to suggest we want messages of type 1.  Maybe we need to
     set the DK2 into a different mode.
  printf("XXX Got %d bytes\n", static_cast<int>(bytes));
  if (buffer[0] == 0x01) {
    printf("XXX Got report\n");
  } else {
    printf("XXX Unexpected message type: %d\n", buffer[0]);
    for (size_t i = 0; i < bytes; i++) {
      printf("%02X ", buffer[i]);
    }
    printf("\n");
  }
  */

  // Make sure the message length is what we expect.
  if (bytes != 64) {
    fprintf(stderr, "vrpn_Oculus_DK2::on_data_received(): Unexpected message length %d, ignoring",
      static_cast<int>(bytes));
    return;
  }

  // Set the timestamp for all reports
  vrpn_gettimeofday(&d_timestamp, NULL);

#if 0
  // This was from OculusRift.cpp and seems not to match our readings...
  // The first two bytes in the message are ignored; they are not present in the
  // HID report for Oliver's code.  The third byte is 0 and the fourth is the number
  // of reports (up to 3).
  size_t num_reports = buffer[3];
  if (num_reports > 3) { num_reports = 3; }
  vrpn_int16 magnetometer_raw[3];
  vrpn_uint8 *magnetometer_base = &buffer[2 + 56];
  for (size_t i = 0; i < 3; i++) {
    magnetometer_raw[i] = vrpn_unbuffer_from_little_endian
      <vrpn_int16, vrpn_uint8>(magnetometer_base);
  }
  printf("XXX Magnetometer_raw = %d, %d, %d\n", magnetometer_raw[0], magnetometer_raw[1], magnetometer_raw[2]);

  // Upack the raw reports for the accelerometer and gyroscope
  // for each available sample.  Then convert each to a scaled
  // double value and send a report.
  for (size_t i = 0; i < num_reports; i++) {
    vrpn_int32 accelerometer_raw[3];
    vrpn_int32 gyroscope_raw[3];
    unpackVector(&buffer[2 + 8 + i * 16], accelerometer_raw);
    unpackVector(&buffer[2 +16 + i * 16], gyroscope_raw);

    // Compute the double values using default calibration.
    // The accelerometer data goes into analogs 0,1,2.
    // The gyro data goes into analogs 3,4,5.
    // The magnetomoter data goes into analogs 6,7,8.
    const double accelerometer_scale = 0.0001;
    const double gyroscope_scale = 0.0001;
    const double magnetometer_scale = 0.0001;
    channel[0] = accelerometer_raw[0] * accelerometer_scale;
    channel[1] = accelerometer_raw[1] * accelerometer_scale;
    channel[2] = accelerometer_raw[2] * accelerometer_scale;

    channel[3] = gyroscope_raw[0] * gyroscope_scale;
    channel[4] = gyroscope_raw[1] * gyroscope_scale;
    channel[5] = gyroscope_raw[2] * gyroscope_scale;

    channel[6] = magnetometer_raw[0] * magnetometer_scale;
    channel[7] = magnetometer_raw[1] * magnetometer_scale;
    channel[8] = magnetometer_raw[2] * magnetometer_scale;
    vrpn_Analog::report_changes();
  }
#endif

  // Started with the two different layouts in Oliver's code using my DK2
  // and could not get the required data from it.  I'm getting 64-byte
  // reports that are somewhat like the 62-byte reports that code has but
  // not the same.  They seem closer to the code form the OculusRiftHIDReports.cpp
  // document, but not the same (the last bytes are always 0, and they are supposed
  // to be the magnetometer, for example).
  // Looked at how the values changed and tried to figure out how to parse each
  // part of the file.
  // The first two bytes in the message are ignored; they are not present in the
  // HID report for Oliver's code.  The third byte is 0 and the fourth is the number
  // of reports (up to 2, according to how much space is before the magnetometer
  // data in the reports we found).
  size_t num_reports = buffer[3];
  if (num_reports > 2) { num_reports = 2; }

  // The magnetometer data comes after the space to store two
  // reports.  We read it first because it seems to apply to
  // all entries in the file (according to the old parsing code;
  // it may be that we get one per report now).
  // @todo Check to see if we get multiple magnetometer reports
  // when we get multiple other reports (I never see multiple
  // other reports from my unit).
  vrpn_uint8 *bufptr;
  bufptr = &buffer[44];
#if 1
  vrpn_int16 magnetometer_raw[3];
  for (size_t i = 0; i < 3; i++) {
    magnetometer_raw[i] = vrpn_unbuffer_from_little_endian
      <vrpn_int16, vrpn_uint8>(bufptr);
  }
#else
  vrpn_int32 magnetometer_raw[3];
  for (size_t i = 0; i < 3; i++) {
    unpackVector(bufptr, magnetometer_raw);
  }
#endif
  const double magnetometer_scale = 0.0001;
  channel[8] = magnetometer_raw[0] * magnetometer_scale;
  channel[9] = magnetometer_raw[1] * magnetometer_scale;
  channel[10] = magnetometer_raw[2] * magnetometer_scale;
  // @todo This is not the magnetometer; it may be parts of two different
  // readings or it may be some sort of strange up vector that needs to be
  // normalized, or it may be a vector perpendicular to North.  Check and see
  // if we have skipped too far ahead and need to be looking at other bytes.
  printf("XXX Magnetometer = %7.3lf, %7.3lf, %7.3lf\n", channel[8], channel[9], channel[10]);

  // Skip the first four bytes and start parsing the other reports from there.
  bufptr = &buffer[4];  // Point at the start of the report

  // The next two bytes seem to be an increasing counter that changes by 1 for
  // every report.
  vrpn_uint16 report_index, temperature;
  report_index = vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);

  // The next entry may be temperature, and it may be in hundredths of a degree C
  const double temperature_scale = 0.01;
  temperature = vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);
  channel[0] = temperature * temperature_scale;

  // The next entry is a 4-byte counter with time since device was
  // powered on in microseconds.  We convert to seconds and report.
  vrpn_uint32 microseconds;
  microseconds = vrpn_unbuffer_from_little_endian<vrpn_uint32, vrpn_uint8>(bufptr);
  channel[1] = microseconds * 1e-6;

  // Unpack a 16-byte accelerometer/gyro report using the routines from
  // Oliver's code.
  // Upack the raw reports for the accelerometer and gyroscope
  // for each available sample.  Then convert each to a scaled
  // double value and send a report.
  for (size_t i = 0; i < num_reports; i++) {
    vrpn_int32 accelerometer_raw[3];
    vrpn_int32 gyroscope_raw[3];
    unpackVector(bufptr, accelerometer_raw);
    bufptr += 8;
    unpackVector(bufptr, gyroscope_raw);
    bufptr += 8;

    // Compute the double values using default calibration.
    // The accelerometer data goes into analogs 0,1,2.
    // The gyro data goes into analogs 3,4,5.
    // The magnetomoter data goes into analogs 6,7,8.
    const double accelerometer_scale = 0.0001;
    const double gyroscope_scale = 0.0001;
    channel[2] = accelerometer_raw[0] * accelerometer_scale;
    channel[3] = accelerometer_raw[1] * accelerometer_scale;
    channel[4] = accelerometer_raw[2] * accelerometer_scale;

    channel[5] = gyroscope_raw[0] * gyroscope_scale;
    channel[6] = gyroscope_raw[1] * gyroscope_scale;
    channel[7] = gyroscope_raw[2] * gyroscope_scale;
    vrpn_Analog::report_changes();
  }
  return;
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
