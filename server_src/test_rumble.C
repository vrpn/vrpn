/*			test_rumble.C

	This is a VRPN client program that will connect to a vrpn_Analog_Output
        device and alternately send 1 and then 0 to its zeroeth element.  This
        is to test the rumble function.
*/

#include <stdio.h>                      // for printf, NULL, fprintf, etc
#include <stdlib.h>                     // for exit
#ifndef	_WIN32_WCE
#include <signal.h>                     // for signal, SIGINT
#endif
#include <vrpn_Analog_Output.h>         // for vrpn_Analog_Output_Remote

#include "vrpn_Shared.h"                // for timeval, vrpn_SleepMsecs, etc
#include "vrpn_Types.h"                 // for vrpn_float64

int done = 0;	    // Signals that the program should exit

// WARNING: On Windows systems, this handler is called in a separate
// thread from the main program (this differs from Unix).  To avoid all
// sorts of chaos as the main program continues to handle packets, we
// set a done flag here and let the main program shut down in its own
// thread by calling shutdown() to do all of the stuff we used to do in
// this handler.

void handle_cntl_c(int) {
    done = 1;
}

void Usage (const char * arg0) {
  fprintf(stderr,
"Usage:  %s device\n",
  arg0);

  exit(0);
}

int main (int argc, char * argv [])
{
  vrpn_Analog_Output_Remote  *anaout = NULL;

  int i;

  // Parse arguments, creating objects as we go.  Arguments that
  // change the way a device is treated affect all devices that
  // follow on the command line.
  if (argc != 2) { Usage(argv[0]); }
  for (i = 1; i < argc; i++) {

	// Name the device and open it as everything
        anaout = new vrpn_Analog_Output_Remote(argv[i]);

  }

#ifndef	_WIN32_WCE
  // signal handler so logfiles get closed right
  signal(SIGINT, handle_cntl_c);
#endif

  // Turn the rumble on and set up to change.
  struct  timeval last_change;
  vrpn_float64  state = 1.0;
  vrpn_gettimeofday(&last_change, NULL);
  anaout->request_change_channel_value(0, state);

/* 
 * main interactive loop
 */
  printf("Turing rumble on and off every two seconds\n");
  printf("Press ^C to exit.\n");
  while ( ! done ) {

    // See if it has been long enough for a change; if so, change state.
    struct  timeval now;
    vrpn_gettimeofday(&now, NULL);
    if (now.tv_sec - last_change.tv_sec > 2) {
      state = 1 - state;
      last_change = now;
      anaout->request_change_channel_value(0, state);
    }

    // Let the device do its thing.
    anaout->mainloop();

    // Sleep for 1ms so we don't eat the CPU
    vrpn_SleepMsecs(1);
  }

  delete anaout;
  return 0;
}   /* main */


