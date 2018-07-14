#include "vrpn_Analog.h"
#include <stdio.h>
#include <string.h>
// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h
// and netinet/in.h and ...
#include "vrpn_Shared.h"

#ifndef VRPN_CLIENT_ONLY
#include "vrpn_Serial.h"
#endif

//#define VERBOSE

vrpn_Analog::vrpn_Analog(const char *name, vrpn_Connection *c)
    : vrpn_BaseClass(name, c)
    , num_channel(0)
{
    // Call the base class' init routine
    vrpn_BaseClass::init();

    // Set the time to 0 just to have something there.
    timestamp.tv_usec = timestamp.tv_sec = 0;
    // Initialize the values in the channels,
    // gets rid of uninitialized memory read error in Purify
    // and makes sure any initial value change gets reported.
    for (vrpn_int32 i = 0; i < vrpn_CHANNEL_MAX; i++) {
        channel[i] = last[i] = 0;
    }
}

int vrpn_Analog::register_types(void)
{
    channel_m_id = d_connection->register_message_type("vrpn_Analog Channel");
    if (channel_m_id == -1) {
        return -1;
    }
    else {
        return 0;
    }
}

void vrpn_Analog::print(void)
{
    printf("Analog Report: ");
    for (vrpn_int32 i = 0; i < num_channel; i++) {
        //    printf("Channel[%d]= %f\t", i, channel[i]);
        printf("%f\t", channel[i]);
    }
    printf("\n");
}

vrpn_int32 vrpn_Analog::getNumChannels(void) const { return num_channel; }

vrpn_int32 vrpn_Analog::encode_to(char *buf)
{
    // Message includes: vrpn_float64 AnalogNum, vrpn_float64 state
    // Byte order of each needs to be reversed to match network standard

    vrpn_float64 double_chan = num_channel;
    int buflen = (vrpn_CHANNEL_MAX + 1) * sizeof(vrpn_float64);

    vrpn_buffer(&buf, &buflen, double_chan);
    for (int i = 0; i < num_channel; i++) {
        vrpn_buffer(&buf, &buflen, channel[i]);
        last[i] = channel[i];
    }

    return (num_channel + 1) * sizeof(vrpn_float64);
}

void vrpn_Analog::report_changes(vrpn_uint32 class_of_service,
                                 const struct timeval time)
{
    vrpn_int32 i;
    vrpn_int32 change = 0;

    if (d_connection) {
        for (i = 0; i < num_channel; i++) {
            if (channel[i] != last[i]) change = 1;
            last[i] = channel[i];
        }
        if (!change) {
#ifdef VERBOSE
            fprintf(stderr, "No change.\n");
#endif
            return;
        }
    }

    // there is indeed some change, send it;
    vrpn_Analog::report(class_of_service, time);
}

void vrpn_Analog::report(vrpn_uint32 class_of_service,
                         const struct timeval time)
{
    // msgbuf must be float64-aligned!
    vrpn_float64 fbuf[vrpn_CHANNEL_MAX + 1];
    char *msgbuf = (char *)fbuf;

    vrpn_int32 len;

    // Replace the time value with the current time if the user passed in the
    // constant time referring to "now".
    if ((time.tv_sec == vrpn_ANALOG_NOW.tv_sec) &&
        (time.tv_usec == vrpn_ANALOG_NOW.tv_usec)) {
        vrpn_gettimeofday(&timestamp, NULL);
    }
    else {
        timestamp = time;
    }
    len = vrpn_Analog::encode_to(msgbuf);
#ifdef VERBOSE
    print();
#endif
    if (d_connection &&
        d_connection->pack_message(len, timestamp, channel_m_id, d_sender_id,
                                   msgbuf, class_of_service)) {
        fprintf(stderr, "vrpn_Analog: cannot write message: tossing\n");
    }
}

