// vrpn_Vality.C: VRPN driver for Vality devices

#include <stdio.h>                      // for fprintf, stderr, NULL
#include <string.h>                     // for memset
#include <math.h>                       // for fabs

#include "vrpn_Vality.h"
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR
VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#if defined(VRPN_USE_HID)

static const double POLL_INTERVAL = 1e+6 / 30.0;		// If we have not heard, ask.

// USB vendor and product IDs for the models we support
static const vrpn_uint16 VALITY_VENDOR = 0x0483;
static const vrpn_uint16 VALITY_VGLASS = 0x5740;

vrpn_Vality::vrpn_Vality(vrpn_HidAcceptor *filter, const char *name,
                          vrpn_Connection *c,
    vrpn_uint16 vendor, vrpn_uint16 product)
  : vrpn_BaseClass(name, c)
  , vrpn_HidInterface(filter, vendor, product)
  , _filter(filter)
{
	init_hid();
}

vrpn_Vality::~vrpn_Vality(void)
{
  try {
    delete _filter;
  } catch (...) {
    fprintf(stderr, "vrpn_Vality::~vrpn_Vality(): delete failed\n");
    return;
  }
}

void vrpn_Vality::init_hid(void)
{
	// Get notifications when clients connect and disconnect
	register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), on_last_disconnect, this);
	register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), on_connect, this);
}

void vrpn_Vality::on_data_received(size_t bytes, vrpn_uint8 *buffer)
{
  decodePacket(bytes, buffer);
}

int vrpn_Vality::on_last_disconnect(void * /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

int vrpn_Vality::on_connect(void * /*thisPtr*/, vrpn_HANDLERPARAM /*p*/)
{
	return 0;
}

vrpn_Vality_vGlass::vrpn_Vality_vGlass(const char *name,
                                                      vrpn_Connection *c)
    : vrpn_Vality(
          new vrpn_HidProductAcceptor(VALITY_VENDOR, VALITY_VGLASS),
          name, c, VALITY_VENDOR, VALITY_VGLASS)
  , vrpn_Analog(name, c)
{
  vrpn_Analog::num_channel = 9;

  // Initialize the state of all the analogs
  memset(channel, 0, sizeof(channel));
  memset(last, 0, sizeof(last));
}

void vrpn_Vality_vGlass::mainloop(void)
{
	update();
	server_mainloop();
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);
	if (vrpn_TimevalDuration(current_time, _timestamp) > POLL_INTERVAL ) {
		_timestamp = current_time;
		report_changes();

    // Call the server_mainloop on our unique base class.
    server_mainloop();
	}
}

void vrpn_Vality_vGlass::report(vrpn_uint32 class_of_service)
{
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = _timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report(class_of_service);
	}
}

void vrpn_Vality_vGlass::report_changes(vrpn_uint32 class_of_service)
{
	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::timestamp = _timestamp;
	}

	if (vrpn_Analog::num_channel > 0)
	{
		vrpn_Analog::report_changes(class_of_service);
	}
}

void vrpn_Vality_vGlass::decodePacket(size_t bytes, vrpn_uint8 *buffer)
{
    // There is only one type of report, which is 62 bytes long on Windows but
    // which only contains data in the first 24 bytes, so we just make sure that
    // the report is long enough and starts with 1 (the proper report type).
    if (bytes < 24) {
        VRPN_MSG_ERROR("Message too short, ignoring");
        return;
    }

    if (buffer[0] != 1) {
        VRPN_MSG_ERROR("Unrecognized message type, ignoring");
        return;
    }

    // Decode the report, storing our values into the analog channels.
    uint8_t *bufptr;

    // Decode the accelerometer values into channels 0=X, 1=Y, 2=Z.
    // Converts from [-1..1] to meters/second/second using the typical
    // sensitivity from the data sheet and the range set in the initialization
    // code in the firmware, which is +/- 16G.
    // The data appears to be stored in little-endian format.
    // NOTE: The BMI160 used by the vGlass reports the negative of
    // the acceleration vector, so we need to invert it after decoding it.
    static const double MSS_SCALE = (1.0 / 32767) * 16 * 9.807;
    bufptr = &buffer[12];
    channel[0] = -vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * MSS_SCALE;
    channel[1] = -vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * MSS_SCALE;
    channel[2] = -vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * MSS_SCALE;

    // Decode the rotational velocity values into channels 3=X, 4=Y, 5=Z.
    // Converts from [-1..1] to radians/second using the typical
    // sensitivity from the data sheet and the default range (the range is not
    // set in the initialization code in the firmware), so assuming that the
    // register setting is 0 (+/- 2000 degrees/second).
    // The data appears to be stored in little-endian format.
    static const double RS_SCALE = (1.0 / 32767) * 2000 * (VRPN_PI/180);
    bufptr = &buffer[6];
    channel[3] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * RS_SCALE;
    channel[4] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * RS_SCALE;
    channel[5] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * RS_SCALE;

    // Decode the magnetometer values into channels 6=X, 7=Y, 8=Z.
    // Converts from [-1..1] to Tesla and then from Tesla to Gauss
    // using the typical sensitivity from the AK09916 data sheet and the
    // specified range of values (not quite the entire 16-bit range).
    // The data appears to be stored in little-endian format.
    // NOTE: The data does not appear to be consistent, so these readings
    // are note very helpful as of 7/11/2019.  Until we can get our hands on
    // a unit and debug this, we'll leave these readings out.
    /*
    static const double MAG_SCALE = (4912.0 / 32752);
    bufptr = &buffer[18];
    channel[6] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * MAG_SCALE;
    channel[7] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * MAG_SCALE;
    channel[8] = vrpn_unbuffer_from_little_endian<vrpn_int16>(bufptr) * MAG_SCALE;
    */
}

// End of VRPN_USE_HID
#endif
