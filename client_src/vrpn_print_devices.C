/*			vrpn_print_devices.C

    This is a VRPN client program that will connect to one or more VRPN
    server devices and print out descriptions of any of a number of
    message types.  These include at least tracker, button, dial and analog
    messages.  It is useful for testing whether a server is up and running,
    and perhaps for simple debugging.
*/

#include <stdio.h>  // for printf, fprintf, NULL, etc
#include <stdlib.h> // for exit, atoi
#ifndef _WIN32_WCE
#include <signal.h> // for signal, SIGINT
#endif
#include <string.h>              // for strcmp
#include <vrpn_Analog.h>         // for vrpn_ANALOGCB, etc
#include <vrpn_Button.h>         // for vrpn_Button_Remote, etc
#include <vrpn_Dial.h>           // for vrpn_Dial_Remote, etc
#include <vrpn_FileConnection.h> // For preload and accumulate settings
#include <vrpn_Shared.h>         // for vrpn_SleepMsecs
#include <vrpn_Text.h>           // for vrpn_Text_Receiver, etc
#include <vrpn_Tracker.h>        // for vrpn_TRACKERACCCB, etc
#include <vector>                // for vector

#include "vrpn_BaseClass.h" // for vrpn_System_TextPrinter, etc
#include "vrpn_Configure.h" // for VRPN_CALLBACK
#include "vrpn_Types.h"     // for vrpn_float64, vrpn_int32

using namespace std;

int done = 0;                // Signals that the program should exit
unsigned tracker_stride = 1; // Every nth report will be printed

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
    vrpn_Dial_Remote *dial;
    vrpn_Text_Receiver *text;
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
};

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

    // See if we have gotten enough reports from this sensor that we should
    // print this one.  If so, print and reset the count.
    if (++t_data->t_counts[t.sensor] >= tracker_stride) {
        t_data->t_counts[t.sensor] = 0;
        printf("Tracker %s, sensor %d:\n     pos (%5.2f, %5.2f, %5.2f); "
               "quat (%5.2f, %5.2f, %5.2f, %5.2f)\n",
               t_data->t_name, t.sensor, t.pos[0], t.pos[1], t.pos[2],
               t.quat[0], t.quat[1], t.quat[2], t.quat[3]);
    }
}

void VRPN_CALLBACK handle_tracker_vel(void *userdata, const vrpn_TRACKERVELCB t)
{
    t_user_callback *t_data = static_cast<t_user_callback *>(userdata);

    // Make sure we have a count value for this sensor
    while (t_data->t_counts.size() <= static_cast<unsigned>(t.sensor)) {
        t_data->t_counts.push_back(0);
    }

    // See if we have gotten enough reports from this sensor that we should
    // print this one.  If so, print and reset the count.
    if (++t_data->t_counts[t.sensor] >= tracker_stride) {
        t_data->t_counts[t.sensor] = 0;
        printf("Tracker %s, sensor %d:\n     vel (%5.2f, %5.2f, %5.2f); "
               "quatvel (%5.2f, %5.2f, %5.2f, %5.2f), dt %5.2f\n",
               t_data->t_name, t.sensor, t.vel[0], t.vel[1], t.vel[2],
               t.vel_quat[0], t.vel_quat[1], t.vel_quat[2], t.vel_quat[3],
               t.vel_quat_dt);
    }
}

void VRPN_CALLBACK handle_tracker_acc(void *userdata, const vrpn_TRACKERACCCB t)
{
    t_user_callback *t_data = static_cast<t_user_callback *>(userdata);

    // Make sure we have a count value for this sensor
    while (t_data->t_counts.size() <= static_cast<unsigned>(t.sensor)) {
        t_data->t_counts.push_back(0);
    }

    // See if we have gotten enough reports from this sensor that we should
    // print this one.  If so, print and reset the count.
    if (++t_data->t_counts[t.sensor] >= tracker_stride) {
        t_data->t_counts[t.sensor] = 0;
        printf("Tracker %s, sensor %d:\n     acc (%5.2f, %5.2f, %5.2f); "
               "quatacc (%5.2f, %5.2f, %5.2f, %5.2f), dt %5.2f\n",
               t_data->t_name, t.sensor, t.acc[0], t.acc[1], t.acc[2],
               t.acc_quat[0], t.acc_quat[1], t.acc_quat[2], t.acc_quat[3],
               t.acc_quat_dt);
    }
}

void VRPN_CALLBACK handle_button(void *userdata, const vrpn_BUTTONCB b)
{
    const char *name = (const char *)userdata;

    printf("##########################################\r\n"
           "Button %s, number %d was just %s\n"
           "##########################################\r\n",
           name, b.button, b.state ? "pressed" : "released");
}

void VRPN_CALLBACK
handle_button_states(void *userdata, const vrpn_BUTTONSTATESCB b)
{
    const char *name = (const char *)userdata;

    printf("Button %s has %d buttons with states:", name, b.num_buttons);
    int i;
    for (i = 0; i < b.num_buttons; i++) {
        printf(" %d", b.states[i]);
    }
    printf("\n");
}

