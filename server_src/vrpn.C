#ifdef	sgi
#define	SGI_BDBOX
#endif
// SGI_BDBOX is a SGI Button & dial BOX connected to an SGI.
// This is also refered to a 'special sgibox' in the code.
// The alternative is SGI BDBOX connected to a linix PC.
// confusing eh?


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
#include "vrpn_raw_sgibox.h" // for access to the SGI button & dial box connected to the serial port of an linix PC
#ifdef	SGI_BDBOX
#include "vrpn_sgibox.h" //for access to the B&D box connected to an SGI via the IRIX GL drivers
#endif

#include "vrpn_Analog.h"
#include "vrpn_Joystick.h"
#include "vrpn_JoyFly.h"
#include "vrpn_CerealBox.h"
#include "vrpn_Tracker_AnalogFly.h"

#include "vrpn_ForwarderController.h"

#define MAX_TRACKERS 100
#define MAX_BUTTONS 100
#define MAX_SOUNDS 2
#define MAX_ANALOG 4
#define	MAX_SGIBOX 2
#define	MAX_CEREALS 8
#define MAX_DIALS 8

#define CHECK(s) \
    retval = (s)(pch, line, config_file); \
    if (retval && bail_on_error) \
      return -1; \
    else \
      continue

#define next() pch += strlen(pch) + 1

void Usage (const char * s)
{
  fprintf(stderr,"Usage: %s [-f filename] [-warn] [-v] [port] [-q]\n",s);
  fprintf(stderr,"       [-client machinename port] [-millisleep n]\n");
  fprintf(stderr,"       [-NIC name] [-l filename mode]\n");
  fprintf(stderr,"       -f: Full path to config file (default vrpn.cfg).\n");
  fprintf(stderr,"       -warn: Only warn on errors (default is to bail).\n");
  fprintf(stderr,"       -v: Verbose.\n");
  fprintf(stderr,"	 -q: Quit when last connection is dropped.\n");
  fprintf(stderr,"       -client: Where server connects when it starts up.\n");
  fprintf(stderr,"       -millisleep: Sleep n milliseconds each loop cycle.\n");
  fprintf(stderr,"       -NIC: Use NIC with given IP address or DNS name.\n");
  fprintf(stderr,"       -l: Log to given filename with mode i, o, or io.\n");
  exit(-1);
}

static char * g_NICname = NULL;

static const char * g_logName = NULL;
static int g_logMode = 0;

vrpn_Tracker	* trackers [MAX_TRACKERS];
int		num_trackers = 0;
vrpn_Button	* buttons [MAX_BUTTONS];
int		num_buttons = 0;
vrpn_Sound	* sounds [MAX_SOUNDS];
int		num_sounds = 0;
vrpn_Analog	* analogs [MAX_ANALOG];
int		num_analogs = 0;
vrpn_raw_SGIBox	* sgiboxes [MAX_SGIBOX];
int		num_sgiboxes = 0;
vrpn_CerealBox	* cereals [MAX_CEREALS];
int		num_cereals = 0;
vrpn_Dial	* dials [MAX_DIALS];
int		num_dials = 0;

vrpn_Connection * connection;

#ifdef	SGI_BDBOX
vrpn_SGIBox	* vrpn_special_sgibox;
#endif

// TCH October 1998
// Use Forwarder as remote-controlled multiple connections.
vrpn_Forwarder_Server * forwarderServer;

int	verbose = 1;

// install a signal handler to shut down the trackers and buttons
#ifndef WIN32
#include <signal.h>
void closeDevices();
//#ifdef sgi
//void sighandler( ... )
//#else
void sighandler (int)
//#endif
{
    closeDevices();
    delete connection;
    //return;
    exit(0);
}
#endif

void closeDevices (void) {
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
  //exit(0);
}

int handle_dlc (void *, vrpn_HANDLERPARAM p)
{
    closeDevices();
    delete connection;
    exit(0);
    return 0;
}

void shutDown (void)
{
    closeDevices();
    delete connection;
    exit(0);
    return;
}

// This function will read one line of the vrpn_AnalogFly configuration (matching
// one axis) and fill in the data for that axis. The axis name, the file to read
// from, and the axis to fill in are passed as parameters. It returns 0 on success
// and -1 on failure.

