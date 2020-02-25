/*			vrpn_print_performance.C

    This is a VRPN client program that will connect to one or more VRPN
    server devices and print out the rate of any of a number of	message types.
    These include only tracker messages at this time, but the rest are easy.
    While it doesn't (currently) attempt to consider latency, its measurement of
    update rate might be informative to some.
*/

#include <stdio.h>  // for printf, NULL, fprintf, etc
#include <stdlib.h> // for exit, atoi
#ifndef _WIN32_WCE
#include <signal.h> // for signal, SIGINT
#endif
#include <string.h>              // for strcmp
#include <vrpn_Analog.h>         // for vrpn_Analog_Remote, etc
#include <vrpn_Button.h>         // for vrpn_Button_Remote, etc
#include <vrpn_FileConnection.h> // For preload and accumulate settings
#include <vrpn_Shared.h>         // for vrpn_gettimeofday, etc
#include <vrpn_Tracker.h>        // for vrpn_TRACKERACCCB, etc
#include <vector>                // for vector

#include "vrpn_BaseClass.h" // for vrpn_System_TextPrinter, etc
#include "vrpn_Configure.h" // for VRPN_CALLBACK

using namespace std;

int done = 0;               // Signals that the program should exit
double report_interval = 1; // Every n seconds a report will be printed

//-------------------------------------
// This section contains the data structure that holds information on
// the devices that are created.  For each named device, a remote of each
// type (tracker, button, analog, dial, text) is created.  A particular device
// may only report a subset of these values, but that doesn't hurt anything --
// there simply will be no messages of the other types to receive and so
// nothing is printed.

class device_info {
public:
    char *name;
    vrpn_Tracker_Remote *tkr;
    vrpn_Button_Remote *btn;
    vrpn_Analog_Remote *ana;
};
const unsigned MAX_DEVICES = 50;

//-------------------------------------
// This section contains the data structure that is used to determine how
// often to print a report for each sensor of each tracker.  Each element
// contains a counter that is used by the callback routine to keep track
// of how many it has skipped.  There is an element for each possible sensor.
// A new array of elements is created for each new tracker object, and a
// pointer to it is passed as the userdata pointer to the callback handlers.
// A separate array is kept for the position, velocity, and acceleration for
// each
// tracker.  The structure passed to the callback handler also has a
// string that is the name of the tracker.

class t_user_callback {
public:
    char t_name[vrpn_MAX_TEXT_LEN];
    vector<unsigned> t_counts;
    vector<struct timeval> t_last_report;
};

struct timeval t_analog_last_report;
unsigned t_analog_count;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void VRPN_CALLBACK
handle_tracker_pos_quat(void *userdata, const vrpn_TRACKERCB t)
{
    t_user_callback *t_data = static_cast<t_user_callback *>(userdata);

    // Make sure we have a count value for this sensor
    while (t_data->t_counts.size() <= static_cast<unsigned>(t.sensor)) {
        t_data->t_counts.push_back(0);
    }
    struct timeval now;
    double interval;
    vrpn_gettimeofday(&now, NULL);
    // Make sure we have a count value for this sensor
    while (t_data->t_last_report.size() <= static_cast<unsigned>(t.sensor)) {
        t_data->t_last_report.push_back(now);
    }

    interval =
        vrpn_TimevalDurationSeconds(now, t_data->t_last_report[t.sensor]);
    t_data->t_counts[t.sensor]++;

    // See if it's been long enough to display a frequency notification
    if (interval >= report_interval) {
        double frequency = t_data->t_counts[t.sensor] / interval;
        t_data->t_counts[t.sensor] = 0;
        t_data->t_last_report[t.sensor] = now;
        printf("Tracker %s, sensor %d:\t%5.2f Hz (%5.2f sec)\n", t_data->t_name,
               t.sensor, frequency, interval);
    }
}

