#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "vrpn_Connection.h"
#include "vrpn_Button.h"
#include "vrpn_Tracker.h"
#include "vrpn_3Space.h"

static	int	MAX_TRACKERS = 100;
static	int	MAX_BUTTONS = 100;

void	Usage(char *s)
{
  fprintf(stderr,"Usage: %s [-f filename] [-warn] [-v]\n",s);
  fprintf(stderr,"       -f: Full path to config file (default vrpn.cfg)\n");
  fprintf(stderr,"       -warn: Only warn on errors (default is to bail)\n");
  fprintf(stderr,"       -v: Verbose\n");
  exit(-1);
}

main (unsigned argc, char *argv[])
{
	char	*config_file_name = "vrpn.cfg";
	FILE	*config_file;
	int	bail_on_error = 1;
	int	verbose = 0;
	int	realparams = 0;
	int	i;

	vrpn_Connection	connection;
	vrpn_Tracker	*trackers[MAX_TRACKERS];
	int		num_trackers = 0;
	vrpn_Button	*buttons[MAX_BUTTONS];
	int		num_buttons = 0;

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
	char	s1[512],s2[512],s3[512];	// String parameters
	int	i1;				// Integer parameters
	float	f1;				// Float parameters

	// Read lines from the file until we run out
	while ( fgets(line, sizeof(line), config_file) != NULL ) {

	  // Make sure the line wasn't too long
	  if (strlen(line) >= sizeof(line)-1) {
		fprintf(stderr,"Line too long in config file: %s\n",line);
		if (bail_on_error) { return -1; }
		else { continue; }	// Skip this line
	  }

	  // Figure out the device from the name and handle appropriately
	  #define isit(s) !strncmp(line,s,strlen(s))
	  if (isit("vrpn_Tracker_3Space")) {

		// Get the arguments (class, tracker_name, port, baud)
		if (sscanf(line,"%511s%511s%511s%d",s1,s2,s3,&i1) != 4) {
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
	  } else if (isit("vrpn_Tracker_NULL")) {

		// Get the arguments (class, tracker_name, sensors, rate)
		if (sscanf(line,"%511s%511s%d%g",s1,s2,&i1,&f1) != 4) {
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

		// Get the arguments (class, button_name, portno)
		if (sscanf(line,"%511s%511s%d",s1,s2,&i1) != 3) {
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

		// Send and receive all messages
		connection.mainloop();
	}
}
