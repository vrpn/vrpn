#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "vrpn_Connection.h"
#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
#include "vrpn_3Space.h"
#include "vrpn_Tracker_Fastrak.h"
#include "vrpn_Flock.h"
#include "vrpn_Flock_Parallel.h"
#include "vrpn_Dyna.h"
#include "vrpn_Sound.h"


#define MAX_TRACKERS 100
#define MAX_BUTTONS 100
#define MAX_SOUNDS 2

void	Usage(char *s)
{
  fprintf(stderr,"Usage: %s [-f filename] [-warn] [-v]\n",s);
  fprintf(stderr,"       -f: Full path to config file (default vrpn.cfg)\n");
  fprintf(stderr,"       -warn: Only warn on errors (default is to bail)\n");
  fprintf(stderr,"       -v: Verbose\n");
  exit(-1);
}

vrpn_Tracker	*trackers[MAX_TRACKERS];
int		num_trackers = 0;
vrpn_Button	*buttons[MAX_BUTTONS];
int		num_buttons = 0;
vrpn_Sound	*sounds[MAX_SOUNDS];
int		num_sounds = 0;

// install a signal handler to shut down the trackers and buttons
#ifndef WIN32
#include <signal.h>
void closeDevices();
#ifdef sgi
void sighandler( ... )
#else
void sighandler( int signal )
#endif
{
    closeDevices();
    return;
}

void closeDevices() {
  int i;
  for (i=0;i < num_buttons; i++) {
    fprintf(stderr, "\nClosing button %d ...", i);
    delete buttons[i];
  }
  for (i=0;i < num_trackers; i++) {
    fprintf(stderr, "\nClosing tracker %d ...", i);
    delete trackers[i];
  }
  fprintf(stderr, "\nAll devices closed. Exiting ...\n");
  exit(0);
}
#endif

main (int argc, char *argv[])
{
	char	*config_file_name = "vrpn.cfg";
	FILE	*config_file;
	int	bail_on_error = 1;
	int	verbose = 1;
	int	realparams = 0;
	int	i;
	int 	loop=0;
#ifdef WIN32
	WSADATA wsaData; 
	int status;
	if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
	  fprintf(stderr, "WSAStartup failed with %d\n", status);
	  exit(1);
	}
#else
#ifdef sgi
  sigset( SIGINT, sighandler );
  sigset( SIGKILL, sighandler );
  sigset( SIGTERM, sighandler );
  sigset( SIGPIPE, sighandler );
#else
  signal( SIGINT, sighandler );
  signal( SIGKILL, sighandler );
  signal( SIGTERM, sighandler );
  signal( SIGPIPE, sighandler );