void VRPN_CALLBACK handle_tracker_vel(void *userdata, const vrpn_TRACKERVELCB t)
{
    t_user_callback *t_data = static_cast<t_user_callback *>(userdata);

    // Make sure we have a count value for this sensor
    while (t_data->t_counts.size() <= static_cast<unsigned>(t.sensor)) {
        t_data->t_counts.push_back(0);
    }
    struct timeval now;
    double interval;
    vrpn_gettimeofday(&now, NULL);
    // Make sure we have a count value for this sensor
    while (t_data->t_last_report.size() <= static_cast<unsigned>(t.sensor)) {
        t_data->t_last_report.push_back(now);
    }

    interval =
        vrpn_TimevalDurationSeconds(now, t_data->t_last_report[t.sensor]);
    t_data->t_counts[t.sensor]++;

    // See if it's been long enough to display a frequency notification
    if (interval >= report_interval) {
        double frequency = t_data->t_counts[t.sensor] / interval;
        t_data->t_counts[t.sensor] = 0;
        t_data->t_last_report[t.sensor] = now;
        printf("Tracker %s, sensor %d:\t\t%5.2f messages per second averaged "
               "over %5.2f seconds\n",
               t_data->t_name, t.sensor, frequency, interval);
    }
}

void VRPN_CALLBACK handle_tracker_acc(void *userdata, const vrpn_TRACKERACCCB t)
{
    t_user_callback *t_data = static_cast<t_user_callback *>(userdata);

    // Make sure we have a count value for this sensor
    while (t_data->t_counts.size() <= static_cast<unsigned>(t.sensor)) {
        t_data->t_counts.push_back(0);
    }
    struct timeval now;
    double interval;
    vrpn_gettimeofday(&now, NULL);
    // Make sure we have a count value for this sensor
    while (t_data->t_last_report.size() <= static_cast<unsigned>(t.sensor)) {
        t_data->t_last_report.push_back(now);
    }

    interval =
        vrpn_TimevalDurationSeconds(now, t_data->t_last_report[t.sensor]);
    t_data->t_counts[t.sensor]++;
    // See if it's been long enough to display a frequency notification
    if (interval >= report_interval) {
        double frequency = t_data->t_counts[t.sensor] / interval;
        t_data->t_counts[t.sensor] = 0;
        t_data->t_last_report[t.sensor] = now;
        printf("Tracker %s, sensor %d:\t\t%5.2f messages per second averaged "
               "over %5.2f seconds\n",
               t_data->t_name, t.sensor, frequency, interval);
    }
}

void VRPN_CALLBACK handle_button(void *userdata, const vrpn_BUTTONCB b)
{
    const char *name = (const char *)userdata;

    printf("Button %s, number %d was just %s\n", name, b.button,
           b.state ? "pressed" : "released");
}

void VRPN_CALLBACK handle_analog(void *userdata, const vrpn_ANALOGCB /*a*/)
{
    const char *name = (const char *)userdata;
    struct timeval now;
    double interval;
    vrpn_gettimeofday(&now, NULL);

    interval = vrpn_TimevalDurationSeconds(now, t_analog_last_report);
    t_analog_count++;
    // See if it's been long enough to display a frequency notification
    if (interval >= report_interval) {
        double frequency = t_analog_count / interval;
        t_analog_count = 0;
        t_analog_last_report = now;
        printf("Analog %s:\t\t%5.2f messages per second averaged over %5.2f "
               "seconds\n",
               name, frequency, interval);
    }
    /*
        const char *name = (const char *)userdata;
        int i;
        printf("Analog %s:\n         %5.2f", name, a.channel[0]);
        for (i = 1; i < a.num_channel; i++) {
        printf(", %5.2f", a.channel[i]);
        }
        printf(" (%d chans)\n", a.num_channel);
    */
}

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
        "Usage:  %s [--notracker] [--nobutton] [--noanalog] [--nodial]\n"
        "           [--reportinterval n]\n"
        "           [--notext] device1 [device2] [device3] [device4] [...]\n"
        "  -trackerstride:  Print every nth report from each tracker sensor\n"
        "  -notracker:  Don't print tracker reports for following devices\n"
        "  -nobutton:  Don't print button reports for following devices\n"
        "  -noanalog:  Don't print analog reports for following devices\n"
        "  deviceX:  VRPN name of device to connect to (eg: Tracker0@ioglab)\n"
        "  The default behavior is to print all message types for all devices "
        "listed\n"
        "  The order of the parameters can be changed to suit\n",
        arg0);

    exit(0);
}

