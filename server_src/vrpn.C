#ifdef	sgi
#define	SGI_BDBOX
#endif
// SGI_BDBOX is a SGI Button & dial BOX connected to an SGI.
// This is also refered to a 'special sgibox' in the code.
// The alternative is SGI BDBOX connected to a linux PC.
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
#include "vrpn_UNC_Joystick.h"
#include "vrpn_Joylin.h"
#include "vrpn_JoyFly.h"
#include "vrpn_CerealBox.h"
#include "vrpn_Tracker_AnalogFly.h"
#include "vrpn_Tracker_ButtonFly.h"
#include "vrpn_Magellan.h"
#include "vrpn_Spaceball.h"
#include "vrpn_ImmersionBox.h"
#include "vrpn_Analog_Radamec_SPI.h"
#include "vrpn_Zaber.h"
#include "vrpn_Wanda.h"
#include "vrpn_Analog_5dt.h"
#include "vrpn_Tng3.h"
#include "vrpn_Tracker_isense.h"
#include "vrpn_DirectXFFJoystick.h"
#include "vrpn_GlobalHapticsOrb.h"

#include "vrpn_ForwarderController.h"
#include <vrpn_RedundantTransmission.h>

#ifdef INCLUDE_TIMECODE_SERVER
#include "timecode_generator_server\vrpn_timecode_generator.h"
#endif

const int MAX_TRACKERS = 100;
const int MAX_BUTTONS =  100;
const int MAX_SOUNDS =     2;
const int MAX_ANALOG =     4;
const int MAX_SGIBOX =     2;
const int MAX_CEREALS =    8;
const int MAX_MAGELLANS =  8;
const int MAX_SPACEBALLS = 8;
const int MAX_IBOXES =     8;
const int MAX_DIALS =      8;
#ifdef INCLUDE_TIMECODE_SERVER
const int MAX_TIMECODE_GENERATORS = 8;
#endif
const int MAX_TNG3S = 8;
#ifdef	VRPN_USE_DIRECTINPUT
const int MAX_DIRECTXJOYS = 8;
#endif
const int MAX_GLOBALHAPTICSORBS = 8;

static	int	done = 0;	// Done and should exit?

const int LINESIZE = 512;

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
  fprintf(stderr,"       [-NIC name] [-li filename] [-lo filename]\n");
  fprintf(stderr,"       -f: Full path to config file (default vrpn.cfg).\n");
  fprintf(stderr,"       -millisleep: Sleep n milliseconds each loop cycle (default 1).\n");
  fprintf(stderr,"       -warn: Only warn on errors (default is to bail).\n");
  fprintf(stderr,"       -v: Verbose.\n");
  fprintf(stderr,"	 -q: Quit when last connection is dropped.\n");
  fprintf(stderr,"       -client: Where server connects when it starts up.\n");
  fprintf(stderr,"       -NIC: Use NIC with given IP address or DNS name.\n");
  fprintf(stderr,"       -li: Log incoming messages to given filename.\n");
  fprintf(stderr,"       -lo: Log outgoing messages to given filename.\n");
  exit(0);
}

static char * g_NICname = NULL;

static const char * g_inLogName = NULL;
static const char * g_outLogName = NULL;

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
vrpn_Magellan	* magellans [MAX_MAGELLANS];
int		num_magellans = 0;
vrpn_Spaceball  * spaceballs [MAX_SPACEBALLS];
int             num_spaceballs = 0;
vrpn_ImmersionBox  *iboxes[MAX_IBOXES];
int             num_iboxes = 0;
vrpn_Dial	* dials [MAX_DIALS];
int		num_dials = 0;
#ifdef INCLUDE_TIMECODE_SERVER
vrpn_Timecode_Generator * timecode_generators[MAX_TIMECODE_GENERATORS];
int		num_generators = 0;
#endif
vrpn_Tng3       *tng3s[MAX_TNG3S];
int             num_tng3s = 0;
#ifdef	VRPN_USE_DIRECTINPUT
vrpn_DirectXFFJoystick	* DirectXJoys [MAX_DIRECTXJOYS];
int		num_DirectXJoys = 0;
#endif
vrpn_GlobalHapticsOrb *ghos[MAX_GLOBALHAPTICSORBS];
int		num_GlobalHapticsOrbs = 0;

vrpn_Connection * connection;

#ifdef	SGI_BDBOX
vrpn_SGIBox	* vrpn_special_sgibox;
#endif

// TCH October 1998
// Use Forwarder as remote-controlled multiple connections.
vrpn_Forwarder_Server * forwarderServer;

vrpn_RedundantTransmission * redundancy;
vrpn_RedundantController * redundantController;

int	verbose = 1;

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
  //XXX Get the other types of devices too...
  fprintf(stderr, "\nAll devices closed...\n");
}

void shutDown (void)
{
    closeDevices();
    delete connection;
    fprintf(stderr,"Deleted connection, Exiting.\n");
    exit(0);
}

int handle_dlc (void *, vrpn_HANDLERPARAM p)
{
    shutDown();
    return 0;
}

// install a signal handler to shut down the trackers and buttons
#ifndef WIN32
#include <signal.h>
//#ifdef sgi
//void sighandler( ... )
//#else
void sighandler (int)
//#endif
{
	done = 1;
}
#endif

