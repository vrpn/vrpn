#include <stdio.h>  // for fprintf, stderr, NULL, etc
#include <stdlib.h> // for atoi, exit
#include <string.h> // for strcmp
#include <string>
#include <sstream>

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_NORMAL, etc
#include "vrpn_Configure.h"             // for VRPN_CALLBACK, etc
#include "vrpn_Connection.h"            // for vrpn_Connection, etc
#include "vrpn_ForwarderController.h"   // for vrpn_Forwarder_Server
#include "vrpn_Generic_server_object.h" // for vrpn_Generic_Server_Object
#include "vrpn_Shared.h"                // for vrpn_SleepMsecs

void Usage(const char *s)
{
    fprintf(stderr, "Usage: %s [-f filename] [-warn] [-v] [port] [-q]\n", s);
    fprintf(stderr, "       [-millisleep n]\n");
    fprintf(stderr, "       [-NIC name] [-li filename] [-lo filename]\n");
    fprintf(stderr,
            "       -f: Full path to config file (default vrpn.cfg).\n");
    fprintf(stderr,
            "       -millisleep: Sleep n milliseconds each loop cycle\n");
    fprintf(stderr, "                    (if no option is specified, the "
                    "Windows architecture\n");
    fprintf(stderr, "                     will free the rest of its time slice "
                    "on each loop\n");
    fprintf(stderr, "                     but leave the processes available to "
                    "be run immediately;\n");
    fprintf(stderr, "                     a 1ms sleep is the default on all "
                    "other architectures).\n");
    fprintf(stderr, "                    -millisleep 0 is recommended when "
                    "using this server and\n");
    fprintf(stderr, "                     a client on the same uniprocessor "
                    "CPU Win32 PC.\n");
    fprintf(stderr, "                    -millisleep -1 will cause the server "
                    "process to use the\n");
    fprintf(stderr,
            "                     whole CPU on any uniprocessor machine.\n");
    fprintf(stderr,
            "       -warn: Only warn on errors (default is to bail).\n");
    fprintf(stderr, "       -v: Verbose.\n");
    fprintf(stderr, "       -q: Quit when last connection is dropped.\n");
    fprintf(stderr,
            "       -NIC: Use NIC with given IP address or DNS name.\n");
    fprintf(stderr, "       -li: Log incoming messages to given filename.\n");
    fprintf(stderr, "       -lo: Log outgoing messages to given filename.\n");
    fprintf(stderr,
            "       -flush: Flush logs to disk after every mainloop().\n");
    exit(0);
}

static volatile int done = 0; // Done and should exit?

vrpn_Connection *connection;
vrpn_Generic_Server_Object *generic_server = NULL;

static char *g_NICname = NULL;

static const char *g_inLogName = NULL;
static const char *g_outLogName = NULL;

// TCH October 1998
// Use Forwarder as remote-controlled multiple connections.
vrpn_Forwarder_Server *forwarderServer;

static bool verbose = false;

void shutDown(void)
{
    if (verbose) {
        fprintf(stderr, "Deleting forwarder server\n");
    }
    if (forwarderServer) {
        delete forwarderServer;
        forwarderServer = NULL;
    }
    if (verbose) {
        fprintf(stderr, "Deleting generic server object...");
    }
    if (generic_server) {
        delete generic_server;
        generic_server = NULL;
    }
    if (verbose) {
        fprintf(stderr, "Deleting connection\n");
    }
    if (connection) {
        connection->removeReference();
        connection = NULL;
    }
    if (verbose) {
        fprintf(stderr, "Deleted server and connection, Exiting.\n");
    }
    exit(0);
}

int VRPN_CALLBACK handle_dlc(void *, vrpn_HANDLERPARAM /*p*/)
{
    shutDown();
    return 0;
}

// install a signal handler to shut down the devices
// On Windows, the signal handler is run in a different thread from
// the main application.  We don't want to go destroying things in
// here while they are being used there, so we set a flag telling the
// main program it is time to exit.
#if defined(_WIN32) && !defined(__CYGWIN__)
/**
 * Handle exiting cleanly when we get ^C or other signals.
 */
BOOL WINAPI handleConsoleSignalsWin(DWORD signaltype)
{
    switch (signaltype) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        done = 1;
        return TRUE;
    // Don't exit, but return FALSE so default handler
    // gets called. The default handler, ExitProcess, will exit.
    default:
        return FALSE;
    }
}

#else
#include <signal.h> // for signal, SIGINT, SIGKILL, etc

void sighandler(int) { done = 1; }
#endif

