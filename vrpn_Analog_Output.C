#include "vrpn_Analog_Output.h"
#include <stdio.h>

vrpn_Analog_Output::vrpn_Analog_Output(const char* name, vrpn_Connection* c)
    : vrpn_BaseClass(name, c)
    , o_num_channel(0)
{
    int i;

    // Call the base class' init routine
    vrpn_BaseClass::init();

    // Set the time to 0 just to have something there.
    o_timestamp.tv_usec = o_timestamp.tv_sec = 0;
    // Initialize the values in the channels,
    // gets rid of uninitialized memory read error in Purify
    // and makes sure any initial value change gets reported.
    for (i = 0; i < vrpn_CHANNEL_MAX; i++) {
        o_channel[i] = 0;
    }
}

int vrpn_Analog_Output::register_types(void)
{
    request_m_id = d_connection->register_message_type(
        "vrpn_Analog_Output Change_request");
    request_channels_m_id = d_connection->register_message_type(
        "vrpn_Analog_Output Change_Channels_request");
    report_num_channels_m_id = d_connection->register_message_type(
        "vrpn_Analog_Output Num_Channels_report");
    got_connection_m_id =
        d_connection->register_message_type(vrpn_got_connection);
    if ((request_m_id == -1) || (request_channels_m_id == -1) ||
        (report_num_channels_m_id == -1) || (got_connection_m_id == -1)) {
        return -1;
    }
    else {
        return 0;
    }
}

void vrpn_Analog_Output::o_print(void)
{
    printf("Analog_Output Report: ");
    for (vrpn_int32 i = 0; i < o_num_channel; i++) {
        // printf("Channel[%d]= %f\t", i, o_channel[i]);
        printf("%f\t", o_channel[i]);
    }
    printf("\n");
}

vrpn_Analog_Output_Server::vrpn_Analog_Output_Server(const char* name,
                                                     vrpn_Connection* c,
                                                     vrpn_int32 numChannels)
    : vrpn_Analog_Output(name, c)
{
    this->setNumChannels(numChannels);

    // Check if we have a connection
    if (d_connection == NULL) {
        fprintf(stderr, "vrpn_Analog_Output: Can't get connection!\n");
    }

    // Register a handler for the request channel change message
    if (register_autodeleted_handler(request_m_id, handle_request_message, this,
                                     d_sender_id)) {
        fprintf(stderr, "vrpn_Analog_Output_Server: can't register change "
                        "channel request handler\n");
        d_connection = NULL;
    }

    // Register a handler for the request channels change message
    if (register_autodeleted_handler(request_channels_m_id,
                                     handle_request_channels_message, this,
                                     d_sender_id)) {
        fprintf(stderr, "vrpn_Analog_Output_Server: can't register change "
                        "channels request handler\n");
        d_connection = NULL;
    }

    // Register a handler for vrpn_got_connection, so we can tell the
    // client how many channels are active
    if (register_autodeleted_handler(got_connection_m_id, handle_got_connection,
                                     this)) {
        fprintf(stderr, "vrpn_Analog_Output_Server: can't register new "
                        "connection handler\n");
        d_connection = NULL;
    }
}

// virtual
vrpn_Analog_Output_Server::~vrpn_Analog_Output_Server(void) {}

vrpn_int32 vrpn_Analog_Output_Server::setNumChannels(vrpn_int32 sizeRequested)
{
    if (sizeRequested < 0) sizeRequested = 0;
    if (sizeRequested > vrpn_CHANNEL_MAX) sizeRequested = vrpn_CHANNEL_MAX;

    o_num_channel = sizeRequested;

    return o_num_channel;
}

/* static */
int vrpn_Analog_Output_Server::handle_request_message(void* userdata,
                                                      vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 chan_num;
    vrpn_int32 pad;
    vrpn_float64 value;
    vrpn_Analog_Output_Server* me = (vrpn_Analog_Output_Server*)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the appropriate value, if the channel number is in the
    // range of the ones we have.
    if ((chan_num < 0) || (chan_num >= me->o_num_channel)) {
        fprintf(stderr, "vrpn_Analog_Output_Server::handle_request_message(): "
                        "Index out of bounds\n");
        char msg[1024];
        sprintf(msg, "Error:  (handle_request_message):  channel %d is not "
                     "active.  Squelching.",
                chan_num);
        me->send_text_message(msg, p.msg_time, vrpn_TEXT_ERROR);
        return 0;
    }
    me->o_channel[chan_num] = value;

    return 0;
}