// This function will read one line of the vrpn_AnalogFly configuration (matching
// one axis) and fill in the data for that axis. The axis name, the file to read
// from, and the axis to fill in are passed as parameters. It returns 0 on success
// and -1 on failure.

int	get_AFline(FILE *config_file, char *axis_name, vrpn_TAF_axis *axis)
{
	char	line[LINESIZE];
	char	_axis_name[LINESIZE];
	char	*name = new char[LINESIZE];	// We need this to stay around for the param
	int	channel;
	float	offset, thresh, power, scale;

	// Read in the line
	if (fgets(line, LINESIZE, config_file) == NULL) {
		perror("AnalogFly Axis: Can't read axis");
		return -1;
	}

	// Get the values from the line
	if (sscanf(line, "%511s%511s%d%g%g%g%g", _axis_name, name,
			&channel, &offset, &thresh,&scale,&power) != 7) {
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
		axis->offset = offset;
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

  char s2 [LINESIZE], s3 [LINESIZE];

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

	    char s2 [LINESIZE];

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


int setup_Timecode_Generator (char * & pch, char * line, FILE * config_file) {
#ifdef INCLUDE_TIMECODE_SERVER
	char s2 [LINESIZE];

	next();
	if (sscanf(pch,"%511s",s2)!=1) {
		fprintf(stderr,"Timecode_Generator line: %s\n",line);
		return -1;
	}

	// Make sure there's room for a new generator
    if (num_generators >= MAX_TIMECODE_GENERATORS) {
      fprintf(stderr,"Too many generators in config file");
      return -1;
    }

	// open the timecode generator
	if (verbose) {
		printf("Opening vrpn_Timecode_Generator on host %s\n", s2);
	}
	if ( ( timecode_generators[num_generators] =	new vrpn_Timecode_Generator(s2, connection)) == NULL) {
		fprintf(stderr,"Can't create new vrpn_Timecode_Generator\n");
		return -1;
	} else {
		num_generators++;
	}
	return 0; // successful completion
#else
	fprintf(stderr, "vrpn_server: Can't open Timecode Generator: INCLUDE_TIMECODE_GENERATOR not defined in vrpn_Configure.h!\n");
	return -1;
#endif
}


int setup_JoyFly (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE], s3 [LINESIZE], s4 [LINESIZE];

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
    char s2 [LINESIZE], s3 [LINESIZE];
    int i1;
    float f1;
    vrpn_Tracker_AnalogFlyParam     p;
    vrpn_bool	absolute;

    next();
    if (sscanf(pch, "%511s%g%511s",s2,&f1,s3) != 3) {
            fprintf(stderr, "Bad vrpn_Tracker_AnalogFly line: %s\n",
		line);
            return -1;
    }

    // See if this should be absolute or differential
    if (strcmp(s3, "absolute") == 0) {
	absolute = vrpn_true;
    } else if (strcmp(s3, "differential") == 0) {
	absolute = vrpn_false;
    } else {
	fprintf(stderr,"vrpn_Tracker_AnalogFly: Expected 'absolute' or 'differential'\n");
	fprintf(stderr,"   but got '%s'\n",s3);
	return -1;
    }

    // Make sure there's room for a new tracker
    if (num_trackers >= MAX_TRACKERS) {
      fprintf(stderr,"Too many trackers in config file");
      return -1;
    }

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
    if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr,"Ran past end of config file in AnalogFly\n");
            return -1;
    }
    if (sscanf(line, "RESET %511s%d", s3, &i1) != 2) {
            fprintf(stderr,"Bad RESET line in AnalogFly: %s\n",line);
            return -1;
    }
    if (strcmp(s3,"NULL") != 0) {
            p.reset_name = s3;
            p.reset_which = i1;
    }

    trackers[num_trackers] = new
       vrpn_Tracker_AnalogFly (s2, connection, &p, f1, absolute);

    if (!trackers[num_trackers]) {
      fprintf(stderr,"Can't create new vrpn_Tracker_AnalogFly\n");
      return -1;
    } else {
      num_trackers++;
    }

    return 0;
}