int main(int argc, char *argv[])
{
    int print_for_tracker = 1; // Print tracker reports?
    int print_for_button = 1;  // Print button reports?
    int print_for_analog = 1;  // Print analog reports?

    // If we happen to open a file, neither preload nor accumulate the
    // messages in memory, to avoid crashing for huge files.
    vrpn_FILE_CONNECTIONS_SHOULD_PRELOAD = false;
    vrpn_FILE_CONNECTIONS_SHOULD_ACCUMULATE = false;

    device_info device_list[MAX_DEVICES];
    int num_devices = 0;

    int i;

    // Parse arguments, creating objects as we go.  Arguments that
    // change the way a device is treated affect all devices that
    // follow on the command line.
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-notracker")) {
            print_for_tracker = 0;
        }
        else if (!strcmp(argv[i], "--nobutton")) {
            print_for_button = 0;
        }
        else if (!strcmp(argv[i], "--noanalog")) {
            print_for_analog = 0;
        }
        else if (!strcmp(argv[i], "--reportinterval")) {
            if (++i >= argc) {
                Usage(argv[0]);
            }
            report_interval = atoi(argv[i]);
            if (report_interval <= 0) {
                fprintf(stderr,
                        "--reportinterval argument must be 1 or greater\n");
                return -1;
            }
        }
        else { // Create a device and connect to it.
            device_info *dev;

            // Make sure we have enough room for the new device
            if (num_devices == MAX_DEVICES) {
                fprintf(stderr, "Too many devices!\n");
                exit(-1);
            }

            // Name the device and open it as everything
            dev = &device_list[num_devices];
            dev->name = argv[i];
            dev->tkr = new vrpn_Tracker_Remote(dev->name);
            dev->ana = new vrpn_Analog_Remote(dev->name);
            dev->btn = new vrpn_Button_Remote(dev->name);
            if ((dev->tkr == NULL) || (dev->ana == NULL) ||
                (dev->btn == NULL)) {
                fprintf(stderr, "Error opening %s\n", dev->name);
                return -1;
            }
            else {
                printf("Opened %s as:", dev->name);
            }

            struct timeval now;
            vrpn_gettimeofday(&now, NULL);

            // If we are printing the tracker reports, prepare the
            // user-data callbacks and hook them up to be called with
            // the correct data for this device.
            if (print_for_tracker) {
                vrpn_Tracker_Remote *tkr = dev->tkr;
                t_user_callback *tc1 = new t_user_callback;
                t_user_callback *tc2 = new t_user_callback;
                t_user_callback *tc3 = new t_user_callback;

                if ((tc1 == NULL) || (tc2 == NULL) || (tc3 == NULL)) {
                    fprintf(stderr, "Out of memory\n");
                }
                printf(" Tracker");

                // Fill in the user-data callback information
                vrpn_strcpy(tc1->t_name, dev->name);
                vrpn_strcpy(tc2->t_name, dev->name);
                vrpn_strcpy(tc3->t_name, dev->name);

                // Set up the tracker callback handlers
                tkr->register_change_handler(tc1, handle_tracker_pos_quat);
                tkr->register_change_handler(tc2, handle_tracker_vel);
                tkr->register_change_handler(tc3, handle_tracker_acc);
            }

            if (print_for_button) {
                printf(" Button");
                dev->btn->register_change_handler(dev->name, handle_button);
            }

            if (print_for_analog) {
                printf(" Analog");
                t_analog_last_report = now;
                t_analog_count = 0;
                dev->ana->register_change_handler(dev->name, handle_analog);
            }

            printf(".\n");
            num_devices++;
        }
    }
    if (num_devices == 0) {
        Usage(argv[0]);
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
        int i;

        // Let all the devices do their things
        for (i = 0; i < num_devices; i++) {
            device_list[i].tkr->mainloop();
            device_list[i].btn->mainloop();
            device_list[i].ana->mainloop();
        }

        // Sleep for 1ms so we don't eat the CPU
        vrpn_SleepMsecs(1);
    }

    return 0;
} /* main */