#ifndef VRPN_CLIENT_ONLY
vrpn_Serial_Analog::vrpn_Serial_Analog(const char *name, vrpn_Connection *c,
                                       const char *port, int baud, int bits,
                                       vrpn_SER_PARITY parity, bool rts_flow)
    : vrpn_Analog(name, c)
    , serial_fd(-1)
    , baudrate(0)
    , bufcounter(0)
{
    // Initialize
    portname[0] = '\0';
    buffer[0] = '\0';
    // Find out the port name and baud rate;
    if (port == NULL) {
        fprintf(stderr, "vrpn_Serial_Analog: NULL port name\n");
        status = vrpn_ANALOG_FAIL;
        return;
    }
    else {
        vrpn_strcpy(portname, port);
    }
    baudrate = baud;

    // Open the serial port we're going to use
    if ((serial_fd = vrpn_open_commport(portname, baudrate, bits, parity,
                                        rts_flow)) == -1) {
        fprintf(stderr, "vrpn_Serial_Analog: Cannot Open serial port\n");
        status = vrpn_ANALOG_FAIL;
    }

    // Reset the analog and find out what time it is
    status = vrpn_ANALOG_RESETTING;
    vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Serial_Analog::~vrpn_Serial_Analog()
{
    // Close com port when destroyed.
    if (serial_fd != -1) {
        vrpn_close_commport(serial_fd);
    }
}

#endif // VRPN_CLIENT_ONLY

vrpn_Analog_Server::vrpn_Analog_Server(const char *name, vrpn_Connection *c,
                                       vrpn_int32 numChannels)
    : vrpn_Analog(name, c)
{
    this->setNumChannels(numChannels);

    // Check if we got a connection.
    if (d_connection == NULL) {
        fprintf(stderr, "vrpn_Analog_Server: Can't get connection!\n");
    }
}

// virtual
void vrpn_Analog_Server::report_changes(vrpn_uint32 class_of_service,
                                        const struct timeval time)
{
    vrpn_Analog::report_changes(class_of_service, time);
}

// virtual
void vrpn_Analog_Server::report(vrpn_uint32 class_of_service,
                                const struct timeval time)
{
    vrpn_Analog::report(class_of_service, time);
}

vrpn_int32 vrpn_Analog_Server::setNumChannels(vrpn_int32 sizeRequested)
{
    if (sizeRequested < 0) sizeRequested = 0;
    if (sizeRequested > vrpn_CHANNEL_MAX) sizeRequested = vrpn_CHANNEL_MAX;
    num_channel = sizeRequested;
    return num_channel;
}

vrpn_Clipping_Analog_Server::vrpn_Clipping_Analog_Server(const char *name,
                                                         vrpn_Connection *c,
                                                         vrpn_int32 numChannels)
    : vrpn_Analog_Server(name, c, numChannels)
{
    int i;

    for (i = 0; i < vrpn_CHANNEL_MAX; i++) {
        clipvals[i].minimum_val = -1.0;
        clipvals[i].lower_zero = 0.0;
        clipvals[i].upper_zero = 0.0;
        clipvals[i].maximum_val = 1.0;
    }
}

/// Set the clipping values for the specified channel.
/// min maps to -1, values between lowzero and highzero map to 0,
/// max maps to 1.  Values less than min map to -1, values larger
/// than max map to 1. Default for each channel is -1,0,0,1
/// It is possible to compress the range to [0..1] by setting the
/// minimum equal to the lowzero.
/// Returns 0 on success, -1 on failure.
int vrpn_Clipping_Analog_Server::setClipValues(int chan, double min,
                                               double lowzero, double highzero,
                                               double max)
{
    if ((chan < 0) || (chan >= vrpn_CHANNEL_MAX)) {
        fprintf(
            stderr,
            "vrpn_Clipping_Analog_Server::setClipValues: Bad channel (%d)\n",
            chan);
        return -1;
    }
    if ((lowzero < min) || (highzero < lowzero) || (max < highzero)) {
        fprintf(stderr, "vrpn_Clipping_Analog_Server::setClipValues: Out of "
                        "order mapping\n");
        return -1;
    }

    clipvals[chan].minimum_val = min;
    clipvals[chan].lower_zero = lowzero;
    clipvals[chan].upper_zero = highzero;
    clipvals[chan].maximum_val = max;

    return 0;
}

/// This method should be used to set the value of a channel.
/// It will be scaled and clipped as described in setClipValues.
/// It returns 0 on success and -1 on failure.
int vrpn_Clipping_Analog_Server::setChannelValue(int chan, double value)
{
    if ((chan < 0) || (chan >= vrpn_CHANNEL_MAX)) {
        fprintf(
            stderr,
            "vrpn_Clipping_Analog_Server::setChannelValue: Bad channel (%d)\n",
            chan);
        return -1;
    }

    // Figure out which clipping values to use
    clipvals_struct clips = clipvals[chan];

    // See if it should be clipped to zero, high, or low.  Zero is checked
    // first so that the range can be compress to [0..1] or [-1..0] by setting
    // a zero value equal to a clip value.
    if ((value >= clips.lower_zero) && (value <= clips.upper_zero)) {
        channel[chan] = 0.0;
    }
    else if (value <= clips.minimum_val) {
        channel[chan] = -1.0;
    }
    else if (value >= clips.maximum_val) {
        channel[chan] = 1.0;

        // If we are below the minzero, then we should map to the -1..0 range.
        // Otherwise, map to the 0..1 range.  Note that if we have reached this
        // point, the range we are mapped to has not been collapsed, so we won't
        // get divide-by-zero problems and such.
    }
    else if (value > clips.lower_zero) {
        channel[chan] =
            (value - clips.upper_zero) / (clips.maximum_val - clips.upper_zero);
    }
    else {
        // Larger negative number (minus smaller negative [net positive]) is
        // negative
        // Negative number divided by positive number is negative.
        channel[chan] =
            (value - clips.lower_zero) / (clips.lower_zero - clips.minimum_val);
    }

    return 0;
}

// ************* CLIENT ROUTINES ****************************

vrpn_Analog_Remote::vrpn_Analog_Remote(const char *name, vrpn_Connection *c)
    : vrpn_Analog(name, c)
{
    vrpn_int32 i;

    // Register a handler for the change callback from this device,
    // if we got a connection.
    if (d_connection != NULL) {
        if (register_autodeleted_handler(channel_m_id, handle_change_message,
                                         this, d_sender_id)) {
            fprintf(stderr, "vrpn_Analog_Remote: can't register handler\n");
            d_connection = NULL;
        }
    }
    else {
        fprintf(stderr, "vrpn_Analog_Remote: Can't get connection!\n");
    }

    // At the start, as far as the client knows, the device could have
    // max channels -- the number of channels is specified in each
    // message.
    num_channel = vrpn_CHANNEL_MAX;
    for (i = 0; i < vrpn_CHANNEL_MAX; i++) {
        channel[i] = last[i] = 0;
    }
    vrpn_gettimeofday(&timestamp, NULL);
}

void vrpn_Analog_Remote::mainloop()
{
    if (d_connection) {
        d_connection->mainloop();
        client_mainloop();
    }
}

int vrpn_Analog_Remote::handle_change_message(void *userdata,
                                              vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_float64 numchannelD; //< Number of channels passed in a double (yuck!)
    vrpn_Analog_Remote *me = (vrpn_Analog_Remote *)userdata;
    vrpn_ANALOGCB cp;

    cp.msg_time = p.msg_time;
    vrpn_unbuffer(&bufptr, &numchannelD);
    cp.num_channel = (long)numchannelD;
    me->num_channel = cp.num_channel;
    for (vrpn_int32 i = 0; i < cp.num_channel; i++) {
        vrpn_unbuffer(&bufptr, &cp.channel[i]);
    }

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->d_callback_list.call_handlers(cp);

    return 0;
}