int setup_Tracker_ButtonFly (char * & pch, char * line, FILE * config_file) {
    char s2 [LINESIZE], s3 [LINESIZE];
    float f1;
    vrpn_Tracker_ButtonFlyParam     p;

    next();
    if (sscanf(pch, "%511s%g",s2,&f1) != 2) {
            fprintf(stderr, "Bad vrpn_Tracker_ButtonFly line: %s\n", line);
            return -1;
    }

    // Make sure there's room for a new tracker
    if (num_trackers >= MAX_TRACKERS) {
      fprintf(stderr,"Too many trackers in config file");
      return -1;
    }

    if (verbose) {
      printf("Opening vrpn_Tracker_ButtonFly "
             "%s with update rate %g\n",s2,f1);
    }

    // Scan the following lines in the configuration file to fill
    // in the start-up parameters for the different axes and the
    // analog velocity and rotational velocity setters.  The last
    // line is "end".

    while (1) {
      if (fgets(line, LINESIZE, config_file) == NULL) {
	fprintf(stderr,"Ran past end of config file in ButtonFly\n");
	return -1;
      }
      if (sscanf(line, "%511s", s3) != 1) {
	fprintf(stderr,"Bad line in config file in ButtonFly\n");
	return -1;
      }
      if (strcmp(s3, "end") == 0) {
	break;	// Done with reading the parameters
      } else if (strcmp(s3, "absolute") == 0) {
	char  name[200];	//< Name of the button device to read from
	int   which;		//< Which button to read from
	float x,y,z, rx,ry,rz;	//< Position and orientation
	vrpn_TBF_axis axis;	//< Axis to add to the ButtonFly

	if (sscanf(line, "absolute %199s%d%g%g%g%g%g%g", name,
		&which, &x,&y,&z, &rx,&ry,&rz) != 8) {
	  fprintf(stderr,"ButtonFly: Bad absolute line\n");
	  return -1;
	}

	float vec[3], rot[3];
	vec[0] = x; vec[1] = y; vec[2] = z;
	rot[0] = rx; rot[1] = ry; rot[2] = rz;
	axis.absolute = true;
	memcpy(axis.name, name, sizeof(axis.name));
	axis.channel = which;
	memcpy(axis.vec, vec, sizeof(axis.vec));
	memcpy(axis.rot, rot, sizeof(axis.rot));

	if (!p.add_axis(axis)) {
	  fprintf(stderr,"ButtonFly: Could not add absolute axis to parameters\n");
	  return -1;
	}
      } else if (strcmp(s3, "differential") == 0) {
	char  name[200];	//< Name of the button device to read from
	int   which;		//< Which button to read from
	float x,y,z, rx,ry,rz;	//< Position and orientation
	vrpn_TBF_axis axis;	//< Axis to add to the ButtonFly

	if (sscanf(line, "differential %199s%d%g%g%g%g%g%g", name,
		&which, &x,&y,&z, &rx,&ry,&rz) != 8) {
	  fprintf(stderr,"ButtonFly: Bad differential line\n");
	  return -1;
	}

	float vec[3], rot[3];
	vec[0] = x; vec[1] = y; vec[2] = z;
	rot[0] = rx; rot[1] = ry; rot[2] = rz;
	axis.absolute = false;
	memcpy(axis.name, name, sizeof(axis.name));
	axis.channel = which;
	memcpy(axis.vec, vec, sizeof(axis.vec));
	memcpy(axis.rot, rot, sizeof(axis.rot));

	if (!p.add_axis(axis)) {
	  fprintf(stderr,"ButtonFly: Could not add differential axis to parameters\n");
	  return -1;
	}
      } else if (strcmp(s3, "vel_scale") == 0) {
	char  name[200];	//< Name of the analog device to read from
	int   channel;		//< Which analog channel to read from
	float offset, scale, power; //< Parameters to apply

	if (sscanf(line, "vel_scale %199s%d%g%g%g", name,
		&channel, &offset, &scale, &power) != 5) {
	  fprintf(stderr,"ButtonFly: Bad vel_scale line\n");
	  return -1;
	}

	memcpy(p.vel_scale_name, name, sizeof(p.vel_scale_name));
	p.vel_scale_channel = channel;
	p.vel_scale_offset = offset;
	p.vel_scale_scale = scale;
	p.vel_scale_power = power;
      } else if (strcmp(s3, "rot_scale") == 0) {
	char  name[200];	//< Name of the analog device to read from
	int   channel;		//< Which analog channel to read from
	float offset, scale, power; //< Parameters to apply

	if (sscanf(line, "rot_scale %199s%d%g%g%g", name,
		&channel, &offset, &scale, &power) != 5) {
	  fprintf(stderr,"ButtonFly: Bad rot_scale line\n");
	  return -1;
	}

	memcpy(p.rot_scale_name, name, sizeof(p.rot_scale_name));
	p.rot_scale_channel = channel;
	p.rot_scale_offset = offset;
	p.rot_scale_scale = scale;
	p.rot_scale_power = power;
      }
    }

    trackers[num_trackers] = new
       vrpn_Tracker_ButtonFly (s2, connection, &p, f1);

    if (!trackers[num_trackers]) {
      fprintf(stderr,"Can't create new vrpn_Tracker_ButtonFly\n");
      return -1;
    } else {
      num_trackers++;
    }

    return 0;
}

int setup_Joystick (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE], s3 [LINESIZE];
  int i1;

    float fhz;
    // Get the arguments
    next();
    if (sscanf(pch, "%511s%511s%d %f", s2, s3, &i1, &fhz) != 4) {
      fprintf(stderr, "Bad vrpn_Joystick line: %s\n", line);
      return -1;
    }

    // Make sure there's room for a new joystick server
    if (num_analogs >= MAX_ANALOG) {
      fprintf(stderr, "Too many analog devices in config file");
      return -1;
    }

    // Open the joystick server
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

  return 0;
}

int setup_DialExample (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE];
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
  char s2 [LINESIZE], s3 [LINESIZE];
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

