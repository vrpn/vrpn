/*			vrpn_log_devices.C

    This is a VRPN client program that will connect to one VRPN
    server and log the devices whose names are requested.
*/

#include <stdio.h>  // for printf, fprintf, NULL, etc
#include <stdlib.h> // for exit, atoi
#ifndef _WIN32_WCE
#include <signal.h> // for signal, SIGINT
#endif
#include <string.h>              // for strcmp
#include <vrpn_FileConnection.h> // For preload and accumulate settings
#include <vrpn_Shared.h>         // for vrpn_SleepMsecs
#include <vrpn_Tracker.h>        // So we can open Tracker devices
#include <vector>                // for vector

#include "vrpn_BaseClass.h" // for vrpn_System_TextPrinter, etc
#include "vrpn_Configure.h" // for VRPN_CALLBACK
#include "vrpn_Types.h"     // for vrpn_float64, vrpn_int32

using namespace std;

int done = 0;                // Signals that the program should exit

// WARNING: On Windows systems, this handler is called in a separate
// thread from the main program (this differs from Unix).  To avoid all
// sorts of chaos as the main program continues to handle packets, we
// set a done flag here and let the main program shut down in its own
// thread by calling shutdown() to do all of the stuff we used to do in
// this handler.

void handle_cntl_c(int) { done = 1; }

void Usage(const char *arg0)
{
    fprintf(
        stderr,
          "Usage: %s logfile server device1 [device2] [device3] [device4] [...]\n"
          "  logfile: Name of the file to save data into (eg: logfile.vrpn)\n"
          "  server: Name of the server to connect to (eg: localhost, ioglab)\n"
          "  deviceX: VRPN name of a device to log (eg: Tracker0, Analog0, Button0)\n"
          , arg0);

    exit(0);
}

int main(int argc, char *argv[])
{
    // If we happen to open a file, neither preload nor accumulate the
    // messages in memory, to avoid crashing for huge files.
    vrpn_FILE_CONNECTIONS_SHOULD_PRELOAD = false;
    vrpn_FILE_CONNECTIONS_SHOULD_ACCUMULATE = false;

    // Tracking devices we've opened
    vrpn_Tracker_Remote** trackers;
    size_t numTrackers = 0;
    trackers = new vrpn_Tracker_Remote * [argc];
    for (int i = 0; i < argc; i++) {
      trackers[i] = NULL;
    }

    if (argc < 4) {
      Usage(argv[0]);
    }

    // Find the name of the server we're to connect to and the name of the
    // first object.  We'll use that to construct the server connection and
    // ask it to log to the specified output file.
    const char *logFileName = argv[1];
    const char *serverName = argv[2];
    const char* firstObjectName = argv[3];
    char objectName[512];
    sprintf(objectName, "%s@%s", firstObjectName, serverName);
    vrpn_Connection* client_connection = vrpn_get_connection_by_name(objectName, logFileName);
    if (client_connection == NULL) {
      fprintf(stderr, "Could not open connection for logging");
      return(100);
    }

    // Construct all of the objects by name, connecting them to the connection
    // we just created.  It doesn't matter what kind of object we construct, they
    // will all cause ping/pong messages to happen, so we pick trackers.
    for (int i = 3; i < argc; i++) {
      sprintf(objectName, "%s@%s", argv[i], serverName);
      trackers[numTrackers++] = new vrpn_Tracker_Remote(objectName, client_connection);
    }

#ifndef _WIN32_WCE
    // signal handler so logfiles get closed right
    signal(SIGINT, handle_cntl_c);
#endif

    /*
     * main interactive loop
     */
    printf("Press ^C to exit.\n");
    while (!done) {
        unsigned i;

        // Let all the devices do their things
        for (i = 0; i < numTrackers; i++) {
            trackers[i]->mainloop();
        }

        // Sleep for 1ms so we don't eat the CPU
        vrpn_SleepMsecs(1);
    }

    // Delete all devices.
    {
        unsigned i;
        for (i = 0; i < numTrackers; i++) {
            delete trackers[i];
        }
        client_connection->removeReference();
    }

    return 0;
} /* main */