int main(int argc, char *argv[])
{
    const char *config_file_name = "vrpn.cfg";
    bool bail_on_error = true;
    bool auto_quit = false;
    bool flush_continuously = false;
    int realparams = 0;
    int i;
    int port = vrpn_DEFAULT_LISTEN_PORT_NO;
#ifdef _WIN32
    // On Windows, the millisleep with 0 option frees up the CPU time slice for
    // other jobs
    // but does not sleep for a specific time.  On Unix, this returns
    // immediately and does
    // not do anything but waste cycles.
    int milli_sleep_time = 0; // How long to sleep each iteration (default: free
                              // the timeslice but be runnable again
                              // immediately)
#else
    int milli_sleep_time = 1; // How long to sleep each iteration (default: 1ms)
#endif
#ifdef _WIN32
    WSADATA wsaData;
    int status;
    if ((status = WSAStartup(MAKEWORD(1, 1), &wsaData)) != 0) {
        fprintf(stderr, "WSAStartup failed with %d\n", status);
        return (1);
    }

    // This handles all kinds of signals.
    SetConsoleCtrlHandler(handleConsoleSignalsWin, TRUE);

#else
#ifdef sgi
    sigset(SIGINT, sighandler);
    sigset(SIGKILL, sighandler);
    sigset(SIGTERM, sighandler);
    sigset(SIGPIPE, sighandler);
#else
    signal(SIGINT, sighandler);
    signal(SIGKILL, sighandler);
    signal(SIGTERM, sighandler);
    signal(SIGPIPE, sighandler);
#endif // not sgi
#endif // not WIN32

    // Parse the command line
    i = 1;
    while (i < argc) {
        if (!strcmp(argv[i], "-f")) { // Specify config-file name
            if (++i > argc) {
                Usage(argv[0]);
            }
            config_file_name = argv[i];
        }
        else if (!strcmp(argv[i],
                         "-millisleep")) { // How long to sleep each loop?
            if (++i > argc) {
                Usage(argv[0]);
            }
            milli_sleep_time = atoi(argv[i]);
        }
        else if (!strcmp(argv[i], "-warn")) { // Don't bail on errors
            bail_on_error = false;
        }
        else if (!strcmp(argv[i], "-v")) { // Verbose
            verbose = true;
            vrpn_System_TextPrinter.set_min_level_to_print(vrpn_TEXT_NORMAL);
        }
        else if (!strcmp(argv[i], "-q")) { // quit on dropped last con
            auto_quit = true;
        }
        else if (!strcmp(argv[i], "-NIC")) { // specify a network interface
            if (++i > argc) {
                Usage(argv[0]);
            }
            if (verbose) {
                fprintf(stderr, "Listening on network interface card %s.\n",
                        argv[i]);
            }
            g_NICname = argv[i];
        }
        else if (!strcmp(argv[i], "-li")) { // specify server-side logging
            if (++i > argc) {
                Usage(argv[0]);
            }
            if (verbose) {
                fprintf(stderr, "Incoming logfile name %s.\n", argv[i]);
            }
            g_inLogName = argv[i];
        }
        else if (!strcmp(argv[i], "-lo")) { // specify server-side logging
            if (++i > argc) {
                Usage(argv[0]);
            }
            if (verbose) {
                fprintf(stderr, "Outgoing logfile name %s.\n", argv[i]);
            }
            g_outLogName = argv[i];
        }
        else if (!strcmp(argv[i], "-flush")) {
            flush_continuously = true;
        }
        else if (argv[i][0] == '-') { // Unknown flag
            Usage(argv[0]);
        }
        else
            switch (realparams) { // Non-flag parameters
            case 0:
                port = atoi(argv[i]);
                realparams++;
                break;
            default:
                Usage(argv[0]);
            }
        i++;
    }

    // Need to have a global pointer to the connection so we can shut it down
    // in the signal handler (so we can close any open logfiles.)
    // Form the name based on the type of connection requested.  For a standard
    // VRPN UDP/TCP port, we give it the name "NIC:port" if there is a NIC name,
    // otherwise just ":port" for the default NIC.
    std::stringstream con_name;
    if (g_NICname) {
        con_name << g_NICname;
    }
    con_name << ":" << port;
    connection =
        vrpn_create_server_connection(con_name.str().c_str(), g_inLogName, g_outLogName);

    // Create the generic server object and make sure it is doing okay.
    generic_server = new vrpn_Generic_Server_Object(
        connection, config_file_name, verbose, bail_on_error);
    if ((generic_server == NULL) || !generic_server->doing_okay()) {
        fprintf(stderr, "Could not start generic server, exiting\n");
        shutDown();
    }

    // Open the Forwarder Server
    forwarderServer = new vrpn_Forwarder_Server(connection);

    // If we're set to auto-quit, then register a handler for the last
    // connection
    // dropped that will cause a callback which will exit.
    if (auto_quit) {
        int dlc_m_id =
            connection->register_message_type(vrpn_dropped_last_connection);
        connection->register_handler(dlc_m_id, handle_dlc, NULL);
    }

    // ********************************************************************
    // **                                                                **
    // **                MAIN LOOP                                       **
    // **                                                                **
    // ********************************************************************

    // ^C handler sets done to let us know to quit.
    while (!done) {
        // Let the generic object server do its thing.
        if (generic_server) {
            generic_server->mainloop();
        }

        // Send and receive all messages.
        connection->mainloop();

        // Save all log messages that are pending so that they are on disk
        // in case we end up exiting improperly.  This may slow down the
        // server waiting for disk writes to complete, but will more reliably
        // log messages.
        if (flush_continuously) {
            connection->save_log_so_far();
        }

        // Bail if the connection is in trouble.
        if (!connection->doing_okay()) {
            shutDown();
        }

        // Handle forwarding requests;  send messages
        // on auxiliary connections.
        forwarderServer->mainloop();

// Sleep so we don't eat the CPU
#if defined(_WIN32)
        if (milli_sleep_time >= 0) {
#else
        if (milli_sleep_time > 0) {
#endif
            vrpn_SleepMsecs(milli_sleep_time);
        }
    }

    shutDown();
    return 0;
}