int setup_Magellan (char * & pch, char * line, FILE * config_file)
{
  char s2[LINESIZE], s3[LINESIZE], s4[LINESIZE];
  int i1;
  int ret;
  bool	altreset = false;

  next();

  // Get the arguments (class, magellan_name, port, baud, [optionally "altreset"]
  if ( (ret = sscanf(pch,"%511s%511s%d%511s",s2,s3, &i1, s4)) < 3) {
    fprintf(stderr,"Bad vrpn_Magellan line: %s\n",line);
    return -1;
  }

  // See if we are using alternate reset line
  if (ret == 4) {
    if (strcmp(s4, "altreset") == 0) {
      altreset = true;
    } else {
      fprintf(stderr,"Bad vrpn_Magellan line: %s\n",line);
      return -1;
    }
  }

  // Make sure there's room for a new magellan
  if (num_magellans >= MAX_MAGELLANS) {
    fprintf(stderr,"Too many Magellans in config file");
    return -1;
  }

  // Open the device
  if (verbose) {
    printf("Opening vrpn_Magellan: %s on port %s, baud %d\n", s2,s3,i1);
  }
  if ((magellans[num_magellans] = new vrpn_Magellan(s2, connection, s3, i1, altreset)) == NULL) {
      fprintf(stderr,"Can't create new vrpn_Magellan\n");
      return -1;
  } else {
    num_magellans++;
  }

  return 0;
}

int setup_Spaceball (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE], s3 [LINESIZE];
  int i1;

            next();
            // Get the arguments (class, magellan_name, port, baud
            if (sscanf(pch,"%511s%511s%d",s2,s3, &i1) != 3)
 {
              fprintf(stderr,"Bad vrpn_Spaceball line: %s\n",line);
              return -1;
            }

            // Make sure there's room for a new magellan
            if (num_spaceballs >= MAX_SPACEBALLS) {
              fprintf(stderr,"Too many Spaceballs in config file");
              return -1;
            }

            // Open the device
            if (verbose)
              printf("Opening vrpn_Spaceball: %s on port %s, baud %d\n",
                    s2,s3,i1);
            if ((spaceballs[num_spaceballs] =
                  new vrpn_Spaceball(s2, connection, s3, i1)) == NULL)

              {
                fprintf(stderr,"Can't create new vrpn_Spaceball\n");
                return -1;
              } else {
                num_spaceballs++;
              }

  return 0;
}

int setup_Radamec_SPI (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE], s3 [LINESIZE];
  int i1;

            next();
            // Get the arguments (class, Radamec_name, port, baud
            if (sscanf(pch,"%511s%511s%d",s2,s3, &i1) != 3)
 {
              fprintf(stderr,"Bad vrpn_Radamec_SPI line: %s\n",line);
              return -1;
            }

            // Make sure there's room for a new analog
            if (num_analogs >= MAX_ANALOG) {
              fprintf(stderr,"Too many Analogs in config file");
              return -1;
            }

            // Open the device
            if (verbose)
              printf("Opening vrpn_Radamec_SPI: %s on port %s, baud %d\n",
                    s2,s3,i1);
            if ((analogs[num_analogs] =
                  new vrpn_Radamec_SPI(s2, connection, s3, i1)) == NULL)
              {
                fprintf(stderr,"Can't create new vrpn_Radamec_SPI\n");
                return -1;
              } else {
                num_analogs++;
              }

  return 0;
}

int setup_Zaber (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE], s3 [LINESIZE];

  next();
  // Get the arguments (class, Radamec_name, port, baud
  if (sscanf(pch,"%511s%511s",s2,s3) != 2) {
    fprintf(stderr,"Bad vrpn_Zaber: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new analog
  if (num_analogs >= MAX_ANALOG) {
    fprintf(stderr,"Too many Analogs in config file");
    return -1;
  }

  // Open the device
  if (verbose) {
    printf("Opening vrpn_Zaber: %s on port %s\n", s2,s3);
  }
  if ((analogs[num_analogs] =
  new vrpn_Zaber(s2, connection, s3)) == NULL) {
    fprintf(stderr,"Can't create new vrpn_Zaber\n");
    return -1;
  } else {
    num_analogs++;
  }

  return 0;
}

int setup_ImmersionBox (char * & pch, char * line, FILE * config_file) {
    char s2 [LINESIZE], s3 [LINESIZE];
    int i1, i2, i3, i4;
    vrpn_ImmersionBox * ibox;
    next();
    // Get the arguments (class, iboxbox_name, port, baud, numdig,
    // numana, numenc)
    if (sscanf(pch,"%511s%511s%d%d%d%d",s2,s3, &i1, &i2, &i3, &i4) != 6) {
        fprintf(stderr,"Bad vrpn_ImmersionBox line: %s\n",line);
        return -1;
    }

    // Make sure there's room for a new box
    if (num_iboxes >= MAX_IBOXES) {
        fprintf(stderr,"Too many Immersion boxes in config file");
        return -1;
    }

    // Open the box
    if (verbose)
        printf("Opening vrpn_ImmersionBox: %s on port %s, baud %d, %d digital, "
               " %d analog, %d encoders\n",s2,s3,i1, i2,i3,i4);

    ibox = new vrpn_ImmersionBox(s2, connection, s3, i1, i2, i3, i4);
    if (NULL == ibox) {
        fprintf(stderr,"Can't create new vrpn_ImmersionBox\n");
        return -1;
    }
    iboxes[num_iboxes++] = ibox;
    return 0;
}

int setup_5dt (char * & pch, char * line, FILE * config_file)
{
  char name [LINESIZE], device [LINESIZE];
  int baud_rate, mode;

  next();
  // Get the arguments (class, 5DT_name, port, baud
  if (sscanf(pch,"%511s%511s%d%d",name,device, &baud_rate, &mode) != 4)
    {
      fprintf(stderr,"Bad vrpn_5dt line: %s\n",line);
      return -1;
    }
  // Make sure there's room for a new analog
  if (num_analogs >= MAX_ANALOG) {
      fprintf(stderr,"Too many Analogs in config file");
      return -1;
  }

  // Open the device
  if (verbose) {
    printf("Opening vrpn_5dt: %s on port %s, baud %d\n",
	   name,device,baud_rate);
  }
  if ((analogs[num_analogs] =
       new vrpn_5dt (name, connection, device, baud_rate, mode)) == NULL) {
      fprintf(stderr,"Can't create new vrpn_5dt\n");
      return -1;
  } else {
      num_analogs++;
  }

  return 0;
}

int setup_Wanda (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE], s3 [LINESIZE];
  int i1;

    float fhz;
    // Get the arguments Name, Serial_Port, Baud_Rate, Min_Update_Rate
    next();
    if (sscanf(pch, "%511s%511s%d %f", s2, s3, &i1, &fhz) != 4) {
      fprintf(stderr, "Bad vrpn_Wanda line: %s\n", line);
      return -1;
    }

    // Make sure there's room for a new wanda server
    if (num_analogs >= MAX_ANALOG) {
      fprintf(stderr, "Too many analog devices in config file");
      return -1;
    }

    // Create the server
    if (verbose)
      printf("Opening vrpn_Wanda:  "
             "%s on port %s baud %d, min update rate = %.2f\n",
             s2,s3, i1, fhz);

    if ((analogs[num_analogs] =
         new vrpn_Wanda(s2, connection,s3, i1, fhz)) == NULL) {
        fprintf(stderr, "Can't create new vrpn_Wanda\n");
        return -1;
    } else {
        num_analogs++;
    }

  return 0;
}

