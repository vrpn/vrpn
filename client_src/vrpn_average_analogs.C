/*			vrpn_average_analogs.C

    This is a VRPN client program that will connect to one VRPN
    analog server device and print out the averaged values over
    time.  This can be useful for calibrating IMU devices.
*/

#include <stdio.h>  // for printf, fprintf, NULL, etc
#include <stdlib.h> // for exit, atoi
#ifndef _WIN32_WCE
#include <signal.h> // for signal, SIGINT
#endif
#include <string.h>              // for strcmp
#include <vrpn_Analog.h>         // for vrpn_ANALOGCB, etc
#include <vrpn_FileConnection.h> // For preload and accumulate settings
#include <vector>                // for vector

using namespace std;

int done = 0;                // Signals that the program should exit

//-------------------------------------
// This section contains the data structure that holds information on
// the devices that are created.  For each named device, a remote of each
// type (tracker, button, analog, dial, text) is created.  A particular device
// may only report a subset of these values, but that doesn't hurt anything --
// there simply will be no messages of the other types to receive and so
// nothing is printed.

class device_info {
public:
    std::vector<double> valueSums;
    std::vector<double> valueMins;
    std::vector<double> valueMaxs;
    size_t numValuesRead;
};

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void VRPN_CALLBACK handle_analog(void *userdata, const vrpn_ANALOGCB a)
{
    size_t i;
    device_info *dev = static_cast<device_info *>(userdata);

    // Make sure we have enough entries in the vector for all of the
    // values we have from the analog.  If we add a new one, zero out
    // the number of values read.
    while (dev->valueSums.size() < a.num_channel) {
      dev->valueSums.push_back(0);
      dev->numValuesRead = 0;
    }

    // Likewise for the min and max values.
    while (dev->valueMins.size() < a.num_channel) {
      dev->valueMins.push_back(a.channel[dev->valueMins.size()]);
    }
    while (dev->valueMaxs.size() < a.num_channel) {
      dev->valueMaxs.push_back(a.channel[dev->valueMaxs.size()]);
    }

    // Add in the new values and increment our value count.
    // Maintain max and min values.
    for (i = 0; i < a.num_channel; i++) {
      double val = a.channel[i];
      dev->valueSums[i] += val;
      if (dev->valueMins[i] > val) { dev->valueMins[i] = val; }
      if (dev->valueMaxs[i] < val) { dev->valueMaxs[i] = val; }
    }
    dev->numValuesRead++;

    printf("Averages:");
    for (i = 0; i < a.num_channel; i++) {
        printf(" %5.2f", dev->valueSums[i]/dev->numValuesRead);
    }
    printf("\n");
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
    fprintf(stderr, "Usage:  %s device\n", arg0);
    exit(0);
}

int main(int argc, char *argv[])
{
    // If we happen to open a file, neither preload nor accumulate the
    // messages in memory, to avoid crashing for huge files.
    vrpn_FILE_CONNECTIONS_SHOULD_PRELOAD = false;
    vrpn_FILE_CONNECTIONS_SHOULD_ACCUMULATE = false;

    // Parse arguments, creating objects as we go.  Arguments that
    // change the way a device is treated affect all devices that
    // follow on the command line.
    if (argc != 2) {
      Usage(argv[0]);
    }
    const char *anaName = argv[1];

    vrpn_Analog_Remote *ana = new vrpn_Analog_Remote(anaName);
    device_info dev;
    dev.numValuesRead = 0;
    ana->register_change_handler(&dev, handle_analog);

#ifndef _WIN32_WCE
    // signal handler so logfiles get closed right
    signal(SIGINT, handle_cntl_c);
#endif

    /*
     * main interactive loop
     */
    printf("Press ^C to exit.\n");
    while (!done) {
        ana->mainloop();

        // Sleep for 1ms so we don't eat the CPU
        vrpn_SleepMsecs(1);
    }

    delete ana;

    // Print high-resolution versions of the min, mean, and max for
    // each channel.
    printf("Min, mean, max values for each channel:\n");
    for (size_t i = 0; i < dev.valueSums.size(); i++) {
      printf("Channel %2d: %lg, %lg, %lg\n", static_cast<int>(i),
        dev.valueMins[i], dev.valueSums[i] / dev.numValuesRead,
        dev.valueMaxs[i]);
    }
    // @todo

    return 0;
} /* main */