void VRPN_CALLBACK handle_analog(void *userdata, const vrpn_ANALOGCB a)
{
    int i;
    const char *name = (const char *)userdata;

    printf("Analog %s:\n         %5.2f", name, a.channel[0]);
    for (i = 1; i < a.num_channel; i++) {
        printf(", %5.2f", a.channel[i]);
    }
    printf(" (%d chans)\n", a.num_channel);
}

void VRPN_CALLBACK handle_dial(void *userdata, const vrpn_DIALCB d)
{
    const char *name = (const char *)userdata;

    printf("Dial %s, number %d was moved by %5.2f\n", name, d.dial, d.change);
}

void VRPN_CALLBACK handle_text(void *userdata, const vrpn_TEXTCB t)
{
    const char *name = (const char *)userdata;

    // Warnings and errors are printed by the system text printer.
    if (t.type == vrpn_TEXT_NORMAL) {
        printf("%s: Text message: %s\n", name, t.message);
    }
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
        "Usage:  %s [-notracker] [-nobutton] [-noanalog] [-nodial]\n"
        "           [-trackerstride n]\n"
        "           [-notext] device1 [device2] [device3] [device4] [...]\n"
        "  -trackerstride:  Print every nth report from each tracker sensor\n"
        "  -notracker:  Don't print tracker reports for following devices\n"
        "  -nobutton:  Don't print button reports for following devices\n"
        "  -noanalog:  Don't print analog reports for following devices\n"
        "  -nodial:  Don't print dial reports for following devices\n"
        "  -notext:  Don't print text messages (warnings, errors) for "
        "following devices\n"
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
    int print_for_dial = 1;    // Print dial reports?
    int print_for_text = 1;    // Print warning/error messages?

    // If we happen to open a file, neither preload nor accumulate the
    // messages in memory, to avoid crashing for huge files.
    vrpn_FILE_CONNECTIONS_SHOULD_PRELOAD = false;
    vrpn_FILE_CONNECTIONS_SHOULD_ACCUMULATE = false;

    device_info device_list[MAX_DEVICES];
    unsigned num_devices = 0;

    int i;

    // Parse arguments, creating objects as we go.  Arguments that
    // change the way a device is treated affect all devices that
    // follow on the command line.
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-notracker")) {
            print_for_tracker = 0;
        }
        else if (!strcmp(argv[i], "-nobutton")) {
            print_for_button = 0;
        }
        else if (!strcmp(argv[i], "-noanalog")) {
            print_for_analog = 0;
        }
        else if (!strcmp(argv[i], "-nodial")) {
            print_for_dial = 0;
        }
        else if (!strcmp(argv[i], "-notext")) {
            print_for_text = 0;
        }
        else if (!strcmp(argv[i], "-trackerstride")) {
            if (++i >= argc) {
                Usage(argv[0]);
            }
            if (atoi(argv[i]) <= 0) {
                fprintf(stderr,
                        "-trackerstride argument must be 1 or greater\n");
                return -1;
            }
            tracker_stride = atoi(argv[i]);
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
            dev->dial = new vrpn_Dial_Remote(dev->name);
            dev->text = new vrpn_Text_Receiver(dev->name);
            if ((dev->ana == NULL) || (dev->btn == NULL) ||
                (dev->dial == NULL) || (dev->tkr == NULL) ||
                (dev->text == NULL)) {
                fprintf(stderr, "Error opening %s\n", dev->name);
                return -1;
            }
            else {
                printf("Opened %s as:", dev->name);
            }

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
                dev->btn->register_states_handler(dev->name,
                                                  handle_button_states);
            }

            if (print_for_analog) {
                printf(" Analog");
                dev->ana->register_change_handler(dev->name, handle_analog);
            }

            if (print_for_dial) {
                printf(" Dial");
                dev->dial->register_change_handler(dev->name, handle_dial);
            }

            if (print_for_text) {
                printf(" Text");
                dev->text->register_message_handler(dev->name, handle_text);
            }
            else {
                vrpn_System_TextPrinter.remove_object(dev->tkr);
                vrpn_System_TextPrinter.remove_object(dev->btn);
                vrpn_System_TextPrinter.remove_object(dev->ana);
                vrpn_System_TextPrinter.remove_object(dev->dial);
                vrpn_System_TextPrinter.remove_object(dev->text);
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
        unsigned i;

        // Let all the devices do their things
        for (i = 0; i < num_devices; i++) {
            device_list[i].tkr->mainloop();
            device_list[i].btn->mainloop();
            device_list[i].ana->mainloop();
            device_list[i].dial->mainloop();
            device_list[i].text->mainloop();
        }

        // Sleep for 1ms so we don't eat the CPU
        vrpn_SleepMsecs(1);
    }

    // Delete all devices.
    {
        unsigned i;
        for (i = 0; i < num_devices; i++) {
            delete device_list[i].tkr;
            delete device_list[i].btn;
            delete device_list[i].ana;
            delete device_list[i].dial;
            delete device_list[i].text;
        }
    }

    return 0;
} /* main */