int setup_Tracker_Dyna (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];
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

  char s2 [LINESIZE], s3 [LINESIZE], s4 [LINESIZE];
  int i1;
  int numparms;
  vrpn_Tracker_Fastrak	*mytracker;
  int do_is900_timing = 0;

        char    rcmd[5000];     // Reset command to send to Fastrak
        next();
        // Get the arguments (class, tracker_name, port, baud, [optional IS900time])
        if ( (numparms = sscanf(pch,"%511s%511s%d%511s",s2,s3,&i1,s4)) < 3) {
          fprintf(stderr,"Bad vrpn_Tracker_Fastrak line: %s\n%s %s\n",
                  line, pch, s3);
          return -1;
        }

	// See if we got the optional parameter to enable IS900 timings
	if (numparms == 4) {
	    if (strncmp(s4, "IS900time", strlen("IS900time")) == 0) {
		do_is900_timing = 1;
		printf(" ...using IS900 timing information\n");
	    } else if (strcmp(s4, "/") == 0) {
		// Nothing to do
	    } else if (strcmp(s4, "\\") == 0) {
		// Nothing to do
	    } else {
		fprintf(stderr,"Fastrak/Isense: Bad timing optional param (expected 'IS900time', got '%s')\n",s4);
		return -1;
	    }
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
          if (fgets(line, LINESIZE, config_file) == NULL) {
              fprintf(stderr,"Ran past end of config file in Fastrak/Isense description\n");
                  return -1;
          }

          // Copy the line into the remote command,
          // then replace \ with \015 if present
          // In any case, make sure we terminate with \015.
          strncat(rcmd, line, LINESIZE);
          if (rcmd[strlen(rcmd)-2] == '\\') {
                  rcmd[strlen(rcmd)-2] = '\015';
                  rcmd[strlen(rcmd)-1] = '\0';
          } else if (rcmd[strlen(rcmd)-2] == '/') {
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

        if ( (trackers[num_trackers] = mytracker =
             new vrpn_Tracker_Fastrak(s2, connection, s3, i1, 1, 4, rcmd, do_is900_timing))
                            == NULL){

#endif

          fprintf(stderr,"Can't create new vrpn_Tracker_Fastrak\n");
          return -1;

#if defined(sgi) || defined(linux) || defined(WIN32)

        } else {
	  // If the last character in the line is a front slash, '/', then
	  // the following line is a command to add a Wand or Stylus to one
	  // of the sensors on the tracker.  Read and parse the line after,
	  // then add the devices needed to support.  Each line has two
	  // arguments, the string name of the devices and the integer
	  // sensor number (starting with 0) to attach the device to.
          while (line[strlen(line)-2] == '/') {
	    char lineCommand[LINESIZE];
	    char lineName[LINESIZE];
	    int	 lineSensor;

            // Read the next line
            if (fgets(line, LINESIZE, config_file) == NULL) {
              fprintf(stderr,"Ran past end of config file in Fastrak/Isense description\n");
                  return -1;
	    }

	    // Parse the line.  Both "Wand" and "Stylus" lines start with the name and sensor #
	    if (sscanf(line, "%511s%511s%d", lineCommand, lineName, &lineSensor) != 3) {
		fprintf(stderr,"Bad line in Wand/Stylus description for Fastrak/Isense (%s)\n",line);
		delete trackers[num_trackers];
		return -1;
	    }

	    // See which command it is and act accordingly
	    if (strcmp(lineCommand, "Wand") == 0) {
		double c0min, c0lo0, c0hi0, c0max;
		double c1min, c1lo0, c1hi0, c1max;

		// Wand line has additional scale/clip information; read it in
		if (sscanf(line, "%511s%511s%d%lf%lf%lf%lf%lf%lf%lf%lf", lineCommand, lineName, &lineSensor,
			   &c0min, &c0lo0, &c0hi0, &c0max,
			   &c1min, &c1lo0, &c1hi0, &c1max) != 11) {
		    fprintf(stderr,"Bad line in Wand description for Fastrak/Isense (%s)\n",line);
		    delete trackers[num_trackers];
		    return -1;
		}

		if (mytracker->add_is900_analog(lineName, lineSensor, c0min, c0lo0, c0hi0, c0max,
		    c1min, c1lo0, c1hi0, c1max)) {
		  fprintf(stderr,"Cannot set Wand analog for Fastrak/Isense (%s)\n",line);
		  delete trackers[num_trackers];
		  return -1;
		}
		if (mytracker->add_is900_button(lineName, lineSensor, 5)) {
		  fprintf(stderr,"Cannot set Wand buttons for Fastrak/Isense (%s)\n",line);
		  delete trackers[num_trackers];
		  return -1;
		}
		printf(" ...added Wand (%s) to sensor %d\n", lineName, lineSensor);

	    } else if (strcmp(lineCommand, "Stylus") == 0) {
		if (mytracker->add_is900_button(lineName, lineSensor, 2)) {
		  fprintf(stderr,"Cannot set Stylus buttons for Fastrak/Isense (%s)\n",line);
		  delete trackers[num_trackers];
		  return -1;
		}
		printf(" ...added Stylus (%s) to sensor %d\n", lineName, lineSensor);
	    } else if (strcmp(lineCommand, "FTStylus") == 0) {
		if (mytracker->add_fastrak_stylus_button(lineName, lineSensor, 1)) {
		  fprintf(stderr,"Cannot set Stylus buttons for Fastrak (%s)\n",line);
		  delete trackers[num_trackers];
		  return -1;
		}
		printf(" ...added FTStylus (%s) to sensor %d\n", lineName, lineSensor);
	    } else {
		fprintf(stderr,"Unknown command in Wand/Stylus description for Fastrak/Isense (%s)\n",lineCommand);
		delete trackers[num_trackers];
		return -1;
	    }

	  }

          num_trackers++;
        }

#endif

  return 0;
}

int setup_Tracker_3Space (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];
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

  char s2 [LINESIZE], s3 [LINESIZE];
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

  char s2 [LINESIZE], s3 [LINESIZE];
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
              rgs[iSlaves]=new char[LINESIZE];
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

  char s2 [LINESIZE];
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
                  ((vrpn_Tracker_NULL *) trackers[num_trackers])->
                    setRedundantTransmission(redundancy);
                  num_trackers++;
                }

  return 0;
}