/* static */
int vrpn_Analog_Output_Server::handle_request_channels_message(
    void* userdata, vrpn_HANDLERPARAM p)
{
    int i;
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_Analog_Output_Server* me = (vrpn_Analog_Output_Server*)userdata;

    // Read the values from the buffer
    vrpn_unbuffer(&bufptr, &num);
    vrpn_unbuffer(&bufptr, &pad);
    if (num > me->o_num_channel) {
        char msg[1024];
        sprintf(msg, "Error:  (handle_request_channels_message):  channels "
                     "above %d not active; "
                     "bad request up to channel %d.  Squelching.",
                me->o_num_channel, num);
        me->send_text_message(msg, p.msg_time, vrpn_TEXT_ERROR);
        num = me->o_num_channel;
    }
    if (num < 0) {
        char msg[1024];
        sprintf(msg, "Error:  (handle_request_channels_message):  invalid "
                     "channel %d.  Squelching.",
                num);
        me->send_text_message(msg, p.msg_time, vrpn_TEXT_ERROR);
        return 0;
    }
    for (i = 0; i < num; i++) {
        vrpn_unbuffer(&bufptr, &(me->o_channel[i]));
    }

    return 0;
}

/* static */
int vrpn_Analog_Output_Server::handle_got_connection(void* userdata,
                                                     vrpn_HANDLERPARAM)
{
    vrpn_Analog_Output_Server* me = (vrpn_Analog_Output_Server*)userdata;
    if (me->report_num_channels() == false) {
        fprintf(stderr, "Error:  failed sending active channels to client.\n");
    }
    return 0;
}

bool vrpn_Analog_Output_Server::report_num_channels(
    vrpn_uint32 class_of_service)
{
    char msgbuf[sizeof(vrpn_int32)];
    vrpn_int32 len = sizeof(vrpn_int32);
    ;

    encode_num_channels_to(msgbuf, this->o_num_channel);
    vrpn_gettimeofday(&o_timestamp, NULL);
    if (d_connection &&
        d_connection->pack_message(len, o_timestamp, report_num_channels_m_id,
                                   d_sender_id, msgbuf, class_of_service)) {
        fprintf(stderr, "vrpn_Analog_Output_Server (report_num_channels): "
                        "cannot write message: tossing\n");
        return false;
    }
    return true;
}

vrpn_int32 vrpn_Analog_Output_Server::encode_num_channels_to(char* buf,
                                                             vrpn_int32 num)
{
    // Message includes: int32 number of active channels
    int buflen = sizeof(vrpn_int32);

    vrpn_buffer(&buf, &buflen, num);
    return sizeof(vrpn_int32);
}

vrpn_Analog_Output_Callback_Server::vrpn_Analog_Output_Callback_Server(
    const char* name, vrpn_Connection* c, vrpn_int32 numChannels)
    : vrpn_Analog_Output_Server(name, c, numChannels)
{
    // Register a handler for the request channel change message.  This will go
    // in the list AFTER the one for the base class, so the values will already
    // have been filled in.  So we just need to call the user-level callbacks
    // and pass them a pointer to the data values.
    if (register_autodeleted_handler(request_m_id, handle_change_message, this,
                                     d_sender_id)) {
        fprintf(stderr, "vrpn_Analog_Output_Callback_Server: can't register "
                        "change channel request handler\n");
        d_connection = NULL;
    }

    // Register a handler for the request channels change message.  This will go
    // in the list AFTER the one for the base class, so the values will already
    // have been filled in.  So we just need to call the user-level callbacks
    // and pass them a pointer to the data values.
    if (register_autodeleted_handler(
            request_channels_m_id, handle_change_message, this, d_sender_id)) {
        fprintf(stderr, "vrpn_Analog_Output_Callback_Server: can't register "
                        "change channels request handler\n");
        d_connection = NULL;
    }
}

// virtual
vrpn_Analog_Output_Callback_Server::~vrpn_Analog_Output_Callback_Server(void) {}

/* static */
// This is inserted into the list AFTER the
int vrpn_Analog_Output_Callback_Server::handle_change_message(
    void* userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Analog_Output_Callback_Server* me =
        (vrpn_Analog_Output_Callback_Server*)userdata;
    vrpn_ANALOGOUTPUTCB callback_data;

    callback_data.msg_time = p.msg_time;
    callback_data.num_channel = me->getNumChannels();
    callback_data.channel = me->o_channels();
    me->d_callback_list.call_handlers(callback_data);

    return 0;
}