int	get_AFline(FILE *config_file, char *axis_name, vrpn_TAF_axis *axis)
{
	char	line[512];
	char	_axis_name[512];
	char	*name = new char[512];	// We need this to stay around for the param
	int	channel;
	float	thresh, power, scale;

	// Read in the line
	if (fgets(line, sizeof(line), config_file) == NULL) {
		perror("AnalogFly Axis: Can't read axis");
		return -1;
	}

	// Get the values from the line
	if (sscanf(line, "%511s%511s%d%g%g%g", _axis_name, name,
			&channel, &thresh,&scale,&power) != 6) {
		fprintf(stderr,"AnalogFly Axis: Bad axis line\n");
		return -1;
	}

	// Check to make sure the name of the line matches
	if (strcmp(_axis_name, axis_name) != 0) {
		fprintf(stderr,"AnalogFly Axis: wrong axis: wanted %s, got %s)\n",
			axis_name, name);
		return -1;
	}

	// Fill in the values if we didn't get the name "NULL". Otherwise, just
	// leave them as they are, and they will have no effect.
	if (strcmp(name,"NULL") != 0) {
		axis->name = name;
		axis->channel = channel;
		axis->thresh = thresh;
		axis->scale = scale;
		axis->power = power;
	}

	return 0;
}

// setup_raw_SGIBox
// uses globals:  num_sgiboxes, sgiboxes[], verbose
// imports from main:  pch
// returns nonzero on error

int setup_raw_SGIBox (char * & pch, char * line, FILE * config_file) {

  char s2 [512], s3 [512];

        // Line will be: vrpn_raw_SGIBox NAME PORT [list of buttons to toggle]
        int tbutton;    // Button to toggle
        next();
        if (sscanf(pch,"%511s %511s",s2,s3)!=2) {
                fprintf(stderr,"Bad vrpn_raw_SGIBox line: %s\n",line);
                return -1;
        }

        // Make sure there's room for a new raw SGIBox
        if (num_sgiboxes >= MAX_SGIBOX) {
          fprintf(stderr,"Too many raw SGI Boxes in config file");
          return -1;
        }

        // Open the raw SGI box
        if (verbose)
          printf("Opening vrpn_raw_SGIBox %s on serial port %s\n", s2, s3);
        if ( (sgiboxes[num_sgiboxes] =
             new vrpn_raw_SGIBox(s2, connection, s3)) == NULL){
          fprintf(stderr,"Can't create new vrpn_raw_SGIBox\n");
            return -1;
        }
        else {
          num_sgiboxes++;
        }

        //setting listed buttons to toggles instead of default momentary
        //pch=s3;
        pch+=strlen(s2)+1; //advance past the name and port
        pch+=strlen(s3)+1;
        while(sscanf(pch,"%s",s2)==1){
                pch+=strlen(s2)+1;
                tbutton=atoi(s2);       
                // set the button to be a toggle,
                // and set the state of that toggle
                // to 'off'
                sgiboxes[num_sgiboxes - 1]->set_toggle(tbutton,
                                                 vrpn_BUTTON_TOGGLE_OFF); 
                //vrpnButton class will make sure I don't set
                //an invalid button number
                printf("\tButton %d is toggle\n",tbutton);
        }

  return 0;  // successful completion
}

int setup_SGIBOX (char * & pch, char * line, FILE * config_file) {

#ifdef SGI_BDBOX
	    char s2 [512];

            int tbutton;
            next();
            if (sscanf(pch,"%511s",s2)!=1) {
              fprintf(stderr,"Bad vrpn_SGIBOX line: %s\n",line);
              return -1;
            }

                // Open the sgibox
              if (verbose) printf("Opening vrpn_SGIBOX on host %s\n", s2);
                if ( (vrpn_special_sgibox =
                  new vrpn_SGIBox(s2, connection)) == NULL){
                  fprintf(stderr,"Can't create new vrpn_SGIBox\n");
                  return -1;
                } 
                
                //setting listed buttons to toggles instead of default momentary
                pch+=strlen(s2)+1;
                while(sscanf(pch,"%s",s2)==1){
                        pch+=strlen(s2)+1;
                        tbutton=atoi(s2);       
                        vrpn_special_sgibox->set_toggle(tbutton,
                                 vrpn_BUTTON_TOGGLE_OFF);  
                        //vrpnButton class will make sure I don't set
                        //an invalid button number
                        printf("Button %d toggles\n",tbutton);
                }
                printf("Opening vrpn_SGIBOX on host %s done\n", s2);

#else
                fprintf(stderr,"vrpn_server: Can't open SGIbox: not an SGI!\n");
#endif


  return 0;  // successful completion
}