int setup_Button_Python (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE];
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
int setup_Button_SerialMouse (char * & pch, char * line, FILE * config_file) {

    char s2 [LINESIZE];
    char s3 [LINESIZE];
    char s4 [32];
    vrpn_MOUSETYPE mType = MAX_MOUSE_TYPES;
    
    next();
    // Get the arguments (class, button_name, portname, type)
    if (sscanf(pch,"%511s%511s%31s",s2,s3,s4) != 3) {
		fprintf(stderr,"Bad vrpn_Button_SerialMouse line: %s\n",line);
		return -1;
    }
    
    // Make sure there's room for a new button
    if (num_buttons >= MAX_BUTTONS) {
		fprintf(stderr,"Too many buttons in config file");
		return -1;
    }
    
    if (strcmp(s4, "mousesystems") == 0) mType = MOUSESYSTEMS;
    else if (strcmp(s4, "3button") == 0) mType = THREEBUTTON_EMULATION;
    
    if (mType == MAX_MOUSE_TYPES) {
		fprintf(stderr,"bad vrpn_SerialMouse_Button type\n");
		return -1;
    }

    // Open the button
    if (verbose) printf("Opening vrpn_SerialMouse_Button: %s on port %s\n", s2,s3);
    if ( (buttons[num_buttons] =
		new vrpn_Button_SerialMouse(s2, connection, s3, 1200, mType)) == NULL){
		fprintf(stderr,"Can't create new vrpn_Button_SerialMouse\n");
		return -1;
    } else {
		num_buttons++;
    }
    return 0;
}