vrpn_Analog_Output_Remote::vrpn_Analog_Output_Remote(const char* name,
                                                     vrpn_Connection* c)
    : vrpn_Analog_Output(name, c)
{
    vrpn_int32 i;

    o_num_channel = vrpn_CHANNEL_MAX;
    for (i = 0; i < vrpn_CHANNEL_MAX; i++) {
        o_channel[i] = 0;
    }
    vrpn_gettimeofday(&o_timestamp, NULL);

    // Register a handler for the report number of active channels message
    if (register_autodeleted_handler(report_num_channels_m_id,
                                     handle_report_num_channels, this,
                                     d_sender_id)) {
        fprintf(stderr, "vrpn_Analog_Output_Remote: can't register active "
                        "channel report handler\n");
        d_connection = NULL;
    }
}

// virtual
vrpn_Analog_Output_Remote::~vrpn_Analog_Output_Remote(void) {}

void vrpn_Analog_Output_Remote::mainloop()
{
    if (d_connection) {
        d_connection->mainloop();
        client_mainloop();
    }
}

/* static */
int vrpn_Analog_Output_Remote::handle_report_num_channels(void* userdata,
                                                          vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_Analog_Output_Remote* me = (vrpn_Analog_Output_Remote*)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &num);

    if (num >= 0 && num <= vrpn_CHANNEL_MAX) {
        me->o_num_channel = num;
    }
    else {
        fprintf(
            stderr,
            "vrpn_Analog_Output_Remote::handle_report_num_channels_message:  "
            "Someone sent us a bogus number of channels:  %d.\n",
            num);
    }
    return 0;
}

bool vrpn_Analog_Output_Remote::request_change_channel_value(
    unsigned int chan, vrpn_float64 val, vrpn_uint32 class_of_service)
{
    // msgbuf must be float64-aligned!
    vrpn_float64 fbuf[2];
    char* msgbuf = (char*)fbuf;

    vrpn_int32 len;

    vrpn_gettimeofday(&o_timestamp, NULL);
    len = encode_change_to(msgbuf, chan, val);
    if (d_connection &&
        d_connection->pack_message(len, o_timestamp, request_m_id, d_sender_id,
                                   msgbuf, class_of_service)) {
        fprintf(stderr,
                "vrpn_Analog_Output_Remote: cannot write message: tossing\n");
        return false;
    }

    return true;
}

bool vrpn_Analog_Output_Remote::request_change_channels(
    int num, vrpn_float64* vals, vrpn_uint32 class_of_service)
{
    if (num < 0 || num > vrpn_CHANNEL_MAX) {
        fprintf(stderr, "vrpn_Analog_Output_Remote: cannot change channels: "
                        "number of channels out of range\n");
        return false;
    }

    vrpn_float64 fbuf[1 + vrpn_CHANNEL_MAX];
    char* msgbuf = (char*)fbuf;
    vrpn_int32 len;

    vrpn_gettimeofday(&o_timestamp, NULL);
    len = encode_change_channels_to(msgbuf, num, vals);
    if (d_connection &&
        d_connection->pack_message(len, o_timestamp, request_channels_m_id,
                                   d_sender_id, msgbuf, class_of_service)) {
        fprintf(stderr,
                "vrpn_Analog_Output_Remote: cannot write message: tossing\n");
        return false;
    }
    return true;
}

vrpn_int32 vrpn_Analog_Output_Remote::encode_change_to(char* buf,
                                                       vrpn_int32 chan,
                                                       vrpn_float64 val)
{
    // Message includes: int32 channel_number, int32 padding, float64
    // request_value
    // Byte order of each needs to be reversed to match network standard

    vrpn_int32 pad = 0;
    int buflen = 2 * sizeof(vrpn_int32) + sizeof(vrpn_float64);

    vrpn_buffer(&buf, &buflen, chan);
    vrpn_buffer(&buf, &buflen, pad);
    vrpn_buffer(&buf, &buflen, val);

    return 2 * sizeof(vrpn_int32) + sizeof(vrpn_float64);
}

vrpn_int32
vrpn_Analog_Output_Remote::encode_change_channels_to(char* buf, vrpn_int32 num,
                                                     vrpn_float64* vals)
{
    int i;
    vrpn_int32 pad = 0;
    int buflen = 2 * sizeof(vrpn_int32) + num * sizeof(vrpn_float64);

    vrpn_buffer(&buf, &buflen, num);
    vrpn_buffer(&buf, &buflen, pad);
    for (i = 0; i < num; i++) {
        vrpn_buffer(&buf, &buflen, vals[i]);
    }

    return 2 * sizeof(vrpn_int32) + num * sizeof(vrpn_float64);
}
