#include <stdio.h>                      // for fprintf, stderr, printf, etc
#include <stdlib.h>                     // for atoi, exit
#include <string.h>                     // for strcmp
#include <vrpn_Shared.h>                // for vrpn_gettimeofday, vrpn_SleepMsecs, timeval

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Poser.h"                 // for vrpn_Poser_Remote
#include "vrpn_Tracker.h"               // for vrpn_TRACKERCB, etc

static	bool  g_verbose = false;

void Usage (const char * s)
{
  fprintf(stderr,"Usage: %s [-v] [-millisleep n] [trackername [posername]]\n",s);
  fprintf(stderr,"       -millisleep: Sleep n milliseconds each loop cycle\n"); 
  fprintf(stderr,"                    (if no option is specified, the Windows architecture\n");
  fprintf(stderr,"                     will free the rest of its time slice on each loop\n");
  fprintf(stderr,"                     but leave the processes available to be run immediately;\n");
  fprintf(stderr,"                     a 1ms sleep is the default on all other architectures).\n");
  fprintf(stderr,"                    -millisleep 0 is recommended when using this server and\n"); 
  fprintf(stderr,"                     a client on the same uniprocessor CPU Win32 PC.\n");
  fprintf(stderr,"                    -millisleep -1 will cause the server process to use the\n"); 
  fprintf(stderr,"                     whole CPU on any uniprocessor machine.\n");
  fprintf(stderr,"       -v: Verbose.\n");
  fprintf(stderr,"       trackername: String name of the tracker device (default Tracker0@localhost)\n");
  fprintf(stderr,"       posername: String name of the poser device (default Poser0@localhost)\n");
  exit(0);
}

//--------------------------------------------------------------------------------------
// This function takes the pose reported by the tracker and sends it
// directly to the poser, without changing it in any way.  For this to
// work properly, the tracker must report translations in just the right
// space for the poser.

void	VRPN_CALLBACK handle_tracker_update(void *userdata, const vrpn_TRACKERCB t)
{
  // Turn the pointer into a poser pointer.
  vrpn_Poser_Remote *psr= (vrpn_Poser_Remote*)userdata;

  // Get the data from the tracker and send it to the poser.
  struct timeval now;
  vrpn_gettimeofday(&now, NULL);
  psr->request_pose(now, t.pos, t.quat);
}

int main (int argc, char * argv[])
{
  char default_tracker[] = "Tracker0@localhost";
  char default_poser[] = "Poser0@localhost";
  char 	*tracker_client_name = default_tracker;
  char 	*poser_client_name = default_poser;
  int	realparams = 0;
  int	i;

#ifdef	_WIN32
  // On Windows, the millisleep with 0 option frees up the CPU time slice for other jobs
  // but does not sleep for a specific time.  On Unix, this returns immediately and does
  // not do anything but waste cycles.
  int	milli_sleep_time = 0;		// How long to sleep each iteration (default: free the timeslice but be runnable again immediately)
#else
  int	milli_sleep_time = 1;		// How long to sleep each iteration (default: 1ms)
#endif
#ifdef WIN32
  WSADATA wsaData; 
  int status;
  if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
    fprintf(stderr, "WSAStartup failed with %d\n", status);
    return(1);
  }
#endif // not WIN32

  //--------------------------------------------------------------------------------------
  // Parse the command line
  i = 1;
  while (i < argc) {
    if (!strcmp(argv[i], "-v")) {		// Specify config-file name
	  g_verbose = true;
    } else if (!strcmp(argv[i], "-millisleep")) {	// How long to sleep each loop?
	  if (++i > argc) { Usage(argv[0]); }
	  milli_sleep_time = atoi(argv[i]);
    } else if (argv[i][0] == '-') {	// Unknown flag
	  Usage(argv[0]);
    } else switch (realparams) {		// Non-flag parameters
      case 0:
	tracker_client_name = argv[i];
	realparams++;
	break;
      case 1:
	poser_client_name = argv[i];
	realparams++;
	break;
      default:
	Usage(argv[0]);
    }
    i++;
  }
  if (realparams > 2) {
    Usage(argv[0]);
  }

  //--------------------------------------------------------------------------------------
  // Open the tracker and poser objects that we're going to use.
  if (g_verbose) {
    printf("Opening tracker %s, poser %s\n", tracker_client_name, poser_client_name);
  }
  vrpn_Tracker_Remote *tkr = new vrpn_Tracker_Remote(tracker_client_name);
  vrpn_Poser_Remote *psr = new vrpn_Poser_Remote(poser_client_name);

  //--------------------------------------------------------------------------------------
  // Set up the callback handler on the tracker object and pass it
  // a pointer to the poser it is to use.  Only ask for data from
  // sensor 0.
  tkr->register_change_handler(psr, handle_tracker_update, 0);

  //--------------------------------------------------------------------------------------
  // Loop forever until killed.
  while (1) {
    tkr->mainloop();
    psr->mainloop();
    vrpn_SleepMsecs(milli_sleep_time);
  }

  return 0;
}
