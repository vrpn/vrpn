/* This program will ping a VRPN server and time how long it takes the server
   that is being pinged to respond.  It makes use of the VRPN ping/pong
   mechanism by which objects detect if they have connected to servers.
   It gives an indication of the round-trip time from a message at the user
   level from a client through the user level at a server.
*/

#include <signal.h>          // for signal, SIGINT
#include <stdio.h>           // for NULL, printf
#include <stdlib.h>          // for exit
#include <vrpn_Connection.h> // for vrpn_Connection, etc
#include <vrpn_Text.h>       // for vrpn_Text_Receiver

#include "vrpn_Configure.h" // for VRPN_CALLBACK
#include "vrpn_Shared.h"    // for timeval, vrpn_gettimeofday, etc
#include "vrpn_Types.h"     // for vrpn_int32

vrpn_Text_Receiver *r;
vrpn_Connection *c;
vrpn_int32 ping_message_id;
vrpn_int32 pong_message_id;
vrpn_int32 sender;

struct timeval now;
struct timeval last_ping;

void handle_cntl_c(int) { exit(0); }

int VRPN_CALLBACK my_pong_handler(void *, vrpn_HANDLERPARAM)
{
    static int count = 0;
    static double min = 10000, max = 0, sum = 0;

    // See how long it has been between the ping request and
    // the pong response.
    struct timeval diff;
    vrpn_gettimeofday(&now, NULL);
    double msecs;
    diff = vrpn_TimevalDiff(now, last_ping);
    msecs = vrpn_TimevalMsecs(vrpn_TimevalNormalize(diff));

    // Keep statistics on the length (min, max, average)
    if (msecs < min) {
        min = msecs;
    };
    if (msecs > max) {
        max = msecs;
    };
    sum += msecs;

    // Print and reset the statistics every once in a while
    if (++count == 500) {
        printf("Min = %4.2g, Max = %4.2g, Mean = %4.2g\n", min, max,
               sum / count);
        count = 0;
        min = 10000;
        max = sum = 0.0;
    }

    // Send a new ping request, and record when we sent it.
    // REMEMBER not to call mainloop() within the handler.
    vrpn_gettimeofday(&last_ping, NULL);
    c->pack_message(0, last_ping, ping_message_id, sender, NULL,
                    vrpn_CONNECTION_RELIABLE);
    c->send_pending_reports();

    return 0;
}

int main(int , char *argv[])
{
    // Declare a new text receiver (all objects are text senders)
    // and find out what connection it is using.
    r = new vrpn_Text_Receiver(argv[1]);
    c = r->connectionPtr();

    // Declare the same sender and message types that the BaseClass
    // will use for doing the ping/pong, so we can use the same
    // mechanism.  Register a handler for the pong message, so we
    // can deal with them.
    ping_message_id = c->register_message_type("Server, are you there?");
    pong_message_id = c->register_message_type("Server is here!");
    sender = c->register_sender(vrpn_copy_service_name(argv[1]));
    c->register_handler(pong_message_id, my_pong_handler, NULL, sender);

    // Let the user kill the program "nicely."
    signal(SIGINT, handle_cntl_c);

    // Wait a few seconds (spinning while we do) in order to allow the
    // real pong message to clear from the system.
    struct timeval then, diff;
    vrpn_gettimeofday(&then, NULL);
    do {
        vrpn_gettimeofday(&now, NULL);
        r->mainloop();
        diff = vrpn_TimevalDiff(now, then);
    } while (vrpn_TimevalMsecs(vrpn_TimevalNormalize(diff)) < 2000);

    // Send a new ping request to the server, and start counting how
    // long it takes to respond.
    vrpn_gettimeofday(&last_ping, NULL);
    c->pack_message(0, last_ping, ping_message_id, sender, NULL,
                    vrpn_CONNECTION_RELIABLE);

    // Loop forever.
    while (1) {
        r->mainloop();
    }
}