#endif // sgi
#endif

	vrpn_Synchronized_Connection	connection;

	// Parse the command line
	i = 1;
	while (i < argc) {
	  if (!strcmp(argv[i], "-f")) {		// Specify config-file name
		if (++i > argc) { Usage(argv[0]); }
		config_file_name = argv[i];
	  } else if (!strcmp(argv[i], "-warn")) {// Don't bail on errors
		bail_on_error = 0;
	  } else if (!strcmp(argv[i], "-v")) {	// Verbose
		verbose = 1;
	  } else if (argv[i][0] == '-') {	// Unknown flag
		Usage(argv[0]);
	  } else switch (realparams) {		// Non-flag parameters
	    default:
		Usage(argv[0]);
	  }
	  i++;
	}


	// Open the configuration file
	if (verbose) printf("Reading from config file %s\n", config_file_name);
	if ( (config_file = fopen(config_file_name, "r")) == NULL) {
		perror("Cannot open config file");
		fprintf(stderr,"  (filename %s)\n", config_file_name);
		return -1;
	}

	// Read the configuration file, creating a device for each entry.
	// Each entry is on one line, which starts with the name of the
	//   class of the object that is to be created.
	// If we fail to open a certain device, print a message and decide
	//  whether we should bail.
      {	char	line[512];	// Line read from the input file
        char *pch;
	char    scrap[512];
	char	s1[512],s2[512],s3[512];	// String parameters
	int	i1, i2;				// Integer parameters
	float	f1;				// Float parameters

	// Read lines from the file until we run out
	while ( fgets(line, sizeof(line), config_file) != NULL ) {

	  // Make sure the line wasn't too long
	  if (strlen(line) >= sizeof(line)-1) {
		fprintf(stderr,"Line too long in config file: %s\n",line);
		if (bail_on_error) { return -1; }
		else { continue; }	// Skip this line
	  }

	  if ((strlen(line)<3)||(line[0]=='#')) {
	    // comment or empty line -- ignore
	    continue;
	  }

	  // copy for strtok work
	  strncpy(scrap, line, 512);
	  // Figure out the device from the name and handle appropriately

	  // WARNING: SUBSTRINGS WILL MATCH THE EARLIER STRING, SO 
	  // ADD AN EMPTY SPACE TO THE END OF STATIC STRINGS!!!!

	  //	  #define isit(s) !strncmp(line,s,strlen(s))
#define isit(s) !strcmp(pch=strtok(scrap," \t"),s)
#define next() pch += strlen(pch) + 1
	  if (isit("vrpn_Linux_Sound")) {
	    // Get the arguments (sound_name)
	    next();
	    if (sscanf(pch,"%511s",s2) != 1) {
	      fprintf(stderr,"Bad vrpn_Linux_Sound line: %s\n",line);
	      if (bail_on_error) { return -1; }
	      else { continue; }	// Skip this line
	    }

	    // Make sure there's room for a new sound server
	    if (num_sounds >= MAX_SOUNDS) {
	      fprintf(stderr,"Too many sound servers in config file");
	      if (bail_on_error) { return -1; }
	      else { continue; }	// Skip this line
	    }

	    // Open the sound server
	    if (verbose) 
	      printf("Opening vrpn_Linux_Sound: %s\n", s2);
	    if ((sounds[num_sounds] =
		  new vrpn_Linux_Sound(s2, &connection)) == NULL) {
		fprintf(stderr,"Can't create new vrpn_Linux_Sound\n");
		if (bail_on_error) { return -1; }
		else { continue; }	// Skip this line
	    } else {
		num_sounds++;
	    }

	  } else if (isit("vrpn_Tracker_Dyna")) {
	    next();
	    // Get the arguments (class, tracker_name, sensors,port, baud)
	    if (sscanf(pch,"%511s%d%511s%d",s2,&i2, s3, &i1) != 4) {
	      fprintf(stderr,"Bad vrpn_Tracker_Dyan line: %s\n",line);
	      if (bail_on_error) { return -1; }
	      else { continue; }	// Skip this line
	    }

	    // Make sure there's room for a new tracker
	    if (num_trackers >= MAX_TRACKERS) {
	      fprintf(stderr,"Too many trackers in config file");
	      if (bail_on_error) { return -1; }
	      else { continue; }	// Skip this line
	    }


	    // Open the tracker
	    if (verbose) 
	      printf("Opening vrpn_Tracker_Dyan: %s on port %s, baud %d, %d sensors\n",
		    s2,s3,i1, i2);
	    if ((trackers[num_trackers] =
		  new vrpn_Tracker_Dyna(s2, &connection, i2, s3, i1)) == NULL)               
	      {
		fprintf(stderr,"Can't create new vrpn_Tracker_Dyna\n");
		if (bail_on_error) { return -1; }
		else { continue; }	// Skip this line
	      } else {
		num_trackers++;
	      }
	  }
	  else if (isit("vrpn_Tracker_3Space")) {
	    next();
		// Get the arguments (class, tracker_name, port, baud)
		if (sscanf(pch,"%511s%511s%d",s2,s3,&i1) != 3) {
		  fprintf(stderr,"Bad vrpn_Tracker_3Space line: %s\n",line);
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		}

		// Make sure there's room for a new tracker
		if (num_trackers >= MAX_TRACKERS) {
		  fprintf(stderr,"Too many trackers in config file");
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		}

		// Open the tracker
		if (verbose) printf(
		    "Opening vrpn_Tracker_3Space: %s on port %s, baud %d\n",
		    s2,s3,i1);
		if ( (trackers[num_trackers] =
		     new vrpn_Tracker_3Space(s2, &connection, s3, i1)) == NULL){
		  fprintf(stderr,"Can't create new vrpn_Tracker_3Space\n");
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		} else {
		  num_trackers++;
		}
	  } else if (isit("vrpn_Tracker_Flock")) {
	    next();
		// Get the arguments (class, tracker_name, sensors, port, baud)
		if (sscanf(pch,"%511s%d%511s%d", s2, 
			   &i1, s3, &i2) != 4) {
		  fprintf(stderr,"Bad vrpn_Tracker_Flock line: %s\n",line);
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		}

		// Make sure there's room for a new tracker
		if (num_trackers >= MAX_TRACKERS) {
		  fprintf(stderr,"Too many trackers in config file");
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		}

		// Open the tracker
		if (verbose) printf(
		    "Opening vrpn_Tracker_Flock: %s (%d sensors, on port %s, baud %d)\n",
		    s2, i1, s3,i2);
		if ( (trackers[num_trackers] =
		     new vrpn_Tracker_Flock(s2,&connection,i1,s3,i2)) == NULL){
		  fprintf(stderr,"Can't create new vrpn_Tracker_Flock\n");
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		} else {
		  num_trackers++;
		}
	  } else if (isit("vrpn_Tracker_Flock_Parallel")) {
	    next();
	    // Get the arguments (class, tracker_name, sensors, port, baud, 
	    // and parallel sensor ports )
	    
	    if (sscanf(pch,"%511s%d%511s%d", s2, 
		       &i1, s3, &i2) != 4) {
	      fprintf(stderr,"Bad vrpn_Tracker_Flock_Parallel line: %s\n",line);
	      if (bail_on_error) { return -1; }
	      else { continue; }	// Skip this line
	    }

	    // set up strtok to get the variable num of port names
	    char rgch[24];
	    sprintf(rgch, "%d", i2);
	    char *pch2 = strstr(pch, rgch); 
	    strtok(pch2," \t");
	    fprintf(stderr,"%s",pch2);
	    // pch points to baud, next strtok will give first port name
	    
	    char *rgs[MAX_SENSORS];
	    // get sensor ports
	    for (int iSlaves=0;iSlaves<i1;iSlaves++) {
	      rgs[iSlaves]=new char[512];
	      if (!(pch2 = strtok(0," \t"))) {
		fprintf(stderr,"Bad vrpn_Tracker_Flock_Parallel line: %s\n",line);
		return -1;
	      } else {
		sscanf(pch2,"%511s", rgs[iSlaves]);
	      }
	    }

	    // Make sure there's room for a new tracker
	    if (num_trackers >= MAX_TRACKERS) {
	      fprintf(stderr,"Too many trackers in config file");
	      if (bail_on_error) { return -1; }
	      else { continue; }	// Skip this line
	    }
	    
	    // Open the tracker
	    if (verbose) printf(
				"Opening vrpn_Tracker_Flock_Parallel: %s (%d sensors, on port %s, baud %d)\n",
		    s2, i1, s3,i2);
	    if ( (trackers[num_trackers] =
		  new vrpn_Tracker_Flock_Parallel(s2,&connection,i1,s3,i2,
						  rgs)) == NULL){
	      fprintf(stderr,"Can't create new vrpn_Tracker_Flock_Parallel\n");
	      if (bail_on_error) { return -1; }
	      else { continue; }	// Skip this line
	    } else {
	      num_trackers++;
	    }
	  } else if (isit("vrpn_Tracker_NULL")) {
	    next();
		// Get the arguments (class, tracker_name, sensors, rate)
		if (sscanf(pch,"%511s%d%g",s2,&i1,&f1) != 3) {
		  fprintf(stderr,"Bad vrpn_Tracker_NULL line: %s\n",line);
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		}

		// Make sure there's room for a new tracker
		if (num_trackers >= MAX_TRACKERS) {
		  fprintf(stderr,"Too many trackers in config file");
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		}

		// Open the tracker
		if (verbose) printf(
		    "Opening vrpn_Tracker_NULL: %s with %d sensors, rate %f\n",
		    s2,i1,f1);
		if ( (trackers[num_trackers] =
		     new vrpn_Tracker_NULL(s2, &connection, i1, f1)) == NULL){
		  fprintf(stderr,"Can't create new vrpn_Tracker_NULL\n");
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		} else {
		  num_trackers++;
		}

	  } else if (isit("vrpn_Button_Python")) {
	    next();
		// Get the arguments (class, button_name, portno)
		if (sscanf(pch,"%511s%d",s2,&i1) != 2) {
		  fprintf(stderr,"Bad vrpn_Button_Python line: %s\n",line);
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		}

		// Make sure there's room for a new button
		if (num_buttons >= MAX_BUTTONS) {
		  fprintf(stderr,"Too many buttons in config file");
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		}

		// Open the button
		if (verbose) printf(
		    "Opening vrpn_Button_Python: %s on port %d\n", s2,i1);
		if ( (buttons[num_buttons] =
		     new vrpn_Button_Python(s2, &connection, i1)) == NULL){
		  fprintf(stderr,"Can't create new vrpn_Button_Python\n");
		  if (bail_on_error) { return -1; }
		  else { continue; }	// Skip this line
		} else {
		  num_buttons++;
		}

	  } else {	// Never heard of it
		sscanf(line,"%511s",s1);	// Find out the class name
		fprintf(stderr,"Unknown class: %s\n",s1);
		if (bail_on_error) { return -1; }
		else { continue; }	// Skip this line
	  }
	}
      }

	// Close the configuration file
	fclose(config_file);
	loop = 0;
	// Loop forever calling the mainloop()s for all devices
	while (1) {
		int	i;

		// Let all the buttons generate reports
		for (i = 0; i < num_buttons; i++) {
			buttons[i]->mainloop();
		}

		// Let all the trackers generate reports
		for (i = 0; i < num_trackers; i++) {
			trackers[i]->mainloop();
		}

		// Let all the sound servers do their thing 
		for (i = 0; i < num_sounds; i++) {
			sounds[i]->mainloop();
		}

		// Send and receive all messages
		connection.mainloop();
	}
}





