/** @file	vrpn_Oculus.C
@brief	Drivers for various Oculus devices.  Initially, just the DK1 & DK2.

  @date 2015
  @copyright 2015 ReliaSolve.com
  @author ReliaSolve.com russ@reliasolve.com
  @license Standard VRPN license.
*/

// Based on the OSVR hacker dev kit driver by Kevin Godby.
// Based on Oliver Kreylos' OculusRiftHIDReports.cpp and OculusRift.cpp.
// He has given permission to release this code under the VRPN license:

#include "vrpn_Oculus.h"

#include "vrpn_BaseClass.h"  // for ::vrpn_TEXT_NORMAL, etc

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 OCULUS_VENDOR = 0x2833;
static const vrpn_uint16 DK1_PRODUCT = 0x0001;
static const vrpn_uint16 DK2_PRODUCT = 0x0021;

vrpn_Oculus::vrpn_Oculus(vrpn_uint16 product_id, vrpn_uint8 num_channels,
  const char *name, vrpn_Connection *c, double keepAliveSeconds)
    : vrpn_Analog(name, c)
    , vrpn_HidInterface(m_filter =
      new vrpn_HidProductAcceptor(OCULUS_VENDOR, product_id))
{
  d_keepAliveSeconds = keepAliveSeconds;

  vrpn_Analog::num_channel = num_channels;

  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));

  // Set the timestamp
  vrpn_gettimeofday(&d_timestamp, NULL);
  d_lastKeepAlive = d_timestamp;
}

vrpn_Oculus::~vrpn_Oculus()
{
  try {
    delete m_acceptor;
  } catch (...) {
    fprintf(stderr, "vrpn_Oculus::~vrpn_Oculus(): delete failed\n");
    return;
  }
}

// Copied from Oliver Kreylos' OculusRift.cpp; he has given permission
// to release this code under the VRPN standard license.
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

void vrpn_Oculus::parse_message_type_1(std::size_t bytes,
  vrpn_uint8 *buffer)
{
  size_t num_reports = buffer[1];
  if (num_reports > 3) { num_reports = 3; }

  // Skip past the report type and num_reports bytes and
  // start parsing there.
  vrpn_uint8 *bufptr = &buffer[2];

  // The next two bytes are an increasing counter that changes by 1 for
  // every report.
  vrpn_uint16 report_index;
  report_index = vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);
  channel[1] = report_index;

  // The next two bytes are zero, so we skip them
  vrpn_uint16 skip;
  skip = vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);

  // The next entry is temperature, and it may be in hundredths of a degree C
  vrpn_uint16 temperature;
  const double temperature_scale = 0.01;
  temperature = vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);
  channel[0] = temperature * temperature_scale;

  // The magnetometer data comes after the space to store three
  // reports.
  vrpn_uint8 *magnetometer_ptr = &buffer[56];
  vrpn_int16 magnetometer_raw[3];
  for (size_t i = 0; i < 3; i++) {
    magnetometer_raw[i] = vrpn_unbuffer_from_little_endian
      <vrpn_int16, vrpn_uint8>(magnetometer_ptr);
  }
  // Invert all axes to make the magnetometer direction match
  // the sign of the gravity vector.
  const double magnetometer_scale = 0.0001;
  channel[8] = -magnetometer_raw[0] * magnetometer_scale;
  channel[9] = -magnetometer_raw[1] * magnetometer_scale;
  channel[10] = -magnetometer_raw[2] * magnetometer_scale;

  // Unpack a 16-byte accelerometer/gyro report using the routines from
  // Oliver's code.
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
}

void vrpn_Oculus::mainloop()
{
  vrpn_gettimeofday(&d_timestamp, NULL);

  // See if it has been long enough to send another keep-alive 
  if (vrpn_TimevalDurationSeconds(d_timestamp, d_lastKeepAlive) >=
      d_keepAliveSeconds) {
    writeKeepAlive();
    d_lastKeepAlive = d_timestamp;
  }

  update();
  server_mainloop();
}

bool vrpn_Oculus::parse_message(std::size_t bytes,
  vrpn_uint8 *buffer)
{
  return false;
}

void vrpn_Oculus::on_data_received(std::size_t bytes,
  vrpn_uint8 *buffer)
{
  /* For debugging
     printf("Got %d bytes:\n", static_cast<int>(bytes));
     for (size_t i = 0; i < bytes; i++) {
     printf("%02X ", buffer[i]);
     }
     printf("\n");
     */

  // Set the timestamp for all reports
  vrpn_gettimeofday(&d_timestamp, NULL);

  // Make sure the message length and type is what we expect.
  // We get 64-byte responses on Windows and 62-byte responses on the mac.
  if ( (bytes != 62) && (bytes != 64) ) {
    fprintf(stderr, "vrpn_Oculus::on_data_received(): Unexpected message length %d, ignoring\n",
        static_cast<int>(bytes));
    return;
  }

  switch(buffer[0]) {
    case 1:
      parse_message_type_1(bytes, buffer);
      break;
    default:
      // Delegate message type to child
      if (!parse_message(bytes, buffer)) {
        fprintf(stderr, "vrpn_Oculus::on_data_received(): Unexpected message type %d, ignoring\n",
            buffer[0]);
      }
      break;
  }
}