int setup_JoyFly (char * & pch, char * line, FILE * config_file) {
  char s2 [512], s3 [512], s4 [512];

            next();
            if (sscanf(pch, "%511s%511s%511s",s2,s3,s4) != 3) {
              fprintf(stderr, "Bad vrpn_JoyFly line: %s\n", line);
              return -1;
            }

#ifdef  _WIN32
            fprintf(stderr,"JoyFly tracker not yet defined for NT\n");
#else
            // Make sure there's room for a new tracker
                if (num_trackers >= MAX_TRACKERS) {
                  fprintf(stderr,"Too many trackers in config file");
                  return -1;
                }

                // Open the tracker
                if (verbose)
                  printf("Opening vrpn_Tracker_JoyFly:  "
                         "%s on server %s with config_file %s\n",
                         s2,s3,s4);

                // HACK HACK HACK
                // Check for illegal character leading '*' to see if it's local

                if (s3[0] == '*') 
                  trackers[num_trackers] =
                    new vrpn_Tracker_JoyFly (s2, connection, &s3[1], s4,
                                             connection);
                else
                  trackers[num_trackers] =
                    new vrpn_Tracker_JoyFly (s2, connection, s3, s4);

                if (!trackers[num_trackers]) {
                  fprintf(stderr,"Can't create new vrpn_Tracker_JoyFly\n");
                  return -1;
                } else {
                  num_trackers++;
                }
#endif

  return 0;
}

int setup_Tracker_AnalogFly (char * & pch, char * line, FILE * config_file) {
  char s2 [512], s3 [512];
  int i1;
  float f1;

                vrpn_Tracker_AnalogFlyParam     p;

                next();
                if (sscanf(pch, "%511s%g",s2,&f1) != 2) {
                        fprintf(stderr, "Bad vrpn_Tracker_AnalogFly line: %s\n",
 line);
                        return -1;
                }

                // Make sure there's room for a new tracker
                if (num_trackers >= MAX_TRACKERS) {
                  fprintf(stderr,"Too many trackers in config file");
                  return -1;
                }

                // Open the tracker
                if (verbose) {
                  printf("Opening vrpn_Tracker_AnalogFly: "
                         "%s with update rate %g\n",s2,f1);
                }

                // Scan the following lines in the configuration file to fill
                // in the start-up parameters for the different axis.

                if (get_AFline(config_file,"X", &p.x)) {
                        fprintf(stderr,"Can't read X line for AnalogFly\n");
                        return -1;
                }
                
                if (get_AFline(config_file,"Y", &p.y)) {
                        fprintf(stderr,"Can't read Y line for AnalogFly\n");
                        return -1;
                }

                if (get_AFline(config_file,"Z", &p.z)) {
                        fprintf(stderr,"Can't read Z line for AnalogFly\n");
                        return -1;
                }

                if (get_AFline(config_file,"RX", &p.sx)) {
                        fprintf(stderr,"Can't read RX line for AnalogFly\n");
                        return -1;
                }

                if (get_AFline(config_file,"RY", &p.sy)) {
                        fprintf(stderr,"Can't read RY line for AnalogFly\n");
                        return -1;
                }

                if (get_AFline(config_file,"RZ", &p.sz)) {
                        fprintf(stderr,"Can't read RZ line for AnalogFly\n");
                        return -1;
                }

        // Read the reset line
        if (fgets(line, sizeof(line), config_file) == NULL) {
                fprintf(stderr,"Ran past end of config file in AnalogFly\n");
                return -1;
        }
        if (sscanf(line, "RESET %511s%d", s3, &i1) != 2) {
                fprintf(stderr,"Bad RESET line in AnalogFly\n");
                return -1;
        }
        if (strcmp(s3,"NULL") != 0) {
                p.reset_name = s3;
                p.reset_which = i1;
        }

        trackers[num_trackers] = new
           vrpn_Tracker_AnalogFly (s2, connection, &p, f1);

                if (!trackers[num_trackers]) {
                  fprintf(stderr,"Can't create new vrpn_Tracker_AnalogFly\n");
                  return -1;
                } else {
                  num_trackers++;
                }
  return 0;
}

