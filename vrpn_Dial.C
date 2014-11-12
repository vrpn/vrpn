#include <stdio.h> // for fprintf, stderr, NULL

#include "vrpn_Connection.h" // for vrpn_Connection, etc
#include "vrpn_Dial.h"

//#define VERBOSE

vrpn_Dial::vrpn_Dial(const char *name, vrpn_Connection *c)
    : vrpn_BaseClass(name, c)
{
    vrpn_BaseClass::init();

    num_dials = 0;
    // Set the time to 0 just to have something there.
    timestamp.tv_usec = timestamp.tv_sec = 0;
}

int vrpn_Dial::register_types(void)
{
    if (d_connection == NULL) {
        return 0;
    }
    change_m_id = d_connection->register_message_type("vrpn_Dial update");
    if (change_m_id == -1) {
        fprintf(stderr, "vrpn_Dial: Can't register type IDs\n");
        d_connection = NULL;
    }
    return 0;
}

vrpn_int32 vrpn_Dial::encode_to(char *buf, vrpn_int32 buflen, vrpn_int32 dial,
                                vrpn_float64 delta)
{
    vrpn_int32 buflensofar = buflen;

    // Message includes: vrpn_int32 dialNum, vrpn_float64 delta
    // We pack them with the delta first, so that everything is aligned
    // on the proper boundary.

    if (vrpn_buffer(&buf, &buflensofar, delta)) {
        fprintf(stderr, "vrpn_Dial::encode_to: Can't buffer delta\n");
        return -1;
    }
    if (vrpn_buffer(&buf, &buflensofar, dial)) {
        fprintf(stderr, "vrpn_Dial::encode_to: Can't buffer dial\n");
        return -1;
    }
    return sizeof(vrpn_float64) + sizeof(vrpn_int32);
}

// This will report any changes that have appeared in the dial values,
// and then clear the dials back to 0. It only sends dials that have
// changed (nonzero values)
void vrpn_Dial::report_changes(void)
{

    char msgbuf[1000];
    vrpn_int32 i;
    vrpn_int32 len;

    if (d_connection) {
        for (i = 0; i < num_dials; i++) {
            if (dials[i] != 0) {
                len = encode_to(msgbuf, sizeof(msgbuf), i, dials[i]);
                if (d_connection->pack_message(len, timestamp, change_m_id,
                                               d_sender_id, msgbuf,
                                               vrpn_CONNECTION_RELIABLE)) {
                    fprintf(stderr,
                            "vrpn_Dial: can't write message: tossing\n");
                }
                dials[i] = 0; // We've reported it the change.
            }
        }
    }
}

// This will report the dials whether they have changed or not; this
// will result in some 0 dial values being sent. Normally, code will
// use the report_changes method to avoid flooding the system with
// un-needed messages.
// It then clears the dials back to 0.
void vrpn_Dial::report(void)
{

    char msgbuf[1000];
    vrpn_int32 i;
    vrpn_int32 len;

    if (d_connection) {
        for (i = 0; i < num_dials; i++) {
            len = encode_to(msgbuf, sizeof(msgbuf), i, dials[i]);
            if (d_connection->pack_message(len, timestamp, change_m_id,
                                           d_sender_id, msgbuf,
                                           vrpn_CONNECTION_RELIABLE)) {
                fprintf(stderr, "vrpn_Dial: can't write message: tossing\n");
            }
            dials[i] = 0; // We've reported it the change.
        }
    }
}

// ************* EXAMPLE SERVER ROUTINES ****************************

vrpn_Dial_Example_Server::vrpn_Dial_Example_Server(const char *name,
                                                   vrpn_Connection *c,
                                                   vrpn_int32 numdials,
                                                   vrpn_float64 spin_rate,
                                                   vrpn_float64 update_rate)
    : vrpn_Dial(name, c)
    , // Construct the base class, which registers message/sender
    _spin_rate(spin_rate)
    ,                         // Set the rate at which the dials will spin
    _update_rate(update_rate) // Set the rate at which to generate reports
{
    // Make sure we asked for a legal number of dials
    if (num_dials > vrpn_DIAL_MAX) {
        fprintf(stderr, "vrpn_Dial_Example_Server: Only using %d dials\n",
                vrpn_DIAL_MAX);
        num_dials = vrpn_DIAL_MAX;
    }
    else {
        num_dials = numdials;
    }

    // IN A REAL SERVER, open the device that will service the dials here
}

void vrpn_Dial_Example_Server::mainloop()
{
    struct timeval current_time;
    int i;

    // Call the generic server mainloop, since we are a server
    server_mainloop();

    // See if its time to generate a new report
    // IN A REAL SERVER, this check would not be done; although the
    // time of the report would be updated to the current time so
    // that the correct timestamp would be issued on the report.
    vrpn_gettimeofday(&current_time, NULL);
    if (vrpn_TimevalDuration(current_time, timestamp) >=
        1000000.0 / _update_rate) {

        // Update the time
        timestamp.tv_sec = current_time.tv_sec;
        timestamp.tv_usec = current_time.tv_usec;

        // Update the values for the dials, to say that each one has
        // moved the appropriate rotation (spin rate is revolutions per
        // second, update rate is report/second, the quotient is the number
        // of revolutions since the last report). When the changes are
        // reported, these values are set back to zero.

        // THIS CODE WILL BE REPLACED by the user code that tells how
        // many revolutions each dial has changed since the last report.
        for (i = 0; i < num_dials; i++) {
            dials[i] = _spin_rate / _update_rate;
        }

        // Send reports. Stays the same in a real server.
        report_changes();
    }
}

// ************* CLIENT ROUTINES ****************************

vrpn_Dial_Remote::vrpn_Dial_Remote(const char *name, vrpn_Connection *c)
    : vrpn_Dial(name, c)
{
    vrpn_int32 i;

    // Register a handler for the change callback from this device,
    // if we got a connection.
    if (d_connection) {
        if (register_autodeleted_handler(change_m_id, handle_change_message,
                                         this, d_sender_id)) {
            fprintf(stderr, "vrpn_Dial_Remote: can't register handler\n");
            d_connection = NULL;
        }
    }
    else {
        fprintf(stderr, "vrpn_Dial_Remote: Can't get connection!\n");
    }

    // At the start, as far as the client knows, the device could have
    // max channels -- the number of channels is specified in each
    // message.
    num_dials = vrpn_DIAL_MAX;
    for (i = 0; i < vrpn_DIAL_MAX; i++) {
        dials[i] = 0.0;
    }
    vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Dial_Remote::~vrpn_Dial_Remote() {}

void vrpn_Dial_Remote::mainloop()
{
    client_mainloop();
    if (d_connection) {
        d_connection->mainloop();
    }
}

int vrpn_Dial_Remote::handle_change_message(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Dial_Remote *me = (vrpn_Dial_Remote *)userdata;
    vrpn_DIALCB cp;
    const char *bufptr = p.buffer;

    cp.msg_time = p.msg_time;
    vrpn_unbuffer(&bufptr, &cp.change);
    vrpn_unbuffer(&bufptr, &cp.dial);

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->d_callback_list.call_handlers(cp);

    return 0;
}
