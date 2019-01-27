/** @file	vrpn_Laputa.C
@brief	Drivers for Laputa VR devices.

  @date 2016
  @copyright 2016 Sensics Inc.
  @author Sensics Inc, georgiy@sensics.com
  @license Standard VRPN license.
*/

// Based on the vrpn_Oculus driver

#include "vrpn_Laputa.h"

#include "vrpn_BaseClass.h" // for ::vrpn_TEXT_NORMAL, etc

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

// USB vendor and product IDs for the models we support
static const vrpn_uint16 LAPUTA_VENDOR = 0x2633;
static const vrpn_uint16 LAPUTA_PRODUCT = 0x0006;
static const vrpn_int32 NUM_CHANNELS = 11;

vrpn_Laputa::vrpn_Laputa(const char *name, vrpn_Connection *c)
    : vrpn_Analog(name, c)
    , vrpn_HidInterface(
          new vrpn_HidProductAcceptor(LAPUTA_VENDOR, LAPUTA_PRODUCT))
{

    vrpn_Analog::num_channel = NUM_CHANNELS;

    memset(channel, 0, sizeof(channel));
    memset(last, 0, sizeof(last));
}

vrpn_Laputa::~vrpn_Laputa()
{
  try {
    delete m_acceptor;
  } catch (...) {
    fprintf(stderr, "vrpn_Laputa::~vrpn_Laputa(): delete failed\n");
    return;
  }
}

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

void vrpn_Laputa::parse_message_type_1(std::size_t bytes, vrpn_uint8 *buffer)
{
    size_t num_reports = buffer[1];
    if (num_reports > 3) {
        num_reports = 3;
    }

    // Skip past the report type and num_reports bytes and
    // start parsing there.
    vrpn_uint8 *bufptr = &buffer[2];

    // The next two bytes are an increasing counter that changes by 1 for
    // every report.
    vrpn_uint16 report_index =
        vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);
    channel[1] = report_index;

    // The next two bytes are zero, so we skip them
    vrpn_uint16 skip =
        vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);

    // The next entry is temperature, and it may be in hundredths of a degree C
    const double temperature_scale = 0.01;
    vrpn_uint16 temperature =
        vrpn_unbuffer_from_little_endian<vrpn_uint16, vrpn_uint8>(bufptr);
    channel[0] = temperature * temperature_scale;

    // The magnetometer data comes after the space to store three
    // reports.
    vrpn_uint8 *magnetometer_ptr = &buffer[56];
    vrpn_int16 magnetometer_raw[3];
    for (size_t i = 0; i < 3; i++) {
        magnetometer_raw[i] =
            vrpn_unbuffer_from_little_endian<vrpn_int16, vrpn_uint8>(
                magnetometer_ptr);
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
        channel[2] =
            static_cast<double>(accelerometer_raw[0]) * accelerometer_scale;
        channel[3] =
            static_cast<double>(accelerometer_raw[1]) * accelerometer_scale;
        channel[4] =
            static_cast<double>(accelerometer_raw[2]) * accelerometer_scale;

        channel[5] = static_cast<double>(gyroscope_raw[0]) * gyroscope_scale;
        channel[6] = static_cast<double>(gyroscope_raw[1]) * gyroscope_scale;
        channel[7] = static_cast<double>(gyroscope_raw[2]) * gyroscope_scale;

        vrpn_Analog::report_changes();
    }
}

void vrpn_Laputa::mainloop()
{
    update();
    server_mainloop();
}

bool vrpn_Laputa::parse_message(std::size_t bytes, vrpn_uint8 *buffer)
{
    return false;
}

void vrpn_Laputa::on_data_received(std::size_t bytes, vrpn_uint8 *buffer)
{
    /*   // For debugging
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
    if ((bytes != 62) && (bytes != 64)) {
        fprintf(stderr, "vrpn_Laputa::on_data_received(): Unexpected message "
                        "length %d, ignoring\n",
                static_cast<int>(bytes));
        return;
    }

    switch (buffer[0]) {
    case 1:
        parse_message_type_1(bytes, buffer);
        break;
    default:
        // Delegate message type to child
        if (!parse_message(bytes, buffer)) {
            fprintf(stderr, "vrpn_Laputa::on_data_received(): Unexpected "
                            "message type %d, ignoring\n",
                    buffer[0]);
        }
        break;
    }
}

#endif