int setup_Joystick (char * & pch, char * line, FILE * config_file) {
  char s2 [512], s3 [512];
  int i1;

    float fhz;
    // Get the arguments
    next();
    if (sscanf(pch, "%511s%511s%d %f", s2, s3, &i1, &fhz) != 4) {
      fprintf(stderr, "Bad vrpn_Joystick line: %s\n", line);
      return -1;
    }

#ifdef  _WIN32
    fprintf(stderr,"Joystick not yet defined for NT\n");
#else

    // Make sure there's room for a new joystick server
    if (num_analogs >= MAX_ANALOG) {
      fprintf(stderr, "Too many analog devices in config file");
      return -1;
    }

    // Open the sound server
    if (verbose) 
      printf("Opening vrpn_Joystick:  "
             "%s on port %s baud %d, min update rate = %.2f\n", 
             s2,s3, i1, fhz);
    if ((analogs[num_analogs] =
          new vrpn_Joystick(s2, connection,s3, i1, fhz)) == NULL) {
        fprintf(stderr, "Can't create new vrpn_Joystick\n");
        return -1;
    } else {
        num_analogs++;
    }
#endif

  return 0;
}

int setup_DialExample (char * & pch, char * line, FILE * config_file) {
  char s2 [512];
  int i1;
  float f1,f2;

  next();

  // Get the arguments (class, dial_name, dials, spin_rate, update_rate)
  if (sscanf(pch,"%511s%d%g%g",s2,&i1,&f1,&f2) != 4) {
    fprintf(stderr,"Bad vrpn_Dial_Example line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new dial
  if (num_dials >= MAX_DIALS) {
    fprintf(stderr,"Too many dials in config file");
    return -1;
  }

  // Open the dial
  if (verbose) printf(
      "Opening vrpn_Dial_Example: %s with %d sensors, spinrate %f, update %f\n",
      s2,i1,f1,f2);
  if ( (dials[num_dials] =
       new vrpn_Dial_Example_Server(s2, connection, i1, f1, f2)) == NULL){
    fprintf(stderr,"Can't create new vrpn_Dial_Example\n");
    return -1;
  } else {
    num_dials++;
  }

  return 0;
}

int setup_CerealBox (char * & pch, char * line, FILE * config_file) {
  char s2 [512], s3 [512];
  int i1, i2, i3, i4;

            next();
            // Get the arguments (class, serialbox_name, port, baud, numdig,
            // numana, numenc)
            if (sscanf(pch,"%511s%511s%d%d%d%d",s2,s3, &i1, &i2, &i3, &i4) != 6)
 {
              fprintf(stderr,"Bad vrpn_Cereal line: %s\n",line);
              return -1;
            }

            // Make sure there's room for a new box
            if (num_cereals >= MAX_CEREALS) {
              fprintf(stderr,"Too many Cereal boxes in config file");
              return -1;
            }

            // Open the box
            if (verbose) 
              printf("Opening vrpn_Cereal: %s on port %s, baud %d, %d dig, "
                     " %d ana, %d enc\n",
                    s2,s3,i1, i2,i3,i4);
            if ((cereals[num_cereals] =
                  new vrpn_CerealBox(s2, connection, s3, i1, i2, i3, i4))
                       == NULL)               
              {
                fprintf(stderr,"Can't create new vrpn_CerealBox\n");
                return -1;
              } else {
                num_cereals++;
              }

  return 0;
}

int setup_Tracker_Dyna (char * & pch, char * line, FILE * config_file) {

  char s2 [512], s3 [512];
  int i1, i2;
  int ret;

            next();
            // Get the arguments (class, tracker_name, sensors, port, baud)
            if ( (ret=sscanf(pch,"%511s%d%511s%d",s2,&i2, s3, &i1)) != 4) {
              fprintf(stderr,"Bad vrpn_Tracker_Dyna line (ret %d): %s\n",ret,
		line);
              return -1;
            }

            // Make sure there's room for a new tracker
            if (num_trackers >= MAX_TRACKERS) {
              fprintf(stderr,"Too many trackers in config file");
              return -1;
            }

            // Open the tracker
            if (verbose) 
              printf("Opening vrpn_Tracker_Dyna: %s on port %s, baud %d, "
                     "%d sensors\n",
                    s2,s3,i1, i2);
            if ((trackers[num_trackers] =
                  new vrpn_Tracker_Dyna(s2, connection, i2, s3, i1)) == NULL)   
            
              {
                fprintf(stderr,"Can't create new vrpn_Tracker_Dyna\n");
                return -1;
              } else {
                num_trackers++;
              }

  return 0;
}

int setup_Tracker_Fastrak (char * & pch, char * line, FILE * config_file) {

  char s2 [512], s3 [512];
  int i1;

        char    rcmd[5000];     // Reset command to send to Fastrak
        next();
        // Get the arguments (class, tracker_name, port, baud)
        if (sscanf(pch,"%511s%511s%d",s2,s3,&i1) != 3) {
          fprintf(stderr,"Bad vrpn_Tracker_Fastrak line: %s\n%s %s\n",
                  line, pch, s3);
          return -1;
        }

                // Make sure there's room for a new tracker
                if (num_trackers >= MAX_TRACKERS) {
                  fprintf(stderr,"Too many trackers in config file");
                  return -1;
                }

        // If the last character in the line is a backslash, '\', then
        // the following line is an additional command to send to the
        // Fastrak at reset time. So long as we find lines with slashes
        // at the ends, we add them to the command string to send. Note
        // that there is a newline at the end of the line, following the
        // backslash.
        sprintf(rcmd, "");
        while (line[strlen(line)-2] == '\\') {
          // Read the next line
          if (fgets(line, sizeof(line), config_file) == NULL) {
              fprintf(stderr,"Ran past end of config file in Fastrak\n");
                  return -1;
          }

          // Copy the line into the remote command,
          // then replace \ with \015 if present
          // In any case, make sure we terminate with \015.
          strncat(rcmd, line, sizeof(line));
          if (rcmd[strlen(rcmd)-2] == '\\') {
                  rcmd[strlen(rcmd)-2] = '\015';
                  rcmd[strlen(rcmd)-1] = '\0';
          } else if (rcmd[strlen(rcmd)-1] == '\n') {
                  rcmd[strlen(rcmd)-1] = '\015';
          } else {        // Add one, we reached the EOF before CR
                  rcmd[strlen(rcmd)+1] = '\0';
                  rcmd[strlen(rcmd)] = '\015';
          }
        }
        if (strlen(rcmd) > 0) {
                printf("... additional reset commands follow:\n");
                printf("%s\n",rcmd);
        }

        // Open the tracker
        if (verbose) printf(
            "Opening vrpn_Tracker_Fastrak: %s on port %s, baud %d\n",
            s2,s3,i1);

#if defined(sgi) || defined(linux) || defined(WIN32)

        if ( (trackers[num_trackers] =
             new vrpn_Tracker_Fastrak(s2, connection, s3, i1, 1, 4, rcmd))
                            == NULL){

#endif

          fprintf(stderr,"Can't create new vrpn_Tracker_Fastrak\n");
          return -1;

#if defined(sgi) || defined(linux) || defined(WIN32)

        } else {
          num_trackers++;
        }

#endif

  return 0;
}

int setup_Tracker_3Space (char * & pch, char * line, FILE * config_file) {

  char s2 [512], s3 [512];
  int i1;

            next();
                // Get the arguments (class, tracker_name, port, baud)
                if (sscanf(pch,"%511s%511s%d",s2,s3,&i1) != 3) {
                  fprintf(stderr,"Bad vrpn_Tracker_3Space line: %s\n",line);
                  return -1;
                }

                // Make sure there's room for a new tracker
                if (num_trackers >= MAX_TRACKERS) {
                  fprintf(stderr,"Too many trackers in config file");
                  return -1;
                }

                // Open the tracker
                if (verbose) printf(
                    "Opening vrpn_Tracker_3Space: %s on port %s, baud %d\n",
                    s2,s3,i1);
                if ( (trackers[num_trackers] =
                     new vrpn_Tracker_3Space(s2, connection, s3, i1)) == NULL){
                  fprintf(stderr,"Can't create new vrpn_Tracker_3Space\n");
                  return -1;
                } else {
                  num_trackers++;
                }



  return 0;
}

int setup_Tracker_Flock (char * & pch, char * line, FILE * config_file) {

  char s2 [512], s3 [512];
  int i1, i2;

            next();
                // Get the arguments (class, tracker_name, sensors, port, baud)
                if (sscanf(pch,"%511s%d%511s%d", s2, 
                           &i1, s3, &i2) != 4) {
                  fprintf(stderr,"Bad vrpn_Tracker_Flock line: %s\n",line);
                  return -1;
                }

                // Make sure there's room for a new tracker
                if (num_trackers >= MAX_TRACKERS) {
                  fprintf(stderr,"Too many trackers in config file");
                  return -1;
                }

        // Open the tracker
        if (verbose) printf("Opening vrpn_Tracker_Flock: "
                            "%s (%d sensors, on port %s, baud %d)\n",
                    s2, i1, s3,i2);
        if ( (trackers[num_trackers] =
             new vrpn_Tracker_Flock(s2,connection,i1,s3,i2)) == NULL){
          fprintf(stderr,"Can't create new vrpn_Tracker_Flock\n");
          return -1;
        } else {
          num_trackers++;
        }

  return 0;

}

int setup_Tracker_Flock_Parallel (char * & pch, char * line, FILE * config_file) {

  char s2 [512], s3 [512];
  int i1, i2;

            next();
            // Get the arguments (class, tracker_name, sensors, port, baud, 
            // and parallel sensor ports )
            
            if (sscanf(pch,"%511s%d%511s%d", s2, 
                       &i1, s3, &i2) != 4) {
              fprintf(stderr,"Bad vrpn_Tracker_Flock_Parallel line: %s\n",line);
              return -1;
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
                fprintf(stderr,"Bad vrpn_Tracker_Flock_Parallel line: %s\n",
                    line);
                return -1;
              } else {
                sscanf(pch2,"%511s", rgs[iSlaves]);
              }
            }

            // Make sure there's room for a new tracker
            if (num_trackers >= MAX_TRACKERS) {
              fprintf(stderr,"Too many trackers in config file");
              return -1;
            }
            
            // Open the tracker
            if (verbose)
              printf("Opening vrpn_Tracker_Flock_Parallel: "
                     "%s (%d sensors, on port %s, baud %d)\n",
                    s2, i1, s3,i2);
            if ( (trackers[num_trackers] =
                  new vrpn_Tracker_Flock_Parallel(s2,connection,i1,s3,i2,
                                                  rgs)) == NULL){
              fprintf(stderr,"Can't create new vrpn_Tracker_Flock_Parallel\n");
              return -1;
            } else {
              num_trackers++;
            }

  return 0;
}

int setup_Tracker_NULL (char * & pch, char * line, FILE * config_file) {

  char s2 [512];
  int i1;
  float f1;

            next();
                // Get the arguments (class, tracker_name, sensors, rate)
                if (sscanf(pch,"%511s%d%g",s2,&i1,&f1) != 3) {
                  fprintf(stderr,"Bad vrpn_Tracker_NULL line: %s\n",line);
                  return -1;
                }

                // Make sure there's room for a new tracker
                if (num_trackers >= MAX_TRACKERS) {
                  fprintf(stderr,"Too many trackers in config file");
                  return -1;
                }

                // Open the tracker
                if (verbose) printf(
                    "Opening vrpn_Tracker_NULL: %s with %d sensors, rate %f\n",
                    s2,i1,f1);
                if ( (trackers[num_trackers] =
                     new vrpn_Tracker_NULL(s2, connection, i1, f1)) == NULL){
                  fprintf(stderr,"Can't create new vrpn_Tracker_NULL\n");
                  return -1;
                } else {
                  num_trackers++;
                }

  return 0;
}

int setup_Button_Python (char * & pch, char * line, FILE * config_file) {

  char s2 [512];
  int i1;

            next();
                // Get the arguments (class, button_name, portno)
                if (sscanf(pch,"%511s%d",s2,&i1) != 2) {
                  fprintf(stderr,"Bad vrpn_Button_Python line: %s\n",line);
                  return -1;
                }

                // Make sure there's room for a new button
                if (num_buttons >= MAX_BUTTONS) {
                  fprintf(stderr,"Too many buttons in config file");
                  return -1;
                }

                // Open the button
                if (verbose) printf(
                    "Opening vrpn_Button_Python: %s on port %d\n", s2,i1);
                if ( (buttons[num_buttons] =
                     new vrpn_Button_Python(s2, connection, i1)) == NULL){
                  fprintf(stderr,"Can't create new vrpn_Button_Python\n");
                  return -1;
                } else {
                  num_buttons++;
                }

  return 0;

}

//================================
int setup_Button_PinchGlove(char* &pch, char *line, FILE *config_file) {

   char name[512], port[512];
   int baud;

   next();
   // Get the arguments (class, button_name, port, baud)
   if (sscanf(pch,"%511s%511s%d", name, port, &baud) != 3) {
      fprintf(stderr,"Bad vrpn_Button_PinchGlove line: %s\n",line);
      return -1;
    }

   // Make sure there's room for a new button
   if (num_buttons >= MAX_BUTTONS) {
      fprintf(stderr,"vrpn_Button_PinchGlove: Too many buttons in config file");
      return -1;
   }

   // Open the button
   if (verbose)   
      printf("Opening vrpn_Button_PinchGlove: %s on port %s at %d baud\n",name,port,baud);
   if ( (buttons[num_buttons] = new vrpn_Button_PinchGlove(name,connection,port,baud)) 
         == NULL ) {
      fprintf(stderr,"Can't create new vrpn_Button_PinchGlove\n");
      return -1;
   } else 
      num_buttons++;
  
   return 0;

}

main (int argc, char * argv[])
{
	char	* config_file_name = "vrpn.cfg";
	FILE	* config_file;
	char 	* client_name = NULL;
	int	client_port;
	int	bail_on_error = 1;
	int	auto_quit = 0;
	int	realparams = 0;
	int	i;
	int 	loop = 0;
	int	port = vrpn_DEFAULT_LISTEN_PORT_NO;
	int	milli_sleep_time = 0;		// How long to sleep each iteration?
#ifdef WIN32
	WSADATA wsaData; 
	int status;
	if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) {
	  fprintf(stderr, "WSAStartup failed with %d\n", status);
	  return(1);
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

	// Parse the command line
	i = 1;
	while (i < argc) {
	  if (!strcmp(argv[i], "-f")) {		// Specify config-file name
		if (++i > argc) { Usage(argv[0]); }
		config_file_name = argv[i];
	  } else if (!strcmp(argv[i], "-millisleep")) {	// How long to sleep each loop?
		if (++i > argc) { Usage(argv[0]); }
		milli_sleep_time = atoi(argv[i]);
	  } else if (!strcmp(argv[i], "-warn")) {// Don't bail on errors
		bail_on_error = 0;
	  } else if (!strcmp(argv[i], "-v")) {	// Verbose
		verbose = 1;
	  } else if (!strcmp(argv[i], "-q")) {  // quit on dropped last con
		auto_quit = 1;
	  } else if (!strcmp(argv[i], "-client")) { // specify a waiting client
		if (++i > argc) { Usage(argv[0]); }
		client_name = argv[i];
		if (++i > argc) { Usage(argv[0]); }
		client_port = atoi(argv[i]);
	  } else if (!strcmp(argv[i], "-NIC")) { // specify a network interface
            if (++i > argc) { Usage(argv[0]); }
            fprintf(stderr, "Listening on network interface card %s.\n",
                    argv[i]);
            g_NICname = argv[i];
          } else if (!strcmp(argv[i], "-l")) { // specify server-side logging
            if (++i > argc) { Usage(argv[0]); }
            fprintf(stderr, "Base logfile name %s.\n", argv[i]);
            g_logName = argv[i];
            if (++i > argc) { Usage(argv[0]); }
            if (strchr(argv[i], 'i')) { g_logMode |= vrpn_LOG_INCOMING; }
            if (strchr(argv[i], 'o')) { g_logMode |= vrpn_LOG_OUTGOING; }
            fprintf(stderr, "Log mode %s: %d.\n", argv[i], g_logMode);
	  } else if (argv[i][0] == '-') {	// Unknown flag
		Usage(argv[0]);
	  } else switch (realparams) {		// Non-flag parameters
	    case 0:
		port = atoi(argv[i]);
		realparams++;
		break;
	    default:
		Usage(argv[0]);
	  }
	  i++;
	}


	// Need to have a global pointer to it so we can shut it down
	// in the signal handler (so we can close any open logfiles.)
	//vrpn_Synchronized_Connection	connection;
	connection = new vrpn_Synchronized_Connection
             (port, g_logName, g_logMode, g_NICname);
	
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
	char	s1[512];
        int retval;

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

          // Rewritten to move all this code out-of-line by Tom Hudson
          // August 99.  We could even make it table-driven now.
          // It seems that we're never going to document this program
          // and that it will allways be necessary to read the code to
          // figure out how to write a config file.  This code rearrangement
          // should make it easier to figure out what the possible tokens
          // are in a config file by listing them close together here
          // instead of hiding them in the middle of functions.

	  if (isit("vrpn_raw_SGIBox")) {
            CHECK(setup_raw_SGIBox);
	  } else if (isit("vrpn_SGIBOX")) {
            CHECK(setup_SGIBOX);
	  } else if (isit("vrpn_JoyFly")) {
            CHECK(setup_JoyFly);
	  } else if (isit("vrpn_Tracker_AnalogFly")) {
            CHECK(setup_Tracker_AnalogFly);
	  } else  if (isit("vrpn_Joystick")) {
            CHECK(setup_Joystick);
	  } else if (isit("vrpn_Dial_Example")) {
            CHECK(setup_DialExample);
	  } else if (isit("vrpn_CerealBox")) {
            CHECK(setup_CerealBox);
	  } else if (isit("vrpn_Tracker_Dyna")) {
            CHECK(setup_Tracker_Dyna);
	  } else if (isit("vrpn_Tracker_Fastrak")) {
            CHECK(setup_Tracker_Fastrak);
	  } else if (isit("vrpn_Tracker_3Space")) {
            CHECK(setup_Tracker_3Space);
	  } else if (isit("vrpn_Tracker_Flock")) {
            CHECK(setup_Tracker_Flock);
	  } else if (isit("vrpn_Tracker_Flock_Parallel")) {
            CHECK(setup_Tracker_Flock_Parallel);
	  } else if (isit("vrpn_Tracker_NULL")) {
            CHECK(setup_Tracker_NULL);
	  } else if (isit("vrpn_Button_Python")) {
            CHECK(setup_Button_Python);
	  } else if (isit("vrpn_Button_PinchGlove")) {
            CHECK(setup_Button_PinchGlove);
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

	// Open the Forwarder Server
        forwarderServer = new vrpn_Forwarder_Server (connection);

	loop = 0;
	if (auto_quit) {
		int dlc_m_id = connection->register_message_type(
					vrpn_dropped_last_connection);
		connection->register_handler(dlc_m_id, handle_dlc, NULL);
	}
	// Loop forever calling the mainloop()s for all devices
#ifdef	SGI_BDBOX
	fprintf(stderr, "sgibox: %p\n", vrpn_special_sgibox);
#endif
	if (client_name) {
	    fprintf(stderr, "vrpn_serv: connecting to client: %s:%d\n",
		client_name, client_port);
	    if (connection->connect_to_client(0, client_name, client_port)){
	 	fprintf(stderr, "server: could not connect to client %s:%d\n",
			client_name, client_port);
		shutDown();
	    }
	}


	// ********************************************************************
	// **                                                                **
	// **                MAIN LOOP                                       **
	// **                                                                **
	// ********************************************************************
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

		// Let all the analogs do their thing
		for (i=0; i< num_analogs; i++) {
			analogs[i]->mainloop();
		}

		// Let all the dials do their thing
		for (i=0; i< num_dials; i++) {
			dials[i]->mainloop();
		}

		// Let all the cereal boxes do their thing
		for (i=0; i< num_cereals; i++) {
			cereals[i]->mainloop();
		}

		// Let all of the SGI button/knob boxes do their thing
		for (i=0; i < num_sgiboxes; i++) {
			sgiboxes[i]->mainloop();
		}
#ifdef SGI_BDBOX
		if (vrpn_special_sgibox) 
		  vrpn_special_sgibox->mainloop();
#endif

		// Send and receive all messages
		connection->mainloop();
		if (!connection->doing_okay()) shutDown();

		// Handle forwarding requests;  send messages
		// on auxiliary connections
		forwarderServer->mainloop();

		// Sleep so we don't eat the CPU
		if (milli_sleep_time > 0) {
			vrpn_SleepMsecs(milli_sleep_time);
		}
	}

	return 0;
}