vrpn_Oculus_DK1::vrpn_Oculus_DK1(const char *name, 
  vrpn_Connection *c, double keepAliveSeconds)
    : vrpn_Oculus(DK1_PRODUCT, 11, 
      name, c, keepAliveSeconds) {};

// Thank you to Oliver Kreylos for the info needed to write this function.
// It is based on his OculusRiftHIDReports.cpp, used with permission.
void vrpn_Oculus_DK1::writeKeepAlive(
  vrpn_uint16 interval
  , vrpn_uint16 commandId)
{
  // Buffer to store our report in.
  vrpn_uint8 pktBuffer[5];

  /* Pack the packet buffer, using little-endian packing: */
  vrpn_uint8 *bufptr = pktBuffer;
  vrpn_int32 buflen = sizeof(pktBuffer);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, vrpn_uint8(0x08U));
  vrpn_buffer_to_little_endian(&bufptr, &buflen, commandId);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, interval);

  /* Write the feature report: */
  send_feature_report(sizeof(pktBuffer), pktBuffer);
}

vrpn_Oculus_DK2::vrpn_Oculus_DK2(bool enableLEDs,
  const char *name, vrpn_Connection *c,
  double keepAliveSeconds)
    : vrpn_Oculus(DK2_PRODUCT, enableLEDs ? 12 : 11, 
      name, c, keepAliveSeconds)
{
  d_enableLEDs = enableLEDs;
}

bool vrpn_Oculus_DK2::parse_message(std::size_t bytes,
  vrpn_uint8 *buffer)
{
  switch (buffer[0]) {
    case 11:
      parse_message_type_11(bytes, buffer);
      break;
    default:
      return false;
  }
  return true;
}

// Thank you to Oliver Kreylos for the info needed to write this function.
// The actual order and meaning of fields was determined by walking through
// the packet to see what was in there, but the vector-decoding routines
// are used to pull out the inertial sensor data.

void vrpn_Oculus_DK2::parse_message_type_11(std::size_t bytes,
  vrpn_uint8 *buffer)
{
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

  // Skip the first four bytes and start parsing the other reports from there.
  vrpn_uint8 *bufptr = &buffer[4];  // Point at the start of the report

  // The next two bytes seem to be an increasing counter that changes by 1 for
  // every report.
  vrpn_uint16 report_index, temperature;
  report_index = vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);
  channel[1] = report_index;

  // The next entry may be temperature, and it may be in hundredths of a degree C
  const double temperature_scale = 0.01;
  temperature = vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);
  channel[0] = temperature * temperature_scale;

  // The next entry is a 4-byte counter with time since device was
  // powered on in microseconds.  We convert to seconds and report.
  vrpn_uint32 microseconds;
  microseconds = vrpn_unbuffer_from_little_endian<vrpn_uint32, vrpn_uint8>(bufptr);
  channel[11] = microseconds * 1e-6;

  // Unpack a 16-byte accelerometer/gyro report using the routines from
  // Oliver's code.  Also the magnetometer values(s).
  // Then convert each to a scaled double value and send a report.
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

    // The magnetometer data comes after the space to store two
    // reports.  We don't know if there are two of them when
    // there are two reports, but there is space in the report
    // for it, so we try to decode two of them.
    vrpn_uint8 *magnetometer_ptr = &buffer[44 + 8 * i];
    vrpn_int16 magnetometer_raw[3];
    for (size_t i = 0; i < 3; i++) {
      magnetometer_raw[i] = vrpn_unbuffer_from_little_endian
        <vrpn_int16, vrpn_uint8>(magnetometer_ptr);
    }
    // Invert these to make the magnetometer direction match
    // the sign of the gravity vector.
    const double magnetometer_scale = - 0.0001;
    channel[8] = -magnetometer_raw[0] * magnetometer_scale;
    channel[9] = -magnetometer_raw[1] * magnetometer_scale;
    channel[10] = -magnetometer_raw[2] * magnetometer_scale;

    vrpn_Analog::report_changes();
  }
}


// Thank you to Oliver Kreylos for the info needed to write this function.
// It is based on his OculusRiftHIDReports.cpp, used with permission.
void vrpn_Oculus_DK2::writeKeepAlive(
  vrpn_uint16 interval
  , vrpn_uint16 commandId)
{
  // Buffer to store our report in.
  vrpn_uint8 pktBuffer[6];

  /* Pack the packet buffer, using little-endian packing: */
  vrpn_uint8 *bufptr = pktBuffer;
  vrpn_int32 buflen = sizeof(pktBuffer);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, vrpn_uint8(0x11U));
  vrpn_buffer_to_little_endian(&bufptr, &buflen, commandId);
  vrpn_uint8 flags = d_enableLEDs ? 0x0bU : 0x01U;
  vrpn_buffer_to_little_endian(&bufptr, &buflen, flags);
  vrpn_buffer_to_little_endian(&bufptr, &buflen, interval);

  /* Write the LED control feature report: */
  send_feature_report(sizeof(pktBuffer), pktBuffer);
}

#endif
