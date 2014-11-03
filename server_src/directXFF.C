//XXX Note that this application can be replaced by building the
// vrpn_server program with DirectInput included in the configuration
// file and then giving it an appropriate vrpn.cfg file.

#include "vrpn_DirectXFFJoystick.h"
#include "vrpn_Tracker_AnalogFly.h"

void	Usage(const char *s)
{
  fprintf(stderr,"Usage: %s [-warn] [-v]\n",s);
  fprintf(stderr,"       -warn: Only warn on errors (default is to bail)\n");
  fprintf(stderr,"       -v: Verbose\n");
  exit(-1);
}


int main (int argc, const char *argv[])
{

	int	bail_on_error = 1;
	int	verbose = 0;
	int	realparams = 0;
	
	// Parse the command line
	int i = 1;
	while (i < argc) {
	if (!strcmp(argv[i], "-warn")) {// Don't bail on errors
		bail_on_error = 0;
	  } else if (!strcmp(argv[i], "-v")) {	// Verbose
		verbose = 1;
	  } else if (argv[i][0] == '-') {	// Unknown flag
		Usage(argv[0]);
	  } else switch (realparams) {		// Non-flag parameters
	    case 0:
	    default:
		Usage(argv[0]);
	  }
	  i++;
	}

#ifdef _WIN32
	WSADATA wsaData; 
	int status;
    if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
         fprintf(stderr, "WSAStartup failed with %d\n", status);
    } else if(verbose) {
		 fprintf(stderr, "WSAStartup success\n");
	}
#endif

	const char handTrackerName[] = "Joystick0";

	vrpn_Connection * connection = vrpn_create_server_connection();
	// Create the joystick for analog and buttons
	vrpn_DirectXFFJoystick *joyServer = new vrpn_DirectXFFJoystick(handTrackerName,
                                       connection, 200, 200);
        if (joyServer==NULL) {
	  fprintf(stderr, "Could not create Joystick\n");
          return -1;
        }

	// Create the tracker to link to the joystick and provide transforms.
	// We put "*" in the name to indicate that it should use the same
	// connection.
	char  analogName[1024];
	sprintf(analogName, "*%s", (char*)handTrackerName);
	vrpn_Tracker_AnalogFlyParam afp;
	afp.x.name = (char*)analogName;
	afp.x.channel = 0;
	afp.x.offset = 0;
	afp.x.thresh = 0;
	afp.x.scale = (float)0.07;
	afp.x.power = 1;
	afp.y = afp.x; afp.y.channel = 1; afp.y.scale = (float)-0.07;
	afp.z = afp.x; afp.z.channel = 6; afp.z.scale = (float)0.2; afp.z.offset = (float)1.5;
	afp.sz = afp.x; afp.sz.channel = 5; afp.sz.scale = (float)0.06;
	vrpn_Tracker_AnalogFly *joyflyServer = new vrpn_Tracker_AnalogFly((char*) handTrackerName,
				connection, &afp, 200, vrpn_true);
        if (joyflyServer==NULL) {
	  fprintf(stderr, "phantom_init(): Could not create AnalogFly\n");
          return -1;
        }

	// Loop forever calling the mainloop()s for all devices
/*	struct timeval tv1, tv2;
*/	int counter = 0;
	while (1) {
	  /*
	  if ( counter == 0 ) {
	    vrpn_gettimeofday( &tv1, NULL );
	  }
	  counter++;
	  */

	  // Let all the trackers generate reports
	  joyServer->mainloop();
	  joyflyServer->mainloop();
	  
	  // Send and receive all messages
	  connection->mainloop();
	  
	  // Sleep for a short while to make sure we don't eat the whole CPU
	  //vrpn_SleepMsecs(1);
	  /*
	  if ( counter == 50 ) {
	    vrpn_gettimeofday( &tv2, NULL );
	    printf( "%f\n", 50.0/(((tv2.tv_sec*1000 +tv2.tv_usec/1000.0) - (tv1.tv_sec*1000 +tv1.tv_usec/1000.0))/1000.0));
	    counter = 0;
	  }
	  */
	}
}
 