//================================
int setup_Button_PinchGlove(char* &pch, char *line, FILE *config_file) {

   char name[LINESIZE], port[LINESIZE];
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

//================================
int setup_Joylin (char * & pch, char * line, FILE * config_file) {
  char s2[LINESIZE];
  char s3[LINESIZE];
  
  // Get the arguments
  next();
  if (sscanf(pch, "%511s%511s", s2, s3) != 2) {
    fprintf(stderr, "Bad vrpn_Joylin line: %s\n", line);
    return -1;
  }
  
  // Make sure there's room for a new joystick server
  if (num_analogs >= MAX_ANALOG) {
    fprintf(stderr, "Too many analog devices in config file");
    return -1;
  }

  // Open the joystick server
  if (verbose) 
    printf("Opening vrpn_Joylin: %s on port %s\n", s2, s3);
  if ((analogs[num_analogs] = 
       new vrpn_Joylin(s2, connection, s3)) == NULL) {
    fprintf(stderr, "Can't create new vrpn_Joylin\n");
    return -1;
  } else {
    num_analogs++;
  }
  return 0;
}

//================================
int setup_Tng3 (char * & pch, char * line, FILE * config_file) {
    char s2 [LINESIZE], s3 [LINESIZE];
    int i1, i2;
    vrpn_Tng3 * tng3;
    next();
    // Get the arguments (class, tng3_name, port, numdig, numana)
    if (sscanf(pch,"%511s%511s%d%d",s2,s3, &i1, &i2) != 4) {
        fprintf(stderr,"Bad vrpn_Tng3 line: %s\n",line);
        return -1;
    }

    // Make sure there's room for a new box
    if (num_tng3s >= MAX_TNG3S) {
        fprintf(stderr,"Too many Tng3 boxes in config file");
        return -1;
    }

    // Open the box
    if (verbose)
        printf("Opening vrpn_Tng3: %s on port %s, baud %d, %d digital, "
               " %d analog\n",s2,s3,19200,i1,i2);

    tng3 = new vrpn_Tng3(s2, connection, s3, 19200, i1, i2);
    if (NULL == tng3) {
        fprintf(stderr,"Can't create new vrpn_Tng3\n");
        return -1;
    }
    tng3s[num_tng3s++] = tng3;
    return 0;
}

//================================
int setup_Tracker_InterSense(char * &pch, char *line, FILE * config_file) {
  char trackerName[64];
  char commStr[10];
  int commPort;

  sscanf(line,"vrpn_Tracker_InterSense %s %s",trackerName,commStr);

  if(!strcmp(commStr,"COM1")) {
    commPort = 1;
  } else if(!strcmp(commStr,"COM2")) {
    commPort = 2;
  } else if(!strcmp(commStr,"COM3")) {
    commPort = 3;
  } else if(!strcmp(commStr,"COM4")) {
    commPort = 4;
  } else {
    fprintf(stderr,"Error determining COM port: %s not should be either COM1, COM2, COM3, or COM4",commStr);
    return 0;
  }

  trackers[num_trackers] = new vrpn_Tracker_InterSense(trackerName,connection,commPort);
  num_trackers++;

  return 0;
}

//================================
int setup_DirectXFFJoystick (char * & pch, char * line, FILE * config_file) {
#ifdef	VRPN_USE_DIRECTINPUT
  char s2 [LINESIZE];
  float f1, f2;

  next();
  // Get the arguments (joystick_name, read update rate, force update rate)
  if (sscanf(pch,"%511s%g%g",s2,&f1, &f2) != 3) {
    fprintf(stderr,"Bad vrpn_DirectXFFJoystick line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new joystick
  if (num_DirectXJoys >= MAX_DIRECTXJOYS) {
    fprintf(stderr,"Too many Direct X FF Joysticks in config file");
    return -1;
  }

  // Open the joystick
  if (verbose) {
  printf("Opening vrpn_DirectXFFJoystick: %s, read rate %g, force rate %g\n",
	s2, f1,f2);
  }
  if ((DirectXJoys[num_DirectXJoys] =
      new vrpn_DirectXFFJoystick(s2, connection, f1, f2)) == NULL)               
  {
    fprintf(stderr,"Can't create new vrpn_DirectXFFJoystick\n");
    return -1;
  } else {
    num_DirectXJoys++;
  }

  return 0;
#else
  fprintf(stderr, "vrpn_server: Can't open DirectXFFJoystick: VRPN_USE_DIRECTINPUT not defined in vrpn_Configure.h!\n");
  return -1;
#endif
}

int setup_GlobalHapticsOrb (char * & pch, char * line, FILE * config_file) {
  char s2[LINESIZE], s3[LINESIZE];
  int  i1;

  next();
  // Get the arguments (orb_name, port name, baud rate)
  if (sscanf(pch,"%511s%s%d",s2,s3, &i1) != 3) {
    fprintf(stderr,"Bad vrpn_GlobalHapticsOrb line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new orb
  if (num_GlobalHapticsOrbs >= MAX_GLOBALHAPTICSORBS) {
    fprintf(stderr,"Too many Global Haptics Orbs in config file");
    return -1;
  }

  // Open the orb
  if (verbose) {
  printf("Opening vrpn_GlobalHapticsOrb: %s, port %s, baud rate %d\n",
	s2, s3, i1);
  }
  if ((ghos[num_GlobalHapticsOrbs] =
      new vrpn_GlobalHapticsOrb(s2, connection, s3, i1)) == NULL)               
  {
    fprintf(stderr,"Can't create new vrpn_GlobalHapticsOrb\n");
    return -1;
  } else {
    num_GlobalHapticsOrbs++;
  }

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
	int	port = vrpn_DEFAULT_LISTEN_PORT_NO;
	int	milli_sleep_time = 1;		// How long to sleep each iteration (default 1ms)
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
#endif // not sgi
#endif // not WIN32

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
          } else if (!strcmp(argv[i], "-li")) { // specify server-side logging
            if (++i > argc) { Usage(argv[0]); }
            fprintf(stderr, "Incoming logfile name %s.\n", argv[i]);
            g_inLogName = argv[i];
          } else if (!strcmp(argv[i], "-lo")) { // specify server-side logging
            if (++i > argc) { Usage(argv[0]); }
            fprintf(stderr, "Outgoing logfile name %s.\n", argv[i]);
            g_outLogName = argv[i];
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
             (port, g_inLogName, g_outLogName, g_NICname);
	
        // Open the Redundant Transmission support
        // Need to have this handy to pass into NULL trackers,
        // which are currently the only things willing to use them.
        redundancy = new vrpn_RedundantTransmission (connection);
        redundantController = new vrpn_RedundantController
                                        (redundancy, connection);

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
      {	char	line[LINESIZE];	// Line read from the input file
        char *pch;
	char    scrap[LINESIZE];
	char	s1[LINESIZE];
        int retval;

	// Read lines from the file until we run out
	while ( fgets(line, LINESIZE, config_file) != NULL ) {

	  // Make sure the line wasn't too long
	  if (strlen(line) >= LINESIZE-1) {
		fprintf(stderr,"Line too long in config file: %s\n",line);
		if (bail_on_error) { return -1; }
		else { continue; }	// Skip this line
	  }

	  if ((strlen(line)<3)||(line[0]=='#')) {
	    // comment or empty line -- ignore
	    continue;
	  }

	  // copy for strtok work
	  strncpy(scrap, line, LINESIZE);
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
	  } else if (isit("vrpn_Tracker_ButtonFly")) {
            CHECK(setup_Tracker_ButtonFly);
	  } else  if (isit("vrpn_Joystick")) {
            CHECK(setup_Joystick);
	  } else  if (isit("vrpn_Joylin")) {
            CHECK(setup_Joylin);
	  } else if (isit("vrpn_Dial_Example")) {
            CHECK(setup_DialExample);
	  } else if (isit("vrpn_CerealBox")) {
            CHECK(setup_CerealBox);
	  } else if (isit("vrpn_Magellan")) {
            CHECK(setup_Magellan);
          } else if (isit("vrpn_Spaceball")) {
            CHECK(setup_Spaceball);
	  } else if (isit("vrpn_Radamec_SPI")) {
            CHECK(setup_Radamec_SPI);
	  } else if (isit("vrpn_Zaber")) {
            CHECK(setup_Zaber);
	  } else if (isit("vrpn_5dt")) {
            CHECK(setup_5dt);
	  } else if (isit("vrpn_ImmersionBox")) {
            CHECK(setup_ImmersionBox);
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
	  } else if (isit("vrpn_Button_SerialMouse")) {
            CHECK(setup_Button_SerialMouse);
	  } else  if (isit("vrpn_Wanda")) {
            CHECK(setup_Wanda);
	  } else if (isit("vrpn_Tng3")) {
            CHECK(setup_Tng3);
	  } else  if (isit("vrpn_TimeCode_Generator")) {
	    CHECK(setup_Timecode_Generator);
	  } else if (isit("vrpn_Tracker_InterSense")) {
	    CHECK(setup_Tracker_InterSense);
	  } else if (isit("vrpn_DirectXFFJoystick")) {
	    CHECK(setup_DirectXFFJoystick);
	  } else if (isit("vrpn_GlobalHapticsOrb")) {
	    CHECK(setup_GlobalHapticsOrb);
	  } else {	// Never heard of it
		sscanf(line,"%511s",s1);	// Find out the class name
		fprintf(stderr,"vrpn_server: Unknown Device: %s\n",s1);
		if (bail_on_error) { return -1; }
		else { continue; }	// Skip this line
	  }
	}
      }

	// Close the configuration file
	fclose(config_file);

	// Open the Forwarder Server
        forwarderServer = new vrpn_Forwarder_Server (connection);

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
	    if (connection->connect_to_client(client_name, client_port)){
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
	while (!done) {
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

	  // Let all the Magellans do their thing
	  for (i=0; i< num_magellans; i++) {
		  magellans[i]->mainloop();
	  }

          // Let all the Spaceballs do their thing
          for (i=0; i< num_spaceballs; i++) {
                  spaceballs[i]->mainloop();
          }

          // Let all the Immersion boxes do their thing
          for (i=0; i< num_iboxes; i++) {
                  iboxes[i]->mainloop();
          }

	  // Let all of the SGI button/knob boxes do their thing
	  for (i=0; i < num_sgiboxes; i++) {
		  sgiboxes[i]->mainloop();
	  }
#ifdef SGI_BDBOX
	  if (vrpn_special_sgibox) 
	    vrpn_special_sgibox->mainloop();
#endif
#ifdef INCLUDE_TIMECODE_SERVER
	  for (i=0; i < num_generators; i++) {
		  timecode_generators[i]->mainloop();
	  }
#endif

	  // Let all the TNG3 do their thing
          for (i=0; i< num_tng3s; i++) {
		  tng3s[i]->mainloop();
          }

#ifdef	VRPN_USE_DIRECTINPUT
	  // Let all the FF joysticks do their thing
	  for (i = 0; i < num_DirectXJoys; i++) {
	    DirectXJoys[i]->mainloop();
	  }
#endif

	  // Let all the Orbs do their thing
	  for (i = 0; i < num_GlobalHapticsOrbs; i++) {
	    ghos[i]->mainloop();
	  }

          redundantController->mainloop();
          redundancy->mainloop();

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

	shutDown();
	return 0;
}
