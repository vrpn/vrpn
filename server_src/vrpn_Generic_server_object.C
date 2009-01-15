#include "vrpn_Generic_server_object.h"

const int LINESIZE = 512;

#define CHECK(s) \
    retval = (s)(pch, line, config_file); \
    if (retval && d_bail_on_open_error) {\
      d_doing_okay = false; return; \
    } else {\
      continue; \
    }

#define next() pch += strlen(pch) + 1

// BUW additions
/* some helper variables to configure the vrpn_Atmel server */
namespace setup_vrpn_Atmel {
  int channel_mode[vrpn_CHANNEL_MAX];
  int channel_count=0;
}

#ifdef	SGI_BDBOX
vrpn_SGIBox	* vrpn_special_sgibox;
#endif

void vrpn_Generic_Server_Object::closeDevices (void) {
  int i;
  for (i=0;i < num_buttons; i++) {
    fprintf(stderr, "\nClosing button %d ...", i);
    delete buttons[i];
  }
  for (i=0;i < num_trackers; i++) {
    fprintf(stderr, "\nClosing tracker %d ...", i);
    delete trackers[i];
  }
#ifdef VRPN_USE_PHANTOM_SERVER
  for (i=0;i < num_phantoms; i++) {
    fprintf(stderr, "\nClosing Phantom %d ...", i);
    delete phantoms[i];
  }
#endif
  for (i=0;i < num_sounds; i++) {
    fprintf(stderr, "\nClosing sound %d ...", i);
    delete sounds[i];
  }
  for (i=0;i < num_analogs; i++) {
    fprintf(stderr, "\nClosing analog %d ...", i);
    delete analogs[i];
  }
  for (i=0;i < num_sgiboxes; i++) {
    fprintf(stderr, "\nClosing sgibox %d ...", i);
    delete sgiboxes[i];
  }
  for (i=0;i < num_cereals; i++) {
    fprintf(stderr, "\nClosing cereal %d ...", i);
    delete cereals[i];
  }
  for (i=0;i < num_magellans; i++) {
    fprintf(stderr, "\nClosing magellan %d ...", i);
    delete magellans[i];
  }
  for (i=0;i < num_spaceballs; i++) {
    fprintf(stderr, "\nClosing spaceball %d ...", i);
    delete spaceballs[i];
  }
  for (i=0;i < num_iboxes; i++) {
    fprintf(stderr, "\nClosing ibox %d ...", i);
    delete iboxes[i];
  }
  for (i=0;i < num_dials; i++) {
    fprintf(stderr, "\nClosing dial %d ...", i);
    delete dials[i];
  }
#ifdef VRPN_INCLUDE_TIMECODE_SERVER
  for (i=0;i < num_timecode_generators; i++) {
    fprintf(stderr, "\nClosing timecode_generator %d ...", i);
    delete timecode_generators[i];
  }
#endif
  for (i=0;i < num_tng3s; i++) {
    fprintf(stderr, "\nClosing tng3 %d ...", i);
    delete tng3s[i];
  }
#ifdef	VRPN_USE_DIRECTINPUT
  for (i=0;i < num_DirectXJoys; i++) {
    fprintf(stderr, "\nClosing DirectXJoy %d ...", i);
    delete DirectXJoys[i];
  }
  for (i=0;i < num_RumblePads; i++) {
    fprintf(stderr, "\nClosing RumblePad %d ...", i);
    delete RumblePads[i];
  }
#endif
#ifdef	_WIN32
  for (i=0;i < num_Win32Joys; i++) {
    fprintf(stderr, "\nClosing win32joy %d ...", i);
    delete win32joys[i];
  }
#endif
  for (i=0;i < num_GlobalHapticsOrbs; i++) {
    fprintf(stderr, "\nClosing Global Haptics Orb %d ...", i);
    delete ghos[i];
  }
  for (i=0;i < num_DTracks; i++) {
    fprintf(stderr, "\nClosing DTrack %d ...", i);
    delete DTracks[i];
  }
  for (i=0;i < num_analogouts; i++) {
    fprintf(stderr, "\nClosing analogout %d ...", i);
    delete analogouts[i];
  }
  for (i=0;i < num_posers; i++) {
    fprintf(stderr, "\nClosing poser %d ...", i);
    delete posers[i];
  }
  for (i=0;i < num_Keyboards; i++) {
    fprintf(stderr, "\nClosing Keyboard %d ...", i);
    delete Keyboards[i];
  }
  for (i=0;i < num_loggers; i++) {
    fprintf(stderr, "\nClosing logger %d ...", i);
    delete loggers[i];
  }
  for (i=0;i < num_imagestreams; i++) {
    fprintf(stderr, "\nClosing imagestream %d ...", i);
    delete imagestreams[i];
  }
  for (i=0;i < num_inertiamouses; i++) {
    fprintf(stderr, "\nClosing inertiamouse %d ...", i);
    delete inertiamouses[i];
  }
#ifdef	VRPN_USE_WIIUSE
  for (i=0;i < num_wiimotes; i++) {
    fprintf(stderr, "\nClosing wiimote %d ...", i);
    delete wiimotes[i];
  }
#endif
  if (verbose) { fprintf(stderr, "\nAll devices closed...\n"); }
}

// setup_raw_SGIBox
// uses globals:  num_sgiboxes, sgiboxes[], verbose
// imports from main:  pch
// returns nonzero on error

int vrpn_Generic_Server_Object::setup_raw_SGIBox (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];

        // Line will be: vrpn_raw_SGIBox NAME PORT [list of buttons to toggle]
        int tbutton;    // Button to toggle
        next();
        if (sscanf(pch,"%511s %511s",s2,s3)!=2) {
                fprintf(stderr,"Bad vrpn_raw_SGIBox line: %s\n",line);
                return -1;
        }

        // Make sure there's room for a new raw SGIBox
        if (num_sgiboxes >= VRPN_GSO_MAX_SGIBOX) {
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

int vrpn_Generic_Server_Object::setup_SGIBox(char * & pch, char * line, FILE * config_file) {

#ifdef SGI_BDBOX

	    char s2 [LINESIZE];

            int tbutton;
            next();
            if (sscanf(pch,"%511s",s2)!=1) {
              fprintf(stderr,"Bad vrpn_SGIBox line: %s\n",line);
              return -1;
            }

                // Open the sgibox
              if (verbose) printf("Opening vrpn_SGIBox on host %s\n", s2);
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
                printf("Opening vrpn_SGIBox on host %s done\n", s2);

#else
                fprintf(stderr,"vrpn_server: Can't open SGIbox: not an SGI!  Try raw_SGIbox instead.\n");
#endif

  return 0;  // successful completion
}


int vrpn_Generic_Server_Object::setup_Timecode_Generator (char * & pch, char * line, FILE * config_file) {
#ifdef VRPN_INCLUDE_TIMECODE_SERVER
	char s2 [LINESIZE];

	next();
	if (sscanf(pch,"%511s",s2)!=1) {
		fprintf(stderr,"Timecode_Generator line: %s\n",line);
		return -1;
	}

	// Make sure there's room for a new generator
    if (num_generators >= VRPN_GSO_MAX_TIMECODE_GENERATORS) {
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


int vrpn_Generic_Server_Object::setup_Phantom(char * &pch, char *line, FILE * config_file) {
#ifdef VRPN_USE_PHANTOM_SERVER
	char	s2[512];	// String parameters
	int	i1;		// Integer parameters
	float	f1;		// Float parameters

	next();
	if (sscanf(pch, "%511s%d%f",s2,&i1,&f1) != 3) {
		fprintf(stderr,"Bad vrpn_Phantom line: %s\n",line);
		return -1;
	}

	if (num_phantoms >= VRPN_GSO_MAX_PHANTOMS) {
		fprintf(stderr,"Too many Phantoms in config file");
		return -1;
	}

	if (verbose) {
		printf("Opening vrpn_Phantom:\n");
	}

	// i1 is a boolean that tells whether to let the user establish the reset
	// position or not.
	if (i1) {
	  printf("Initializing phantom, you have 10 seconds to establish reset position\n");
	  vrpn_SleepMsecs(10000);
	}

	phantoms[num_phantoms] =  new vrpn_Phantom(s2, connection, f1);
	if (phantoms[num_phantoms] == NULL) {
	  fprintf(stderr,"Can't create new vrpn_Phantom\n");
	  return -1;
	}

	num_phantoms++;

	return 0;
#else
	fprintf(stderr, "vrpn_server: Can't open Phantom server: VRPN_USE_PHANTOM_SERVER not defined in vrpn_Configure.h!\n");
	return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_JoyFly (char * & pch, char * line, FILE * config_file) {
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
                if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
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

// This function will read one line of the vrpn_AnalogFly configuration (matching
// one axis) and fill in the data for that axis. The axis name, the file to read
// from, and the axis to fill in are passed as parameters. It returns 0 on success
// and -1 on failure.

int vrpn_Generic_Server_Object::get_AFline(char *line, vrpn_TAF_axis *axis)
{
    char    _axis_name[LINESIZE];
    char *name = new char[LINESIZE]; // We need this to stay around for the param
    int channel;
    float   offset, thresh, power, scale;

    // Get the values from the line
    if (sscanf(line, "%511s%511s%d%g%g%g%g", _axis_name, name,
            &channel, &offset, &thresh,&scale,&power) != 7) {
        fprintf(stderr,"AnalogFly Axis: Bad axis line\n");
        return -1;
    }

    if (strcmp(name, "NULL") == 0) {
      axis->name = NULL;
    } else {
      axis->name = name;
    }
    axis->channel = channel;
    axis->offset = offset;
    axis->thresh = thresh;
    axis->scale = scale;
    axis->power = power;

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_AnalogFly (char * & pch, char * line, FILE * config_file)
{
    char s2 [LINESIZE], s3 [LINESIZE];
    int i1;
    float f1;
    vrpn_Tracker_AnalogFlyParam     p;
    vrpn_bool    absolute;

    next();

    if (sscanf(pch, "%511s%g%511s",s2,&f1,s3) != 3) {
        fprintf(stderr, "Bad vrpn_Tracker_AnalogFly line: %s\n", line);
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
    if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
        fprintf(stderr,"Too many trackers in config file");
        return -1;
    }

    if (verbose) {
        printf("Opening vrpn_Tracker_AnalogFly: "
               "%s with update rate %g\n",s2,f1);
    }

    // Scan the following lines in the configuration file to fill
    // in the start-up parameters for the different axis.

    // parse lines until an empty line is encountered
    while(1)
    {
        char line[LINESIZE];

        // Read in the line
        if (fgets(line, LINESIZE, config_file) == NULL)
        {
            perror("AnalogFly Can't read line!");
            return -1;
        }

        // if it is an empty line, finish parsing
        if(line[0] == '\n')
            break;

        // get the first token
        char tok[LINESIZE];
        sscanf(line, "%s", tok);

        if(strcmp(tok, "X") == 0)
        {
            if (get_AFline(line, &p.x)) {
                    fprintf(stderr,"Can't read X line for AnalogFly\n");
                    return -1;
            }

        } else if(strcmp(tok, "Y") == 0)
        {
            if (get_AFline(line, &p.y)) {
                    fprintf(stderr,"Can't read Y line for AnalogFly\n");
                    return -1;
            }

        } else if(strcmp(tok, "Z") == 0)
        {
            if (get_AFline(line, &p.z)) {
                    fprintf(stderr,"Can't read Z line for AnalogFly\n");
                    return -1;
            }

        } else if(strcmp(tok, "RX") == 0)
        {
            if (get_AFline(line, &p.sx)) {
                    fprintf(stderr,"Can't read RX line for AnalogFly\n");
                    return -1;
            }

        } else if(strcmp(tok, "RY") == 0)
        {
            if (get_AFline(line, &p.sy)) {
                    fprintf(stderr,"Can't read RY line for AnalogFly\n");
                    return -1;
            }

        } else if(strcmp(tok, "RZ") == 0)
        {
            if (get_AFline(line, &p.sz)) {
                    fprintf(stderr,"Can't read RZ line for AnalogFly\n");
                    return -1;
            }

        } else if(strcmp(tok, "RESET") == 0)
        {
            // Read the reset line
            if (sscanf(line, "RESET %511s%d", s3, &i1) != 2) {
                fprintf(stderr,"Bad RESET line in AnalogFly: %s\n",line);
                return -1;
            }

            if (strcmp(s3,"NULL") != 0) {
                p.reset_name = strdup(s3);
                p.reset_which = i1;
            }

        } else if(strcmp(tok, "CLUTCH") == 0)
        {
            if (sscanf(line, "CLUTCH %511s%d", s3, &i1) != 2) {
                    fprintf(stderr,"Bad CLUTCH line in AnalogFly: %s\n",line);
                    return -1;
            }

            if (strcmp(s3,"NULL") != 0) {
                    p.clutch_name = strdup(s3);
                    p.clutch_which = i1;
            }
        }
    }

    trackers[num_trackers] = new vrpn_Tracker_AnalogFly (s2, connection, &p, f1, absolute);

    if (!trackers[num_trackers]) {
        fprintf(stderr,"Can't create new vrpn_Tracker_AnalogFly\n");
        return -1;
    } else {
        num_trackers++;
    }

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_ButtonFly (char * & pch, char * line, FILE * config_file) {
    char s2 [LINESIZE], s3 [LINESIZE];
    float f1;
    vrpn_Tracker_ButtonFlyParam     p;

    next();
    if (sscanf(pch, "%511s%g",s2,&f1) != 2) {
            fprintf(stderr, "Bad vrpn_Tracker_ButtonFly line: %s\n", line);
            return -1;
    }

    // Make sure there's room for a new tracker
    if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
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

int vrpn_Generic_Server_Object::setup_Joystick (char * & pch, char * line, FILE * config_file) {
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
    if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
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

int vrpn_Generic_Server_Object::setup_DialExample (char * & pch, char * line, FILE * config_file) {
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
  if (num_dials >= VRPN_GSO_MAX_DIALS) {
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

int vrpn_Generic_Server_Object::setup_CerealBox (char * & pch, char * line, FILE * config_file) {
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
            if (num_cereals >= VRPN_GSO_MAX_CEREALS) {
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

int vrpn_Generic_Server_Object::setup_Magellan (char * & pch, char * line, FILE * config_file)
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
  if (num_magellans >= VRPN_GSO_MAX_MAGELLANS) {
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


int vrpn_Generic_Server_Object::setup_Spaceball (char * & pch, char * line, FILE * config_file) {
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
            if (num_spaceballs >= VRPN_GSO_MAX_SPACEBALLS) {
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

int vrpn_Generic_Server_Object::setup_Radamec_SPI (char * & pch, char * line, FILE * config_file) {
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
            if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
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

int vrpn_Generic_Server_Object::setup_Zaber (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE], s3 [LINESIZE];

  next();
  // Get the arguments (class, Radamec_name, port, baud
  if (sscanf(pch,"%511s%511s",s2,s3) != 2) {
    fprintf(stderr,"Bad vrpn_Zaber: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new analog
  if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
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

int vrpn_Generic_Server_Object::setup_NationalInstrumentsOutput (char * & pch, char * line, FILE * config_file) {

#ifndef	VRPN_USE_NATIONAL_INSTRUMENTS
  fprintf(stderr, "Attemting to use National Instruments board, but not compiled in\n");
  fprintf(stderr, "  (Define VRPN_USE_NATIONAL_INSTRUMENTS in vrpn_Configuration.h\n");
#else
  fprintf(stderr,"Warning: vrpn_NI_Analog_Output is deprecated: use vrpn_National_Instruments instead\n");
  char s2 [LINESIZE], s3 [LINESIZE];
  int i1, i2;
  float f1, f2;

  next();
  // Get the arguments (vrpn_name, NI_board_type, num_channels, polarity, min_voltage, max_voltage
  if (sscanf(pch,"%511s%511s%d%d%f%f",s2,s3, &i1, &i2, &f1, &f2) != 6) {
    fprintf(stderr,"Bad vrpn_NI_Analog_Output: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new analog
  if (num_analogouts >= VRPN_GSO_MAX_ANALOGOUT) {
    fprintf(stderr,"Too many Analog Outputs in config file");
    return -1;
  }

  // Open the device
  if (verbose) {
    printf("Opening vrpn_NI_Analog_Output: %s with %d channels\n", s2, i1);
  }
  if ((analogouts[num_analogouts] =
  new vrpn_Analog_Output_Server_NI(s2, connection, s3, i1, i2 != 0, f1, f2)) == NULL) {
    fprintf(stderr,"Can't create new vrpn_NI_Analog_Output\n");
    return -1;
  } else {
    num_analogouts++;
  }
#endif

  return 0;
}

int vrpn_Generic_Server_Object::setup_NationalInstruments (char * & pch, char * line, FILE * config_file) {

#ifndef	VRPN_USE_NATIONAL_INSTRUMENTS
  fprintf(stderr, "Attemting to use National Instruments board, but not compiled in\n");
  fprintf(stderr, "  (Define VRPN_USE_NATIONAL_INSTRUMENTS in vrpn_Configuration.h\n");
#else
  char s2 [LINESIZE], s3 [LINESIZE];
  int num_in_channels, in_polarity, in_mode, in_range, in_drive_ais, in_gain;
  int num_out_channels, out_polarity;
  float minimum_delay, min_out_voltage, max_out_voltage;

  next();
  // Get the arguments (vrpn_name, NI_board_type,
  //    num_in_channels, minimum_delay, in_polarity, in_mode, in_range, in_drive_ais, in_gain
  //    num_out_channels, out_polarity, min_out_voltage, max_out_voltage
  if (sscanf(pch,"%511s%511s%d%f%d%d%d%d%d%d%d%f%f",s2,s3,
      &num_in_channels, &minimum_delay, &in_polarity, &in_mode, &in_range, &in_drive_ais, &in_gain,
      &num_out_channels, &out_polarity, &min_out_voltage, &max_out_voltage) != 13) {
    fprintf(stderr,"Bad vrpn_National_Instruments_Server: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new analog
  if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
    fprintf(stderr,"Too many Analogs in config file");
    return -1;
  }

  // Open the device
  if (verbose) {
    printf("Opening vrpn_National_Instruments_Server: %s with %d in and %d out channels\n", s2, num_in_channels, num_out_channels);
    printf("  MinDelay %f, In polarity %d, In mode %d, In range %d\n", minimum_delay, in_polarity, in_mode, in_range);
    printf("  In driveAIS %d, In gain %d\n", in_drive_ais, in_gain);
    printf("  Out polarity %d, Min out voltage %f, Max out voltage %f\n", out_polarity, min_out_voltage, max_out_voltage);
  }
  if ((analogs[num_analogs] =
  new vrpn_National_Instruments_Server(s2, connection, s3, num_in_channels, num_out_channels,
          minimum_delay, in_polarity != 0, in_mode, in_range, in_drive_ais != 0, in_gain,
          out_polarity != 0, min_out_voltage, max_out_voltage)) == NULL) {
    fprintf(stderr,"Can't create new vrpn_National_Instruments_Server\n");
    return -1;
  } else {
    num_analogs++;
  }
#endif

  return 0;
}

int vrpn_Generic_Server_Object::setup_ImmersionBox (char * & pch, char * line, FILE * config_file) {
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
    if (num_iboxes >= VRPN_GSO_MAX_IBOXES) {
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

int vrpn_Generic_Server_Object::setup_5dt (char * & pch, char * line, FILE * config_file)
{
  char name [LINESIZE], device [LINESIZE];
  int baud_rate, mode, tenbytes;

  next();
  // Get the arguments (class, 5DT_name, port, baud
  if (sscanf(pch,"%511s%511s%d%d%d",name,device, &baud_rate, &mode, &tenbytes) != 5)
    {
      fprintf(stderr,"Bad vrpn_5dt line: %s\n",line);
      return -1;
    }
  // Make sure there's room for a new analog
  if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
      fprintf(stderr,"Too many Analogs in config file");
      return -1;
  }

  // Open the device
  if (verbose) {
    printf("Opening vrpn_5dt: %s on port %s, baud %d\n",
	   name,device,baud_rate);
  }
  if ((analogs[num_analogs] =
       new vrpn_5dt (name, connection, device, baud_rate, mode, tenbytes != 0)) == NULL) {
      fprintf(stderr,"Can't create new vrpn_5dt\n");
      return -1;
  } else {
      num_analogs++;
  }

  return 0;
}

int vrpn_Generic_Server_Object::setup_5dt16 (char * & pch, char * line, FILE * config_file)
{
        char name [LINESIZE], device [LINESIZE];
        int baud_rate;

        next();
        // Get the arguments (class, 5DT_name, port, baud
        if (sscanf(pch,"%511s%511s%d",name,device, &baud_rate) != 3) {
                fprintf(stderr,"Bad vrpn_5dt16 line: %s\n",line);
                return -1;
        }
        // Make sure there's room for a new analog
        if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
                fprintf(stderr,"Too many Analogs in config file");
                return -1;
        }

        // Open the device
        if (verbose) {
                printf("Opening vrpn_5dt16: %s on port %s, baud %d\n",
                        name,device,baud_rate);
        }
        if ((analogs[num_analogs] = new vrpn_5dt16 (name, connection, device, baud_rate )) == NULL) {
		fprintf(stderr,"Can't create new vrpn_5dt16\n");
		return -1;
	} else {
		num_analogs++;
	}

        return 0;
}


int vrpn_Generic_Server_Object::setup_Keyboard(char * & pch, char * line, FILE * config_file)
{
	char name [LINESIZE];

	next();
	// Get the arguments (class, name
	if (sscanf(pch,"%511s%d%d",name) != 1)
	{
		fprintf(stderr,"Bad vrpn_Keyboard line: %s\n",line);
		return -1;
	}
	// Make sure there's room for a new keyboard
	if (num_Keyboards >= VRPN_GSO_MAX_KEYBOARD) {
		fprintf(stderr,"Too many Keyboards in config file");
		return -1;
	}

	// Open the device
	if (verbose) {
		printf("Opening Keyboard: %s\n", name);
	}
	if ((Keyboards[num_Keyboards] = new vrpn_Keyboard(name, connection)) == NULL)
	{
		fprintf(stderr,"Can't create new vrpn_Keyboard\n");
		return -1;
	}
	else
	{
		vrpn_Keyboard *device=Keyboards[num_Keyboards];
		num_Keyboards++;
	}

		return 0;
}


int vrpn_Generic_Server_Object::setup_Button_USB(char * & pch, char * line, FILE * config_file)
{
	char name[LINESIZE],deviceName[LINESIZE];

	next();
	// Get the arguments (button_name)
	if (sscanf(pch,"%511s%511s", name,deviceName) != 2) {
		fprintf(stderr,"Bad vrpn_Button_5dt_Server line: %s\n",line);
		return -1;
	}

	// Make sure there's room for a new button
	if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
		fprintf(stderr,"vrpn_Button_USB: Too many buttons in config file");
		return -1;
	}

	// Open the button
	if (verbose) {
		printf("Opening vrpn_Button_USB: %s \n",name);
	}
#ifdef	_WIN32
	if ( (buttons[num_buttons] = new vrpn_Button_USB(name,deviceName,connection))
		== NULL ) {
			fprintf(stderr,"Can't create new vrpn_Button_USB\n");
			return -1;
	} else {
		num_buttons++;
	}
#else
	printf("vrpn_Button_USB only compiled for Windows.\n");
#endif

	return 0;

}

int vrpn_Generic_Server_Object::setup_Button_5DT_Server(char * & pch, char * line, FILE * config_file)
{
        char name[LINESIZE],deviceName[LINESIZE];
        double center[16];

        next();
        // Get the arguments (button_name)
        if (sscanf(pch,"%511s%511s%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf", name,deviceName,
		&center[0],&center[1],
		&center[2],&center[3],&center[4],&center[5],&center[6],&center[7],
		&center[8],&center[9],&center[10],&center[11],&center[12],&center[13],
		&center[14],&center[15]) != 18) {
                fprintf(stderr,"Bad vrpn_Button_5dt_Server line: %s\n",line);
                return -1;
        }

        // Make sure there's room for a new button
        if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
                fprintf(stderr,"vrpn_Button_5dt_Server: Too many buttons in config file");
                return -1;
        }

        // Open the button
        if (verbose)
                printf("Opening vrpn_Button_5dt_Server: %s \n",name);
        if ( (buttons[num_buttons] = new vrpn_Button_5DT_Server(name,deviceName,connection,center))
                	== NULL ) {
		fprintf(stderr,"Can't create new vrpn_Button_5dt_Server\n");
		return -1;
        } else {
		num_buttons++;
	}

	return 0;
}

int vrpn_Generic_Server_Object::setup_Wanda (char * & pch, char * line, FILE * config_file) {
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
    if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
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

int vrpn_Generic_Server_Object::setup_Tracker_Dyna (char * & pch, char * line, FILE * config_file)
{
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
  if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
    fprintf(stderr,"Too many trackers in config file");
    return -1;
  }

  // Open the tracker
  if (verbose)
    printf("Opening vrpn_Tracker_Dyna: %s on port %s, baud %d, "
           "%d sensors\n",
          s2,s3,i1, i2);
  if ((trackers[num_trackers] =
        new vrpn_Tracker_Dyna(s2, connection, i2, s3, i1)) == NULL) {
    fprintf(stderr,"Can't create new vrpn_Tracker_Dyna\n");
    return -1;
  } else {
    num_trackers++;
  }

  return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_3DMouse (char * & pch, char * line, FILE * config_file) {
	char s2 [LINESIZE], s3 [LINESIZE], s4 [LINESIZE];
	int i1;
	int filtering_count = 1;
	int numparms;
	vrpn_Tracker_3DMouse	*mytracker;

	next();

	// Get the arguments (class, tracker_name, port, baud, [optional filtering_count])
	if ( (numparms = sscanf(pch,"%511s%511s%d%511s",s2,s3,&i1,s4)) < 3)
	{
		fprintf(stderr,"Bad vrpn_Tracker_3DMouse line: %s\n%s %s\n", line, pch, s3);
		return -1;
	}

	// See if we got the optional parameter to set the filtering count
	if (numparms == 4) {
	    filtering_count = atoi(s4);
	    if (filtering_count < 1 || filtering_count > 5)
	    {
		fprintf(stderr, "3DMouse: Bad filtering count optional param (expected 1-5, got '%s')\n",s4);
		return -1;
	    }
	}

	// Open the tracker
	if (verbose) {
		printf("Opening vrpn_Tracker_3DMouse: %s on port %s, baud %d\n", s2,s3,i1);
	}

	if ( (trackers[num_trackers] = mytracker =
		 new vrpn_Tracker_3DMouse(s2, connection, s3, i1, filtering_count)) == NULL)
	{
		fprintf(stderr, "Can't create new vrpn_Tracker_3DMouse\n");
		return -1;
        } else {
          num_trackers++;
        }

  return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Fastrak (char * & pch, char * line, FILE * config_file) {

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
        if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
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

        if ( (trackers[num_trackers] = mytracker =
             new vrpn_Tracker_Fastrak(s2, connection, s3, i1, 1, 4, rcmd, do_is900_timing))
                            == NULL){

          fprintf(stderr,"Can't create new vrpn_Tracker_Fastrak\n");
          return -1;

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
		if (mytracker->add_is900_button(lineName, lineSensor, 6)) {
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

  return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Liberty (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];
  int i1, i2;
  vrpn_Tracker_Liberty	*mytracker;
  int numparms;

        char    rcmd[5000];     // Reset command to send to Liberty
        next();
        // Get the arguments (class, tracker_name, port, baud, [whoami_len])
        numparms = sscanf(pch,"%511s%511s%d%d",s2,s3,&i1,&i2);
        if (numparms < 3) {
          fprintf(stderr,"Bad vrpn_Tracker_Liberty line: %s\n%s %s\n",
                  line, pch, s3);
          return -1;
        }


        // Make sure there's room for a new tracker
        if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
	    fprintf(stderr,"Too many trackers in config file");
            return -1;
        }

        // If the last character in the line is a backslash, '\', then
        // the following line is an additional command to send to the
        // Liberty at reset time. So long as we find lines with slashes
        // at the ends, we add them to the command string to send. Note
        // that there is a newline at the end of the line, following the
        // backslash.
        sprintf(rcmd, "");
        while (line[strlen(line)-2] == '\\') {
          // Read the next line
          if (fgets(line, LINESIZE, config_file) == NULL) {
              fprintf(stderr,"Ran past end of config file in Liberty description\n");
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
            "Opening vrpn_Tracker_Liberty: %s on port %s, baud %d\n",
            s2,s3,i1);

        if (numparms == 3) {
            if ( (trackers[num_trackers] = mytracker =
                 new vrpn_Tracker_Liberty(s2, connection, s3, i1, 0, 8, rcmd))
                                == NULL) {

              fprintf(stderr,"Can't create new vrpn_Tracker_Liberty\n");
              return -1;
            }
         } else {
            if ( (trackers[num_trackers] = mytracker =
                new vrpn_Tracker_Liberty(s2, connection, s3, i1, 0, 8, rcmd, i2))
                                == NULL) {

              fprintf(stderr,"Can't create new vrpn_Tracker_Liberty\n");
              return -1;
            }
         }

	  // If the last character in the line is a front slash, '/', then
	  // the following line is a command to add a Stylus to one
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
              fprintf(stderr,"Ran past end of config file in Liberty description\n");
                  return -1;
	    }

	    // Parse the line.
	    if (sscanf(line, "%511s%511s%d", lineCommand, lineName, &lineSensor) != 3) {
		fprintf(stderr,"Bad line in Stylus description for Liberty (%s)\n",line);
		delete trackers[num_trackers];
		return -1;
	    }

	    if (strcmp(lineCommand, "Stylus") == 0) {
		if (mytracker->add_stylus_button(lineName, lineSensor, 2)) {
		  fprintf(stderr,"Cannot set Stylus buttons for Liberty (%s)\n",line);
		  delete trackers[num_trackers];
		  return -1;
		}
		printf(" ...added Stylus (%s) to sensor %d\n", lineName, lineSensor);
	    } else {
		fprintf(stderr,"Unknown command inStylus description for Liberty (%s)\n",lineCommand);
		delete trackers[num_trackers];
		return -1;
	    }

	  }
	num_trackers++;

  return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_3Space (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];
  int i1;

            next();
                // Get the arguments (class, tracker_name, port, baud)
                if (sscanf(pch,"%511s%511s%d",s2,s3,&i1) != 3) {
                  fprintf(stderr,"Bad vrpn_Tracker_3Space line: %s\n",line);
                  return -1;
                }

                // Make sure there's room for a new tracker
                if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
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

int vrpn_Generic_Server_Object::setup_Tracker_Flock (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];
  int i1, i2, i3;
  char useERT[LINESIZE]; strcpy(useERT, "y");
  char hemi[LINESIZE]; strcpy(hemi, "+z");
  bool invertQuaternion;

  next();
  // Get the arguments (tracker_name, sensors, port, baud, invert_quat, useERT, active_hemisphere)
  int nb=sscanf(pch,"%511s%d%511s%d%d%511s%511s", s2, &i1, s3, &i2, &i3, useERT, hemi);
  if ((nb != 5) && (nb != 6) && (nb != 7)) {
    fprintf(stderr,"Bad vrpn_Tracker_Flock line: %s\n",line);
    return -1;
  }

  // Interpret the active hemisphere parameter
  int hemi_id;
  if (hemi[0] == '-')
  { if      (hemi[1] == 'x' || hemi[1] == 'X')   hemi_id = vrpn_Tracker_Flock::HEMI_MINUSX;
    else if (hemi[1] == 'y' || hemi[1] == 'Y')   hemi_id = vrpn_Tracker_Flock::HEMI_MINUSY;
    else if (hemi[1] == 'z' || hemi[1] == 'Z')   hemi_id = vrpn_Tracker_Flock::HEMI_MINUSZ;
	else {fprintf(stderr,"Bad vrpn_Tracker_Flock line: %s\n",line); return -1;}
  }
  else if (hemi[0] == '+')
  { if      (hemi[1] == 'x' || hemi[1] == 'X')   hemi_id = vrpn_Tracker_Flock::HEMI_PLUSX;
    else if (hemi[1] == 'y' || hemi[1] == 'Y')   hemi_id = vrpn_Tracker_Flock::HEMI_PLUSY;
    else if (hemi[1] == 'z' || hemi[1] == 'Z')   hemi_id = vrpn_Tracker_Flock::HEMI_PLUSZ;
	else {fprintf(stderr,"Bad vrpn_Tracker_Flock line: %s\n",line); return -1;}
  }
  else
    {fprintf(stderr,"Bad vrpn_Tracker_Flock line: %s\n",line); return -1;}


  // Make sure there's room for a new tracker
  if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
    fprintf(stderr,"Too many trackers in config file");
    return -1;
  }

  // Open the tracker
  bool buseERT=true;
  if ((useERT[0] == 'n') || (useERT[0] == 'N')) {
    buseERT = false;
  }
  invertQuaternion = (i3 != 0);
  if (verbose) {
    printf("Opening vrpn_Tracker_Flock: "
      "%s (%d sensors, on port %s, baud %d) %s ERT\n",
      s2, i1, s3,i2, buseERT ? "with" : "without");
  }
  if ( (trackers[num_trackers] =
    new vrpn_Tracker_Flock(s2,connection,i1,s3,i2, 1, buseERT, invertQuaternion, hemi_id)) == NULL) {
      fprintf(stderr,"Can't create new vrpn_Tracker_Flock\n");
      return -1;
  } else {
      num_trackers++;
  }

  return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Flock_Parallel (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];
  int i1, i2, i3;
  bool invertQuaternion;

  next();
  // Get the arguments (class, tracker_name, sensors, port, baud, invertQuaternion
  // and parallel sensor ports )

  if (sscanf(pch,"%511s%d%511s%d%d", s2,
             &i1, s3, &i2, &i3) != 5) {
    fprintf(stderr,"Bad vrpn_Tracker_Flock_Parallel line: %s\n",line);
    return -1;
  }

  // set up strtok to get the variable num of port names
  char rgch[24];
  sprintf(rgch, "%d", i2);
  char *pch2 = strstr(pch, rgch);
  strtok(pch2," \t");
  // pch points to baud, next strtok will give invertQuaternion
  strtok(NULL," \t");
  // pch points to invertQuaternion, next strtok will give first port name

  char *rgs[MAX_SENSORS];
  // get sensor ports
  for (int iSlaves=0;iSlaves<i1;iSlaves++) {
    rgs[iSlaves]=new char[LINESIZE];
    if (!(pch2 = strtok(NULL," \t"))) {
      fprintf(stderr,"Bad vrpn_Tracker_Flock_Parallel line: %s\n",
          line);
      return -1;
    } else {
      sscanf(pch2,"%511s", rgs[iSlaves]);
    }
  }

  // Make sure there's room for a new tracker
  if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
    fprintf(stderr,"Too many trackers in config file");
    return -1;
  }

  // Open the tracker
  invertQuaternion = (i3 != 0);
  if (verbose)
    printf("Opening vrpn_Tracker_Flock_Parallel: "
           "%s (%d sensors, on port %s, baud %d)\n",
          s2, i1, s3,i2);
  if ( (trackers[num_trackers] =
        new vrpn_Tracker_Flock_Parallel(s2,connection,i1,s3,i2,
                                        rgs, invertQuaternion)) == NULL){
    fprintf(stderr,"Can't create new vrpn_Tracker_Flock_Parallel\n");
    return -1;
  } else {
    num_trackers++;
  }

  return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_NULL (char * & pch, char * line, FILE * config_file) {

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
                if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
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

int vrpn_Generic_Server_Object::setup_Button_Python (char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE];
  int i1,i2;

  next();
  i2 = 0; // Set it to use the default value if we don't read a value from the file.
  // Get the arguments (class, button_name, which_lpt, optional_hex_port_number)
  if (sscanf(pch,"%511s%d%x",s2,&i1, &i2) < 2) {
    fprintf(stderr,"Bad vrpn_Button_Python line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) printf(
      "Opening vrpn_Button_Python: %s on port %d\n", s2,i1);
  if ( (buttons[num_buttons] =
       new vrpn_Button_Python(s2, connection, i1, i2)) == NULL){
    fprintf(stderr,"Can't create new vrpn_Button_Python\n");
    return -1;
  } else {
    num_buttons++;
  }

  return 0;

}

//================================
int vrpn_Generic_Server_Object::setup_Button_SerialMouse (char * & pch, char * line, FILE * config_file) {

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
    if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
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
int vrpn_Generic_Server_Object::setup_Button_PinchGlove(char* &pch, char *line, FILE *config_file) {

   char name[LINESIZE], port[LINESIZE];
   int baud;

   next();
   // Get the arguments (class, button_name, port, baud)
   if (sscanf(pch,"%511s%511s%d", name, port, &baud) != 3) {
      fprintf(stderr,"Bad vrpn_Button_PinchGlove line: %s\n",line);
      return -1;
    }

   // Make sure there's room for a new button
   if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
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
int vrpn_Generic_Server_Object::setup_Joylin (char * & pch, char * line, FILE * config_file) {
  char s2[LINESIZE];
  char s3[LINESIZE];

  // Get the arguments
  next();
  if (sscanf(pch, "%511s%511s", s2, s3) != 2) {
    fprintf(stderr, "Bad vrpn_Joylin line: %s\n", line);
    return -1;
  }

  // Make sure there's room for a new joystick server
  if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
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
int vrpn_Generic_Server_Object::setup_Joywin32 (char * & pch, char * line, FILE * config_file) {
  char s2[LINESIZE];
  int joyId;
  int readRate;
  int mode;
  int deadZone;

  // Get the arguments
  next();
  if (sscanf(pch, "%511s%d%d%d%d", s2, &joyId, &readRate, &mode, &deadZone) != 5) {
    fprintf(stderr, "Bad vrpn_Joywin32 line: %s\n", line);
    return -1;
  }
#if defined(_WIN32)
  // Make sure there's room for a new joystick server
  if (num_Win32Joys >= VRPN_GSO_MAX_WIN32JOYS) {
    fprintf(stderr, "Too many win32 joysticks devices in config file");
    return -1;
  }

  // Open the joystick server
  if (verbose)
    printf("Opening vrpn_Joywin32: %s (device %d), baud rate:%d, mode:%d, dead zone:%d\n", s2, joyId, readRate, mode, deadZone);
  if ((win32joys[num_Win32Joys] =
       new vrpn_Joywin32(s2, connection, joyId, readRate, mode, deadZone)) == NULL) {
    fprintf(stderr, "Can't create new vrpn_Joywin32\n");
    return -1;
  } else {
    num_Win32Joys++;
  }
  return 0;
#else
    fprintf(stderr, "Joywin32 is for use under Windows only");
	return -2;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_Tng3 (char * & pch, char * line, FILE * config_file) {
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
	if (num_tng3s >= VRPN_GSO_MAX_TNG3S) {
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
int vrpn_Generic_Server_Object::setup_Mouse (char * & pch, char * line, FILE * config_file) {
    char s2 [LINESIZE];
    vrpn_Mouse * mouse;
    next();

    // Get the arguments (class, mouse_name)
    if (sscanf(pch,"%511s",s2) != 1) {
        fprintf(stderr,"Bad vrpn_Mouse line: %s\n",line);
        return -1;
    }

    // Make sure there's room for a new mouse
    if (num_mouses >= VRPN_GSO_MAX_MOUSES) {
        fprintf(stderr,"Too many mouses (mice) in config file");
        return -1;
    }

    // Open the box
    if (verbose)
        printf("Opening vrpn_Mouse: %s\n",s2);

    try {
	    mouse = new vrpn_Mouse(s2, connection);
    }
    catch (...) {
	fprintf( stderr, "could not create vrpn_Mouse\n" );
#ifdef linux
        fprintf( stderr, "- Is the GPM server running?\n" );
	fprintf( stderr, "- Are you running on a linux console (not an xterm)?\n" );
#endif
	return -1;
    }
    if (NULL == mouse) {
        fprintf(stderr,"Can't create new vrpn_Mouse\n");
        return -1;
    }
    mouses[num_mouses++] = mouse;
    return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_Tracker_Crossbow (char * & pch, char *line, FILE *config_file) {
        char port[LINESIZE], name[LINESIZE];
        long baud;
        float gRange, aRange;

        next();

        // Get the arguments (class, tracker_name, port, baud, g-range, a-range)
        // g-range is the linear acceleration sensitivity in Gs
        // a-range is the angular velocity sensitivity in degrees per second
        if (sscanf(pch, "%511s%511s%ld%f%f", name, port, &baud, &gRange, &aRange) < 5) {
                fprintf(stderr, "Bad vrpn_Tracker_Crossbow line: %s\n%s %s\n", line, pch, port);
                return -1;
        }

        if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
                fprintf(stderr, "Too many trackers in config file");
                return -1;
        }

        if (verbose)
                printf("Opening vrpn_Tracker_Crossbow: %s on %s with baud %ld, G-range %f, and A-range %f\n",
                        name, port, baud, gRange, aRange);

        if (!(trackers[num_trackers] = new vrpn_Tracker_Crossbow(name, connection, port, baud, gRange, aRange))) {
                fprintf(stderr, "Can't create new vrpn_Tracker_Crossbow\n");
                return -1;
        }
        else {
                num_trackers++;
        }

        return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_3DMicroscribe(char * & pch, char * line, FILE * config_file)
{
        char name [LINESIZE], device [LINESIZE];
        int baud_rate;
        float x,y,z,s;

        next();
        // Get the arguments (class, 5DT_name, port, baud, xoff, yoff, zoff, scale)
        if (sscanf(pch,"%511s%511s%d%f%f%f%f",name,device, &baud_rate,&x,&y,&z,&s) != 7) {
                fprintf(stderr,"Bad vrpn_3dMicroscribe line: %s\n",line);
                return -1;
        }
        // Make sure there's room for a new tracker
        if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
                fprintf(stderr,"Too many Trackers in config file");
                return -1;
        }

        // Open the device
        if (verbose) {
                printf("Opening 3dMicroscribe: %s on port %s, baud %d\n",
                        name,device,baud_rate);
        }
#ifdef VRPN_USE_MICROSCRIBE
        if ((trackers[num_trackers] =
                new vrpn_3DMicroscribe (name, connection, device, baud_rate,x,y,z,s )) == NULL) {
                        fprintf(stderr,"Can't create new vrpn_3DMicroscribe\n");
                        return -1;
                } else {
                        num_trackers++;
                }
#else
	fprintf(stderr,"3DMicroscribe support not configured in VRPN, edit vrpn_Configure.h and rebuild\n");
#endif
	return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_Tracker_InterSense(char * &pch, char *line, FILE * config_file) {
#ifdef	VRPN_INCLUDE_INTERSENSE
  char trackerName[LINESIZE];
  char commStr[100];
  int commPort;

  char s4 [LINESIZE];
  char s5 [LINESIZE];
  char s6 [LINESIZE];
  int numparms;
  vrpn_Tracker_InterSense	*mytracker;
  int do_is900_timing = 0;
  int reset_at_start = 0;		// nahon@virtools.com

  char    rcmd[5000];     // Reset command to send to Intersense

  next();

  // Get the arguments (class, tracker_name, port, [optional IS900time])
  sscanf(line,"vrpn_Tracker_InterSense %s %s",trackerName,commStr);
  if ( (numparms = sscanf(pch,"%511s%511s%511s%511s%511s",trackerName,commStr,s4,s5,s6)) < 2) {
	fprintf(stderr,"Bad vrpn_Tracker_InterSense line: %s\n%s %s\n",
            line, pch, trackerName);
    return -1;
  }

  if(!strcmp(commStr,"COM1")) {
    commPort = 1;
  } else if(!strcmp(commStr,"COM2")) {
    commPort = 2;
  } else if(!strcmp(commStr,"COM3")) {
    commPort = 3;
  } else if(!strcmp(commStr,"COM4")) {
    commPort = 4;
  }	else if(!strcmp(commStr,"AUTO")) {
    commPort = 0; //the intersense SDK will find the tracker on any COM or USB port.
  } else {
    fprintf(stderr,"Error determining COM port: %s not should be either COM1, COM2, COM3, COM4 or AUTO",commStr);
    return -1;
  }
  int found_a_valid_option=0;
  // See if we got the optional parameter to enable IS900 timings
  if (numparms >= 3) {
    if ( (strncmp(s4, "IS900time", strlen("IS900time")) == 0) ||
		 (strncmp(s5, "IS900time", strlen("IS900time")) == 0) ||
		 (strncmp(s6, "IS900time", strlen("IS900time")) == 0)
	) {
	  do_is900_timing = 1;
	  found_a_valid_option = 1;
	  printf(" ...using IS900 timing information\n");
    }
    if (
	 (strncmp(s4, "ResetAtStartup", strlen("ResetAtStartup")) == 0) ||
	 (strncmp(s5, "ResetAtStartup", strlen("ResetAtStartup")) == 0) ||
	 (strncmp(s6, "ResetAtStartup", strlen("ResetAtStartup")) == 0)
	) {
	  reset_at_start = 1;
	  found_a_valid_option = 1;
	  printf(" ...Sensor will reset at startup\n");
    }
    if (strcmp(s4, "/") == 0) {
		found_a_valid_option = 1;
    }
    if (strcmp(s5, "/") == 0) {
		found_a_valid_option = 1;
    }
    if (strcmp(s6, "/") == 0) {
		found_a_valid_option = 1;
    }
    if (strcmp(s4, "\\") == 0) {
		found_a_valid_option = 1;
    }
    if (strcmp(s5, "\\") == 0) {
		found_a_valid_option = 1;
    }
    if (strcmp(s6, "\\") == 0) {
		found_a_valid_option = 1;
    }
    if (!found_a_valid_option) {
	fprintf(stderr,"InterSense: Bad optional param (expected 'IS900time', 'ResetAtStartup' and/or 'SensorsStartsAtOne', got '%s %s %s')\n",s4,s5,s6);
	return -1;
    }
  }

    // Make sure there's room for a new tracker
    if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
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
              fprintf(stderr,"Ran past end of config file in Isense description\n");
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
    if (verbose)
		printf("Opening vrpn_Tracker_InterSense: %s on port %s\n",
               trackerName, commStr);

     if ( (trackers[num_trackers] = mytracker =
             new vrpn_Tracker_InterSense(trackerName, connection, commPort, rcmd, do_is900_timing,
				reset_at_start)) == NULL){
          fprintf(stderr,"Can't create new vrpn_Tracker_InterSense\n");
          return -1;

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
              fprintf(stderr,"Ran past end of config file in InterSense description\n");
                  return -1;
		    }

			// Parse the line.  Both "Wand" and "Stylus" lines start with the name and sensor #
			if (sscanf(line, "%511s%511s%d", lineCommand, lineName, &lineSensor) != 3) {
				fprintf(stderr,"Bad line in Wand/Stylus description for InterSense (%s)\n",line);
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
					fprintf(stderr,"Bad line in Wand description for InterSense (%s)\n",line);
					delete trackers[num_trackers];
					return -1;
				}

				if (mytracker->add_is900_analog(lineName, lineSensor, c0min, c0lo0, c0hi0, c0max,
												c1min, c1lo0, c1hi0, c1max)) {
					fprintf(stderr,"Cannot set Wand analog for InterSense (%s)\n",line);
					delete trackers[num_trackers];
					return -1;
				}
				if (mytracker->add_is900_button(lineName, lineSensor, 6)) {
					fprintf(stderr,"Cannot set Wand buttons for InterSense (%s)\n",line);
					delete trackers[num_trackers];
					return -1;
				}
				printf(" ...added Wand (%s) to sensor %d\n", lineName, lineSensor);

			} else if (strcmp(lineCommand, "Stylus") == 0) {
				if (mytracker->add_is900_button(lineName, lineSensor, 2)) {
					fprintf(stderr,"Cannot set Stylus buttons for InterSense (%s)\n",line);
					delete trackers[num_trackers];
					return -1;
				}
				printf(" ...added Stylus (%s) to sensor %d\n", lineName, lineSensor);
			} else {
				fprintf(stderr,"Unknown command in Wand/Stylus description for InterSense (%s)\n",lineCommand);
				delete trackers[num_trackers];
				return -1;
			}
	  }

      num_trackers++;
  }

#else
	fprintf(stderr, "vrpn_server: Can't open Intersense native server: VRPN_INCLUDE_INTERSENSE not defined in vrpn_Configure.h!\n");
	return -1;
#endif

  return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_DirectXFFJoystick (char * & pch, char * line, FILE * config_file) {
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
  if (num_DirectXJoys >= VRPN_GSO_MAX_DIRECTXJOYS) {
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


//================================
int vrpn_Generic_Server_Object::setup_RumblePad (char * & pch, char * line, FILE * config_file) {
#ifdef	VRPN_USE_DIRECTINPUT
  char s2 [LINESIZE];

  next();
  // Get the arguments (joystick_name, read update rate, force update rate)
  if (sscanf(pch,"%511s",s2) != 1) {
    fprintf(stderr,"Bad vrpn_DirectXRumblePad line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new joystick
  if (num_RumblePads >= VRPN_GSO_MAX_RUMBLEPADS) {
    fprintf(stderr,"Too many Direct X Rumble Pads in config file");
    return -1;
  }

  // Open the joystick
  if (verbose) {
    printf("Opening vrpn_DirectXRumblePad: %s\n", s2);
  }
  if ((RumblePads[num_RumblePads] = new vrpn_DirectXRumblePad(s2, connection)) == NULL)
  {
    fprintf(stderr,"Can't create new vrpn_DirectXRumblePad\n");
    return -1;
  } else {
    num_RumblePads++;
  }

  return 0;
#else
  fprintf(stderr, "vrpn_server: Can't open RumblePad: VRPN_USE_DIRECTINPUT not defined in vrpn_Configure.h!\n");
  return -1;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_XInputPad (char * & pch, char * line, FILE * config_file) {
#ifdef	VRPN_USE_DIRECTINPUT
  char s2 [LINESIZE];
  unsigned controller;

  next();
  // Get the arguments (joystick_name, controller index)
  if (sscanf(pch,"%511s%u",s2,&controller) != 2) {
    fprintf(stderr,"Bad vrpn_XInputGamepad line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new joystick
  if (num_XInputPads >= VRPN_GSO_MAX_XINPUTPADS) {
    fprintf(stderr,"Too many XInput gamepads in config file");
    return -1;
  }

  // Open the joystick
  if (verbose) {
    printf("Opening vrpn_XInputGamepad: %s\n", s2);
  }
  if ((XInputPads[num_XInputPads] = new vrpn_XInputGamepad(s2, connection, controller)) == NULL)
  {
	  fprintf(stderr,"Can't create new vrpn_XInputGamepad\n");
    return -1;
  } else {
    num_XInputPads++;
  }

  return 0;
#else
  fprintf(stderr, "vrpn_server: Can't open XInputGamepad: VRPN_USE_DIRECTINPUT not defined in vrpn_Configure.h!\n");
  return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_GlobalHapticsOrb (char * & pch, char * line, FILE * config_file) {
  char s2[LINESIZE], s3[LINESIZE];
  int  i1;

  next();
  // Get the arguments (orb_name, port name, baud rate)
  if (sscanf(pch,"%511s%s%d",s2,s3, &i1) != 3) {
    fprintf(stderr,"Bad vrpn_GlobalHapticsOrb line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new orb
  if (num_GlobalHapticsOrbs >= VRPN_GSO_MAX_GLOBALHAPTICSORBS) {
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

//================================
int vrpn_Generic_Server_Object::setup_ADBox(char* &pch, char *line, FILE *config_file) {

  char name[LINESIZE], port[LINESIZE];
  int baud;

  next();
  // Get the arguments (class, button_name, port, baud)
  if (sscanf(pch,"%511s%511s%d", name, port, &baud) != 3) {
    fprintf(stderr,"Bad vrpn_ADBox line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_ADBox : Too many buttons in config file");
    return -1;
  }

  // Make sure there's room for a new analog
  if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
    fprintf(stderr,"vrpn_ADBox : Too many analogs in config file");
    return -1;
  }

  // Open the button
  if (verbose)
    printf("Opening vrpn_ADBox: %s on port %s at %d baud\n",name,port,baud);

  if ( (buttons[num_buttons] = new vrpn_ADBox(name,connection,port,baud))
       == NULL ) {
    fprintf(stderr,"Can't create new vrpn_ADBox\n");
    return -1;
  } else {

    analogs[num_analogs] = (vrpn_Analog*)buttons[num_buttons];
    num_buttons++;
    num_analogs++;
  }

  return 0;
}

int vrpn_Generic_Server_Object::setup_VPJoystick(char* &pch, char *line, FILE *config_file) {

  char name[LINESIZE], port[LINESIZE];
  int baud;

  next();
  // Get the arguments (class, button_name, port, baud)
  if (sscanf(pch,"%511s%511s%d", name, port, &baud) != 3) {
    fprintf(stderr,"Bad vrpn_VPJoystick line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_ADBox : Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose)
    printf("Opening vrpn_VPJoystick: %s on port %s at %d baud\n",name,port,baud);

  if ( (buttons[num_buttons] = new vrpn_VPJoystick(name,connection,port,baud))
       == NULL ) {
    fprintf(stderr,"Can't create new vrpn_VPJoystick\n");
    return -1;
  } else {

    num_buttons++;
  }

  return 0;
}


int vrpn_Generic_Server_Object::setup_DTrack (char* &pch, char* line, FILE* config_file)
{
	char* s2;
	char* str[LINESIZE];
	char *s;
	char sep[] = " ,\t,\n";
	int  count = 0;
	int dtrackPort;
	float timeToReachJoy;
	int nob, nof, nidbf;
	int idbf[VRPN_GSO_MAX_TRACKERS];
	bool actTracing;

	next();

	// Get the arguments:

	str[count] = strtok( pch, sep );
	while( str[count] != NULL )
	{
		count++;
		// Get next token:
		str[count] = strtok( NULL, sep );
	}

	if(count < 2){
		fprintf(stderr,"Bad vrpn_Tracker_DTrack line: %s\n",line);
		return -1;
	}

	// Make sure there's room for a new one:

	if (num_DTracks >= VRPN_GSO_MAX_DTRACKS) {
		fprintf(stderr,"Too many DTracks in config file");
		return -1;
	}

	s2 = str[0];
	dtrackPort = (int )strtol(str[1], &s, 0);

	// tracing (optional; always last argument):

	actTracing = false;

	if(count > 2){
		if(!strcmp(str[count-1], "-")){
			actTracing = true;
			count--;
		}
	}

	// time needed to reach the maximum value of the joystick (optional):

	timeToReachJoy = 0;

	if(count > 2){
		timeToReachJoy = (float )strtod(str[2], &s);
	}

	// number of bodies and flysticks (optional):

	nob = nof = -1;

	if(count > 3){
		if(count < 5){  // there must be two additional arguments at least
			fprintf(stderr, "Bad vrpn_Tracker_DTrack line: %s\n", line);
			return -1;
		}

		nob = (int )strtol(str[3], &s, 0);
		nof = (int )strtol(str[4], &s, 0);
	}

	// renumbering of targets (optional):

	nidbf = 0;

	if(count > 5){
		if(count < 5 + nob + nof){  // there must be an argument for each target
			fprintf(stderr, "Bad vrpn_Tracker_DTrack line: %s\n", line);
			return -1;
		}

		for(int i=0; i<nob+nof; i++){
			idbf[i] = (int )strtol(str[5+i], &s, 0);
			nidbf++;
		}
	}

	// Open vrpn_Tracker_DTrack:

	int* pidbf = NULL;

	if(nidbf > 0){
		pidbf = idbf;
	}

	if(verbose){
		printf("Opening vrpn_Tracker_DTrack: %s at port %d, timeToReachJoy %.2f", s2, dtrackPort, timeToReachJoy);
		if(nob >= 0 && nof >= 0){
			printf(", fixNtargets %d %d", nob, nof);
		}
		if(nidbf > 0){
			printf(", fixId");
			for(int i=0; i<nidbf; i++){
				printf(" %d", idbf[i]);
			}
		}
		printf("\n");
	}

	if((DTracks[num_DTracks] = new vrpn_Tracker_DTrack(s2, connection, dtrackPort, timeToReachJoy,
		                                                nob, nof, pidbf, actTracing)) == NULL)
	{
		fprintf(stderr,"Can't create new vrpn_Tracker_DTrack\n");
		return -1;
	}

	num_DTracks++;

	return 0;
}

// This function will read one line of the vrpn_Poser_Analog configuration (matching
// one axis) and fill in the data for that axis. The axis name, the file to read
// from, and the axis to fill in are passed as parameters. It returns 0 on success
// and -1 on failure.

int	vrpn_Generic_Server_Object::get_poser_axis_line(FILE *config_file, char *axis_name, vrpn_PA_axis *axis, vrpn_float64 *min, vrpn_float64 *max)
{
	char	line[LINESIZE];
	char	_axis_name[LINESIZE];
	char	*name = new char[LINESIZE];	// We need this to stay around for the param
	int	channel;
	float	offset, scale;

	// Read in the line
	if (fgets(line, LINESIZE, config_file) == NULL) {
		perror("Poser Analog Axis: Can't read axis");
		return -1;
	}

	// Get the values from the line
	if (sscanf(line, "%511s%511s%d%g%g%lg%lg", _axis_name, name,
			&channel, &offset, &scale, min, max) != 7) {
		fprintf(stderr,"Poser Analog Axis: Bad axis line\n");
		return -1;
	}

	// Check to make sure the name of the line matches
	if (strcmp(_axis_name, axis_name) != 0) {
		fprintf(stderr,"Poser Analog Axis: wrong axis: wanted %s, got %s)\n",
			axis_name, name);
		return -1;
	}

	// Fill in the values if we didn't get the name "NULL". Otherwise, just
	// leave them as they are, and they will have no effect.
	if (strcmp(name,"NULL") != 0) {
	  axis->ana_name = name;
	  axis->channel = channel;
	  axis->offset = offset;
	  axis->scale = scale;
	}

	return 0;
}

int vrpn_Generic_Server_Object::setup_Poser_Analog (char * & pch, char * line, FILE * config_file)
{
    char s2 [LINESIZE];
    int  i1;
    vrpn_Poser_AnalogParam     p;

    next();
    if (sscanf(pch, "%511s%d",s2,&i1) != 2) {
            fprintf(stderr, "Bad vrpn_Poser_Analog line: %s\n",
		line);
            return -1;
    }

    // Make sure there's room for a new poser
    if (num_posers >= VRPN_GSO_MAX_POSER) {
      fprintf(stderr,"Too many posers in config file");
      return -1;
    }

    if (verbose) {
      printf("Opening vrpn_Poser_Analog: "
             "%s\n",s2);
    }

    // Scan the following lines in the configuration file to fill
    // in the start-up parameters for the different axis.

    if (get_poser_axis_line(config_file,"X", &p.x, &p.pos_min[0], &p.pos_max[0])) {
            fprintf(stderr,"Can't read X line for Poser Analog\n");
            return -1;
    }
    if (get_poser_axis_line(config_file,"Y", &p.y, &p.pos_min[1], &p.pos_max[1])) {
            fprintf(stderr,"Can't read y line for Poser Analog\n");
            return -1;
    }
    if (get_poser_axis_line(config_file,"Z", &p.z, &p.pos_min[2], &p.pos_max[2])) {
            fprintf(stderr,"Can't read Z line for Poser Analog\n");
            return -1;
    }
    if (get_poser_axis_line(config_file,"RX", &p.rx, &p.pos_rot_min[0], &p.pos_rot_max[0])) {
            fprintf(stderr,"Can't read RX line for Poser Analog\n");
            return -1;
    }
    if (get_poser_axis_line(config_file,"RY", &p.ry, &p.pos_rot_min[1], &p.pos_rot_max[1])) {
            fprintf(stderr,"Can't read RY line for Poser Analog\n");
            return -1;
    }
    if (get_poser_axis_line(config_file,"RZ", &p.rz, &p.pos_rot_min[2], &p.pos_rot_max[2])) {
            fprintf(stderr,"Can't read RZ line for Poser Analog\n");
            return -1;
    }

    posers[num_posers] = new
       vrpn_Poser_Analog(s2, connection, &p, i1 != 0);

    if (!posers[num_posers]) {
      fprintf(stderr,"Can't create new vrpn_Poser_Analog\n");
      return -1;
    } else {
      num_posers++;
    }

    return 0;
}

int vrpn_Generic_Server_Object::setup_nikon_controls (char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE], s3 [LINESIZE];

    // Get the arguments
    next();
    if (sscanf(pch, "%511s%511s", s2, s3) != 2) {
      fprintf(stderr, "Bad vrpn_nikon_controls line: %s\n", line);
      return -1;
    }

    // Make sure there's room for a new server
    if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
      fprintf(stderr, "Too many analog devices in config file");
      return -1;
    }

    // Open the server
    if (verbose)
      printf("Opening vrpn_nikon_control %s on port %s \n",
             s2,s3);
    if ((analogs[num_analogs] =
          new vrpn_Nikon_Controls(s2, connection,s3)) == NULL) {
        fprintf(stderr, "Can't create new vrpn_nikon_controls\n");
        return -1;
    } else {
        num_analogs++;
    }

  return 0;
}

int vrpn_Generic_Server_Object::setup_Poser_Tek4662 (char * & pch, char * line, FILE * config_file)
{
    char s2 [LINESIZE], s3 [LINESIZE];
    int i1;

    next();
    if ( sscanf(pch,"%511s%511s%d",s2,s3, &i1) != 3) {
      fprintf(stderr, "Bad vrpn_Poser_Tek4662 line: %s\n", line);
      return -1;
    }

    // Make sure there's room for a new poser
    if (num_posers >= VRPN_GSO_MAX_POSER) {
      fprintf(stderr,"Too many posers in config file");
      return -1;
    }

    if (verbose) {
      printf("Opening vrpn_Poser_Tek4662 %s on port %s at baud %d\n"
             ,s2, s3, i1);
    }

    posers[num_posers] = new vrpn_Poser_Tek4662(s2, connection, s3, i1);

    if (!posers[num_posers]) {
      fprintf(stderr,"Can't create new vrpn_Poser_Tek4662\n");
      return -1;
    } else {
      num_posers++;
    }

    return 0;
}

// ----------------------------------------------------------------------
// BUW additions
// ----------------------------------------------------------------------

/******************************************************************************/
/* setup Atmel microcontroller */
/******************************************************************************/
int vrpn_Generic_Server_Object::setup_Atmel(char* &pch, char *line, FILE *config_file)
{
#ifndef _WIN32
  char name[LINESIZE];
  char port[LINESIZE];
  int baud = 0;
  int channel_count = 0;

  next();

  // first line
  if (setup_vrpn_Atmel::channel_count == 0) {

    // Get the arguments
    if (sscanf(pch,"%511s%511s%d%d", name, port, &baud, &channel_count) != 4) {
      fprintf(stderr,"Bad vrpn_Atmel line: %s\n",line);
      return -1;
    }
    else {
      if (verbose) {
        printf("name: %s\n",name);
        printf("port: %s\n",port);
        printf("baud: %d\n",baud);
        printf("channel_count: %d\n",channel_count);
      }
    }

    // Make sure there's room for a new analog
    if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
      fprintf(stderr,"vrpn_Atmel : Too many analogs in config file");
      return -1;
    }

    setup_vrpn_Atmel::channel_count = channel_count;

    // init the channel_mode array
    for(int k=0; k<channel_count; ++k)
      setup_vrpn_Atmel::channel_mode[k] = VRPN_ATMEL_MODE_NA;

    return 0;

  } // end of handline first line

  //*********************************************************

  //*********************************************************


  int channel;
  char mode[3];

  // Get the arguments
  if (sscanf(pch,"%d%511s", &channel, mode) != 2) {
    fprintf(stderr,"Bad vrpn_Atmel line: %s\n",line);
    return -1;
  }
  else {
    if (verbose) {
      printf("channel: %d - mode: %s\n",channel,mode);
    }
  }

  // check if it is a valid channel
  if (channel >= setup_vrpn_Atmel::channel_count)  {
    fprintf(stderr,"channel value out of range\n\n");
    return -1;
  }

  //**************************************************
  //last line of vrpn_Atmel
  if (channel == -1) {

    // here we use a factory interface because a lot of init things have to be done
    vrpn_Analog * self = vrpn_Atmel::Create( name,
                                             connection,
                                             port,
                                             baud,
                                             channel_count ,
                                             setup_vrpn_Atmel::channel_mode);

    // reset the params so that another atmel can be configured
    setup_vrpn_Atmel::channel_count = 0;

    // check if instance has been created
    if (self == NULL ) {

      fprintf(stderr,"Can't create new vrpn_Atmel\n\n");

      return -1;
     }
     else {

      printf("\nAtmel %s started.\n\n", name);

      // the Analog_Output is handled implict by analog like done in Zaber

      analogs[num_analogs] = (vrpn_Analog *) self;
      num_analogs++;

      return 0;
    }
  }

  // check if it is a valid channel
  if (channel < 0)  {
    fprintf(stderr,"channel value out of range\n\n");
    return -1;
  }

  // channel init line

  //**************************************************
  //set the mode array
  int mode_int;

  #define is_mode(s) !strcmp(pch=strtok(mode," \t"),s)

  // convert the char * in an integer
  if (is_mode("RW"))
    mode_int = VRPN_ATMEL_MODE_RW;
  else if (is_mode("RO"))
    mode_int = VRPN_ATMEL_MODE_RO;
  else if (is_mode("WO"))
    mode_int = VRPN_ATMEL_MODE_WO;
  else if (is_mode("NA")) {
    mode_int = VRPN_ATMEL_MODE_NA;
  }
  else {
    fprintf(stderr,"unknown io-mode: %s\n\n", mode);
    return -1;
  }

  // write it to the array
  setup_vrpn_Atmel::channel_mode[channel] = mode_int;

#else
  fprintf(stderr,"vrpn_Generic_Server_Object::setup_Atmel(): Not implemented on this architecture\n");
#endif

  return 0;
}


/******************************************************************************/
/* setup mouse connected via event interface */
/******************************************************************************/
int vrpn_Generic_Server_Object::setup_Event_Mouse(char* &pch, char *line, FILE *config_file)
{

  char name[LINESIZE], port[LINESIZE];

  next();
  // Get the arguments (class, button_name, port)
  if (sscanf(pch,"%511s%511s", name, port) != 2) {
    fprintf(stderr,"Bad vrpn_Event_Mouse line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_Event_Mouse : Too many buttons in config file");
    return -1;
  }

  // Make sure there's room for a new analog
  if (num_analogs >= VRPN_GSO_MAX_ANALOG) {
    fprintf(stderr,"vrpn_Event_Mouse : Too many analogs in config file");
    return -1;
  }

  // Open the button
  if (verbose) {

    printf("Opening vrpn_Event_Mouse: %s on port %s\n",name,port);
  }

  buttons[num_buttons] = new vrpn_Event_Mouse( name, connection, port);

  analogs[num_analogs] = (vrpn_Analog*)buttons[num_buttons];
  num_buttons++;
  num_analogs++;

  return 0;
}

/*
 *  inertiamouse config file setup routine
 *
 */
int vrpn_Generic_Server_Object::setup_inertiamouse (char * & pch, char * line, FILE * config_file)
{
    char name[LINESIZE], port[LINESIZE];
    int baud;
    int ret;

    next();

    // Get the arguments (class, magellan_name, port, baud
    if ( (ret = sscanf(pch,"%511s%511s%d",name, port, &baud)) < 3) {
        fprintf (stderr,"Bad vrpn_intertiamouse line: %s\n",line);
        return -1;
    }


    // Make sure there's room for a new magellan
    if (num_magellans >= VRPN_GSO_MAX_INERTIAMOUSES) {
        fprintf (stderr,"Too many intertiamouses in config file\n");
        return -1;
    }

  // Open the device
  if (verbose) {
    printf("Opening vrpn_inertiamouse: %s on port %s, baud %d\n",
           name,
           port,
           baud);
  }
  if (!(inertiamouses[num_inertiamouses] =
      vrpn_inertiamouse::create(name,
                                connection,
                                port,
                                baud))) {
      fprintf(stderr,"Can't create new vrpn_inertiamouse\n");
      return -1;
  } else {
      ++num_inertiamouses;
  }

  return 0;
}


// ----------------------------------------------------------------------


int vrpn_Generic_Server_Object::setup_Analog_USDigital_A2 (char * & pch, char * line, FILE * config_file)
{
#ifdef  VRPN_USE_USDIGITAL
    char A2name[LINESIZE];
    int  comPort, numChannels, numArgs, reportChange;

    next();
    // Get the arguments (class, USD_A2_name, comPort, numChannels, [reportChange]
    numArgs = sscanf(pch,"%511s%d%d%d", A2name, &comPort, &numChannels, &reportChange) ;
    if (numArgs!=3 && numArgs!=4)
    {
        fprintf(stderr,"Bad vrpn_Analog_USDigital_A2 line: %s\n",line);
        return -1;
    }

    // Handle optional parameter
    if (numArgs==3)
    {
        reportChange = 0 ;
    }

    // Make sure the parameters look OK
    if (comPort<0 || comPort>100)
    {
          fprintf(stderr,"Invalid COM port %d in vrpn_Analog_USDigital_A2 line: %s\n",comPort, line);
          return -1;
    }
    if (numChannels<1 ||
        (vrpn_uint32) numChannels>vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_CHANNEL_MAX ||
        numChannels>vrpn_CHANNEL_MAX)
    {
          fprintf(stderr,"Invalid number of channels %d in vrpn_Analog_USDigital_A2 line: %s\n",
          numChannels, line);
          return -1;
    }

    // Make sure there's room for a new analog
    if (num_analogs >= VRPN_GSO_MAX_ANALOG)
    {
        fprintf(stderr,"Too many Analogs in config file");
        return -1;
    }

    // Open the device
    if (verbose)
        printf(
      "Opening vrpn_Analog_USDigital_A2: %s on port %d (%u=search for port), with %d channels, reporting %s\n",
               A2name,comPort,vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_FIND_PORT,
               numChannels, (reportChange==0)?"always":"on change");
    if ((analogs[num_analogs] =
         new vrpn_Analog_USDigital_A2(A2name, connection, (vrpn_uint32) comPort,
         (vrpn_uint32) numChannels,
         (reportChange!=0))) == NULL)
    {
        fprintf(stderr,"Can't create new vrpn_Analog_USDigital_A2\n");
        return -1;
    }
    else
    {
        num_analogs++;
    }

    return 0;
#else
	printf("Warning:  Server not compiled with  VRPN_USE_USDIGITAL defined.\n");
	return -1;
#endif
}    //  setup_USDigital_A2


int vrpn_Generic_Server_Object::setup_Button_NI_DIO24 (char * & pch, char * line, FILE * config_file)
{
#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
    char DIO24name[LINESIZE];
    int  numChannels ;
    int  numArgs ;

    next();
    // Get the arguments (class, D24_name, numChannels)
    numArgs = sscanf(pch,"%511s%d%d", DIO24name, &numChannels) ;
    if (numArgs != 1 && numArgs != 2)
    {
        fprintf(stderr,"Bad vrpn_Button_NI_DIO24 line: %s\n",line);
        return -1;
    }

    //  Do error checking on numChannels
    if (numArgs>1 &&
        (numChannels<1 ||
        numChannels>vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24_CHANNEL_MAX ||
        numChannels>vrpn_CHANNEL_MAX))
    {
        fprintf(stderr,"Invalid number of channels %d in vrpn_Button_NI_DIO24 line: %s\n",
                numChannels, line);
        return -1;
    }

    // Make sure there's room for a new button
    if (num_buttons >= VRPN_GSO_MAX_BUTTONS)
    {
        fprintf(stderr,"Too many Buttons in config file");
        return -1;
    }

    //  if numChannels is wrong, use default
    if (numArgs<2 || numChannels<1)
        numChannels = vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24_CHANNEL_MAX ;

    // Open the device
    if (verbose)
        printf("Opening vrpn_Button_NI_DIO24: %s with up to %d buttons\n",
               DIO24name, numChannels);

    if ((buttons[num_buttons] =
         new vrpn_Button_NI_DIO24(DIO24name, connection, numChannels)) == NULL)
    {
        fprintf(stderr,"Can't create new vrpn_Button_NI_DIO24\n");
        return -1;
    }
    else
    {
        num_buttons++;
    }

    return 0;
#else
	printf("Warning:  Server not compiled with VRPN_USE_NATIONAL_INSTRUMENTS_MX defined.\n");
	return -1;
#endif

}    //  setup_Button_NI_DIO24

int vrpn_Generic_Server_Object::setup_Tracker_PhaseSpace (char * & pch, char * line, FILE * config_file) 
{
#ifdef VRPN_INCLUDE_PHASESPACE

  char trackerName[LINESIZE];
  char device[LINESIZE];
  float framerate = 0;
  int readflag = 0;
  int slaveflag = 0;
	
  //get tracker name and device
  if( sscanf(line,"vrpn_Tracker_PhaseSpace %s %s %f %d %d",trackerName,device,&framerate,&readflag,&slaveflag) < 5)
    {
      fprintf(stderr,"Bad vrpn_Tracker_PhaseSpace line: %s\nProper format is:  vrpn_Tracker_Phasespace [trackerName] [device] [framerate] [readflag] [slaveflag]\n", line);
      return -1;
    }

  // Make sure there's room for a new tracker
  if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
    fprintf(stderr,"Too many trackers in config file");
    return -1;
  }
  
  vrpn_Tracker_PhaseSpace* pstracker =  new vrpn_Tracker_PhaseSpace(trackerName, connection, device, framerate, readflag, slaveflag);

  if(pstracker == NULL)
    {
      fprintf(stderr,"Unable to create new vrpn_Tracker_PhaseSpace.\n");
      return -1;
    }

  trackers[num_trackers] = pstracker;
  num_trackers++;

  char tag[LINESIZE];
  int sensor = 0;
  int id = 0;
  float x = 0;
  float y = 0;
  float z = 0;
  bool inTag = false;

  //read file for markers and rigid body specifications     
  //Parse these even if they aren't used in slave mode just to consume their place in the input stream.
  while ( fgets(line, LINESIZE, config_file) != NULL ) {  

    //cut off comments
    for(int i = 0; i < LINESIZE && line[i] != '\0'; i++)
      {
        if(line[i] == '#')
          {
            line[i] = '\0';
            break;
          }
      }

    //read tags and params
    if(sscanf(line,"%s",tag) == 1)
      {
        if(strcmp("<owl>",tag) == 0)
          {
            if(inTag)
              {
                fprintf(stderr,"Error, nested <owl> tag encountered.  Aborting...\n");
                return -1;
              }
            else
              {
                inTag = true;
                continue;
              }
          }
        else if(strcmp("</owl>",tag) == 0)
          {
            if(inTag)
              {
                inTag = false;
                break;
              }
            else
              {
                fprintf(stderr,"Error, </owl> tag without <owl> tag.  Aborting...\n");
                return-1;
              }
          }
      }
    if(inTag)
      {
        if(sscanf(line,"%d : rb+ %d %f %f %f", &sensor,&id,&x,&y,&z) == 5)
          {
            if(!pstracker->addRigidMarker(sensor,id,x,y,z))
              {
                fprintf(stderr,"Error, unable to add new rigid body marker: %d:%d %f %f %f\n",sensor,id,x,y,z);
                continue;
              }
          }
        else if(sscanf(line,"%d : pt %d", &sensor,&id) == 2)
          {
            if(!pstracker->addMarker(sensor,id))
              {
                fprintf(stderr,"Error, unable to add marker %d:%d\n",sensor,id);
                continue;
              }
          }
        else if(sscanf(line,"%d : rbnew", &sensor) == 1)
          {
            if(!pstracker->startNewRigidBody(sensor))
              {
                fprintf(stderr,"Error, unable to add new rigid body: %d\n",sensor);
                continue;
              }
          }
        else
          {
            fprintf(stderr,"Ignoring line: %s\n",line);
            continue;
          }
      }
  }

  if(!pstracker->enableTracker(true))
    {
      fprintf(stderr,"Error, unable to enable OWL Tracker.\n");
      return -1;
    }

#else
  fprintf(stderr, "vrpn_server: Can't open PhaseSpace OWL server: VRPN_INCLUDE_PHASESPACE not defined in vrpn_Configure.h!\n");
  return -1;
#endif

  return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_NDI_Polaris (char * & pch, char * line, FILE * config_file) {
	char trackerName[LINESIZE];
	char device[LINESIZE];
	int numRigidBodies;
	char* rigidBodyFileNames[VRPN_GSO_MAX_NDI_POLARIS_RIGIDBODIES];
	
	
	//get tracker name and device
	if( sscanf(line,"vrpn_Tracker_NDI_Polaris %s %s %d",trackerName,device,&numRigidBodies) < 3)
    {
		fprintf(stderr,"Bad vrpn_Tracker_NDI_Polaris line: %s\n", line);
		return -1;
    }
	printf("DEBUG Tracker_NDI_Polaris: num of rigidbodies %d\n",numRigidBodies);
	
	//parse the filename for each rigid body
	int rbNum;
	for (rbNum=0; rbNum<numRigidBodies; rbNum++ ) {
		fgets(line, LINESIZE, config_file); //advance to next line of config file
		rigidBodyFileNames[rbNum]= new char[LINESIZE]; //allocate string for filename
		if (sscanf(line,"%s", rigidBodyFileNames[rbNum])!=1) {
			fprintf(stderr,"Tracker_NDI_Polaris: error reading .rom filename #%d from config file in line: %s\n",rbNum,line);
			return -1;
		} else { 
			printf("DEBUG Tracker_NDI_Polaris: filename >%s<\n",rigidBodyFileNames[rbNum]);
		}
		
	}
	
	// Make sure there's room for a new tracker
	if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
		fprintf(stderr,"Too many trackers in config file");
		return -1;
	}
	
	vrpn_Tracker_NDI_Polaris* nditracker =  new vrpn_Tracker_NDI_Polaris(trackerName, connection, device,numRigidBodies,(const char **) rigidBodyFileNames);
	if (nditracker==NULL) {
		fprintf(stderr,"Tracker_NDI_Polaris: error initializing tracker object\n");
		return -1;
	}
	trackers[num_trackers]=nditracker;
	num_trackers++;
	
	//free the .rom filename strings
	for (rbNum=0; rbNum<numRigidBodies; rbNum++ ) {	
		delete(rigidBodyFileNames[rbNum]);
	}
	return(0); //success
}

int vrpn_Generic_Server_Object::setup_Logger(char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];

  // Line will be: vrpn_Auxiliary_Logger_Server_Generic NAME CONNECTION_TO_LOG
  next();
  if (sscanf(pch,"%511s %511s",s2,s3)!=2) {
          fprintf(stderr,"Bad vrpn_Auxiliary_Logger_Server_Generic line: %s\n",line);
          return -1;
  }

  // Make sure we don't have a full complement already.
  if (num_loggers >= VRPN_GSO_MAX_LOGGER) {
    fprintf(stderr,"Too many vrpn_Auxiliary_Logger_Server_Generic loggers.\n");
    return -1;
  }

  // Open the logger
  if (verbose) printf("Opening vrpn_Auxiliary_Logger_Server_Generic %s\n", s2);
  vrpn_Auxiliary_Logger_Server_Generic* logger =  new vrpn_Auxiliary_Logger_Server_Generic(s2, s3, connection);

  if(logger == NULL) {
    fprintf(stderr,"Unable to create new vrpn_Auxiliary_Logger_Server_Generic.\n");
    return -1;
  }

  loggers[num_loggers] = logger;
  num_loggers++;

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_ImageStream(char * & pch, char * line, FILE * config_file) {

  char s2 [LINESIZE], s3 [LINESIZE];

  // Line will be: vrpn_Imager_Stream_Buffer NAME IMAGER_SERVER_TO_LOG
  next();
  if (sscanf(pch,"%511s %511s",s2,s3)!=2) {
          fprintf(stderr,"Bad vrpn_Imager_Stream_Buffer line: %s\n",line);
          return -1;
  }

  // Make sure we don't have a full complement already.
  if (num_imagestreams >= VRPN_GSO_MAX_IMAGE_STREAM) {
    fprintf(stderr,"Too many vrpn_Imager_Stream_Buffer loggers.\n");
    return -1;
  }

  // Open the stream buffer
  if (verbose) printf("Opening vrpn_Imager_Stream_Buffer %s\n", s2);
  vrpn_Imager_Stream_Buffer* imagestream =  new vrpn_Imager_Stream_Buffer(s2, s3, connection);

  if(imagestream == NULL) {
    fprintf(stderr,"Unable to create new vrpn_Imager_Stream_Buffer.\n");
    return -1;
  }

  imagestreams[num_imagestreams] = imagestream;
  num_imagestreams++;

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_WiiMote(char * & pch, char * line, FILE * config_file) {
#ifdef	VRPN_USE_WIIUSE
  char s2 [LINESIZE];
  unsigned controller;

  next();
  // Get the arguments (wiimote_name, controller index)
  if (sscanf(pch,"%511s%u",s2,&controller) != 2) {
    fprintf(stderr,"Bad vrpn_WiiMote line: %s\n",line);
    return -1;
  }

  // Make sure there's room for a new WiiMote
  if (num_XInputPads >= VRPN_GSO_MAX_WIIMOTES) {
    fprintf(stderr,"Too many WiiMotes in config file");
    return -1;
  }

  // Open the WiiMote
  if (verbose) {
    printf("Opening vrpn_WiiMote: %s\n", s2);
  }
  if ((wiimotes[num_wiimotes] = new vrpn_WiiMote(s2, connection, controller)) == NULL)
  {
    fprintf(stderr,"Can't create new vrpn_WiiMote\n");
    return -1;
  } else {
    num_wiimotes++;
  }

  return 0;
#else
  fprintf(stderr, "vrpn_server: Can't open WiiMote: VRPN_USE_WIIUSE not defined in vrpn_Configure.h!\n");
  return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Xkeys_Desktop(char * & pch, char * line, FILE * config_file) {
#ifdef _WIN32
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad Xkeys_Desktop line: %s\n",line);
    return -1;
  }

  // Open the Xkeys
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_Xkeys_Desktop: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_Xkeys_Desktop on host %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_Xkeys_Desktop(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_Xkeys_Desktop\n");
     return -1;
  } else {
    num_buttons++;
  }
#else
  fprintf(stderr,"vrpn_server: Can't open Xkeys: HID not compiled in.\n");
#endif

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_Xkeys_Pro(char * & pch, char * line, FILE * config_file) {
#ifdef _WIN32
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad Xkeys_Pro line: %s\n",line);
    return -1;
  }

  // Open the Xkeys
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_Xkeys_Pro: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_Xkeys_Pro on host %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_Xkeys_Pro(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_Xkeys_Pro\n");
     return -1;
  } else {
    num_buttons++;
  }
#else
  fprintf(stderr,"vrpn_server: Can't open Xkeys: HID not compiled in.\n");
#endif

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_Xkeys_Joystick(char * & pch, char * line, FILE * config_file) {
#ifdef _WIN32
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad Xkeys_Joystick line: %s\n",line);
    return -1;
  }

  // Open the Xkeys
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_Xkeys_Joystick: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_Xkeys_Joystick on host %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_Xkeys_Joystick(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_Xkeys_Joystick\n");
     return -1;
  } else {
    num_buttons++;
  }
#else
  fprintf(stderr,"vrpn_server: Can't open Xkeys: HID not compiled in.\n");
#endif

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_Xkeys_Jog_And_Shuttle(char * & pch, char * line, FILE * config_file) {
#ifdef _WIN32
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad Xkeys_Jog_And_Shuttle line: %s\n",line);
    return -1;
  }

  // Open the Xkeys
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_Xkeys_Jog_And_Shuttle: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_Xkeys_Jog_And_Shuttle on host %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_Xkeys_Jog_And_Shuttle(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_Xkeys_Jog_And_Shuttle\n");
     return -1;
  } else {
    num_buttons++;
  }
#else
  fprintf(stderr,"vrpn_server: Can't open Xkeys: HID not compiled in.\n");
#endif

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_3DConnexion_Navigator(char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad 3DConnexion_Navigator line: %s\n",line);
    return -1;
  }

  // Open the 3DConnexion_Navigator
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_3DConnexion_Navigator: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_3DConnexion_Navigator %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_3DConnexion_Navigator(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_3DConnexion_Navigator\n");
     return -1;
  } else {
    num_buttons++;
  }

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_3DConnexion_Traveler(char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad 3DConnexion_Traveler line: %s\n",line);
    return -1;
  }

  // Open the 3DConnexion_Traveler
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_3DConnexion_Traveler: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_3DConnexion_Traveler %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_3DConnexion_Traveler(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_3DConnexion_Traveler\n");
     return -1;
  } else {
    num_buttons++;
  }

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_3DConnexion_SpaceMouse(char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad 3DConnexion_Traveler line: %s\n",line);
    return -1;
  }

  // Open the 3DConnexion_Traveler
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_3DConnexion_Traveler: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_3DConnexion_SpaceMouse %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_3DConnexion_SpaceMouse(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_3DConnexion_SpaceMouse\n");
     return -1;
  } else {
    num_buttons++;
  }

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_3DConnexion_SpaceExplorer(char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad 3DConnexion_SpaceExplorer line: %s\n",line);
    return -1;
  }

  // Open the 3DConnexion_Traveler
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_3DConnexion_SpaceExplorer: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_3DConnexion_SpaceExplorer %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_3DConnexion_SpaceExplorer(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_3DConnexion_SpaceExplorer\n");
     return -1;
  } else {
    num_buttons++;
  }

  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_3DConnexion_SpaceBall5000(char * & pch, char * line, FILE * config_file) {
  char s2 [LINESIZE];

  next();
  if (sscanf(pch,"%511s",s2)!=1) {
    fprintf(stderr,"Bad 3DConnexion_SpaceBall5000 line: %s\n",line);
    return -1;
  }

  // Open the 3DConnexion_SpaceBall5000
  // Make sure there's room for a new button
  if (num_buttons >= VRPN_GSO_MAX_BUTTONS) {
    fprintf(stderr,"vrpn_3DConnexion_SpaceBall5000: Too many buttons in config file");
    return -1;
  }

  // Open the button
  if (verbose) {
    printf("Opening vrpn_3DConnexion_SpaceBall5000 %s\n", s2);
  }
  if ( (buttons[num_buttons] = new vrpn_3DConnexion_SpaceBall5000(s2, connection)) == NULL ) {
    fprintf(stderr,"Can't create new vrpn_3DConnexion_SpaceBall5000\n");
     return -1;
  } else {
    num_buttons++;
  }

  return 0;  // successful completion
}


int vrpn_Generic_Server_Object::setup_Tracker_MotionNode(char * & pch, char * line, FILE * config_file)
{
  char name[LINESIZE];
  unsigned num_sensors = 0;
  char address[LINESIZE];
  unsigned port = 0;

  next();
  // Get the arguments (class, tracker_name, sensors, rate)
  if (4 != sscanf(pch,"%511s%u%511s%u", name, &num_sensors, address, &port)) {
    fprintf(stderr, "Bad vrpn_Tracker_MotionNode line: %s\n", line);
    return -1;
  }

  // Make sure there's room for a new tracker
  if (num_trackers >= VRPN_GSO_MAX_TRACKERS) {
    fprintf(stderr,"Too many trackers in config file");
    return -1;
  }

#ifdef  VRPN_USE_MOTIONNODE
  // Open the tracker
  if (verbose) {
    printf("Opening vrpn_Tracker_MotionNode: %s with %u sensors, address %s, port %u\n",
      name, num_sensors, address, port);
  }

  trackers[num_trackers] = new vrpn_Tracker_MotionNode(name, connection, num_sensors, address, port);

  if (NULL == trackers[num_trackers]) {
    fprintf(stderr, "Failed to create new vrpn_Tracker_MotionNode\n");
    return -1;
  } else {
    num_trackers++;
  }
#else
  fprintf(stderr,"vrpn_Tracker_MotionNode: Not compiled in (add VRPN_USE_MOTIONNODE to vrpn_Configure.h and recompile)\n");
#endif
  return 0;
}


vrpn_Generic_Server_Object::vrpn_Generic_Server_Object(vrpn_Connection *connection_to_use, const char *config_file_name, int port, bool be_verbose, bool bail_on_open_error) :
  connection(connection_to_use),
  d_doing_okay(true),
  verbose(be_verbose),
  d_bail_on_open_error(bail_on_open_error),
  num_trackers(0),
  num_buttons(0),
  num_sounds(0),
  num_analogs(0),
  num_sgiboxes(0),
  num_cereals(0),
  num_magellans(0),
  num_spaceballs(0),
  num_iboxes(0),
  num_dials(0),
  num_generators(0),
  num_tng3s(0),
  num_DirectXJoys(0),
  num_RumblePads(0),
  num_XInputPads(0),
  num_Win32Joys(0),
  num_GlobalHapticsOrbs(0),
  num_phantoms(0),
  num_analogouts(0),
  num_DTracks(0),
  num_posers(0),
  num_mouses(0)
  , num_inertiamouses (0)
  , num_Keyboards(0)
  , num_loggers(0)
  , num_imagestreams(0)
#ifdef	VRPN_USE_WIIUSE
  , num_wiimotes(0)
#endif
  {
    FILE    * config_file;
    char    * client_name = NULL;

    // Open the configuration file
    if (verbose) printf("Reading from config file %s\n", config_file_name);
    if ( (config_file = fopen(config_file_name, "r")) == NULL) {
        perror("vrpn_Generic_Server_Object::vrpn_Generic_Server_Object(): Cannot open config file");
        fprintf(stderr,"  (filename %s)\n", config_file_name);
        d_doing_okay = false;
            return;
    }

    // Read the configuration file, creating a device for each entry.
    // Each entry is on one line, which starts with the name of the
    //   class of the object that is to be created.
    // If we fail to open a certain device, print a message and decide
    //  whether we should bail.
    {
        char    line[LINESIZE]; // Line read from the input file
        char *pch;
        char    scrap[LINESIZE];
        char    s1[LINESIZE];
        int retval;

        // Read lines from the file until we run out
        while ( fgets(line, LINESIZE, config_file) != NULL ) {

          // Make sure the line wasn't too long
          if (strlen(line) >= LINESIZE-1) {
              fprintf(stderr,"vrpn_Generic_Server_Object::vrpn_Generic_Server_Object(): Line too long in config file: %s\n",line);
              if (d_bail_on_open_error) { d_doing_okay = false; return; }
              else { continue; }  // Skip this line
          }

          // Ignore comments and empty lines.  Skip white space before comment mark (#).
          if (strlen(line)<3) {
              continue;
          }
          bool ignore = false;
          for (int j=0; line[j] != '\0'; j++) {
              if (line[j]==' ' || line[j]=='\t') {
                  continue;
              }
              if (line[j]=='#') {
                  ignore = true;
              }
              break;
          }
          if (ignore) {
              continue;
          }

          // copy for strtok work
          strncpy(scrap, line, LINESIZE);
          // Figure out the device from the name and handle appropriately

          // WARNING: SUBSTRINGS WILL MATCH THE EARLIER STRING, SO
          // ADD AN EMPTY SPACE TO THE END OF STATIC STRINGS!!!!

          //    #define isit(s) !strncmp(line,s,strlen(s))
          #define isit(s) !strcmp(pch=strtok(scrap," \t"),s)

          // Rewritten to move all this code out-of-line by Tom Hudson
          // August 99.  We could even make it table-driven now.
          // It seems that we're never going to document this program
          // and that it will always be necessary to read the code to
          // figure out how to write a config file.  This code rearrangement
          // should make it easier to figure out what the possible tokens
          // are in a config file by listing them close together here
          // instead of hiding them in the middle of functions.

          if (isit("vrpn_raw_SGIBox")) {
              CHECK(setup_raw_SGIBox);
          } else if (isit("vrpn_SGIBOX")) {
              CHECK(setup_SGIBox);
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
          } else  if (isit("vrpn_Joywin32")) {
              CHECK(setup_Joywin32);
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
          } else if (isit("vrpn_5dt16")) {
              CHECK(setup_5dt16);
          } else if (isit("vrpn_Button_5DT_Server")) {
              CHECK(setup_Button_5DT_Server);
          } else if (isit("vrpn_ImmersionBox")) {
              CHECK(setup_ImmersionBox);
          } else if (isit("vrpn_Tracker_Dyna")) {
              CHECK(setup_Tracker_Dyna);
          } else if (isit("vrpn_Tracker_Fastrak")) {
              CHECK(setup_Tracker_Fastrak);
		  } else if (isit("vrpn_Tracker_NDI_Polaris")) {
              CHECK(setup_Tracker_NDI_Polaris);
          } else if (isit("vrpn_Tracker_Liberty")) {
              CHECK(setup_Tracker_Liberty);
          } else if (isit("vrpn_Tracker_3Space")) {
              CHECK(setup_Tracker_3Space);
          } else if (isit("vrpn_Tracker_Flock")) {
              CHECK(setup_Tracker_Flock);
          } else if (isit("vrpn_Tracker_Flock_Parallel")) {
              CHECK(setup_Tracker_Flock_Parallel);
          } else if (isit("vrpn_Tracker_3DMouse")) {
              CHECK(setup_Tracker_3DMouse);
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
          } else if (isit("vrpn_Mouse")) {
              CHECK(setup_Mouse);
          } else if (isit("vrpn_Tng3")) {
              CHECK(setup_Tng3);
          } else  if (isit("vrpn_TimeCode_Generator")) {
              CHECK(setup_Timecode_Generator);
          } else if (isit("vrpn_Tracker_InterSense")) {
              CHECK(setup_Tracker_InterSense);
          } else if (isit("vrpn_DirectXFFJoystick")) {
              CHECK(setup_DirectXFFJoystick);
          } else if (isit("vrpn_DirectXRumblePad")) {
              CHECK(setup_RumblePad);
		  } else if (isit("vrpn_XInputGamepad")) {
			  CHECK(setup_XInputPad);
          } else if (isit("vrpn_GlobalHapticsOrb")) {
              CHECK(setup_GlobalHapticsOrb);
          } else if (isit("vrpn_Phantom")) {
              CHECK(setup_Phantom);
          } else if (isit("vrpn_ADBox")) {
              CHECK(setup_ADBox);
          } else if (isit("vrpn_VPJoystick")) {
              CHECK(setup_VPJoystick);
          } else if (isit("vrpn_Tracker_DTrack")) {
              CHECK(setup_DTrack);
          } else if (isit("vrpn_NI_Analog_Output")) {
              CHECK(setup_NationalInstrumentsOutput);
          } else if (isit("vrpn_National_Instruments")) {
              CHECK(setup_NationalInstruments);
          } else if (isit("vrpn_nikon_controls")) {
              CHECK(setup_nikon_controls);
          } else if (isit("vrpn_Tek4662")) {
              CHECK(setup_Poser_Tek4662);
          } else if (isit("vrpn_Poser_Analog")) {
              CHECK(setup_Poser_Analog);
          } else if (isit("vrpn_Tracker_Crossbow")) {
              CHECK(setup_Tracker_Crossbow);
          } else if (isit("vrpn_3DMicroscribe")) {
              CHECK(setup_3DMicroscribe);
	  } else if (isit("vrpn_Keyboard")) {
            CHECK(setup_Keyboard);
	  } else if (isit("vrpn_Button_USB")) {
            CHECK(setup_Button_USB);
	  } else if (isit("vrpn_Analog_USDigital_A2")) {
            CHECK(setup_Analog_USDigital_A2);
	  } else if (isit("vrpn_Button_NI_DIO24")) {
            CHECK(setup_Button_NI_DIO24);
	  } else if (isit("vrpn_Tracker_PhaseSpace")) {
            CHECK(setup_Tracker_PhaseSpace);
	  } else if (isit("vrpn_Auxiliary_Logger_Server_Generic")) {
            CHECK(setup_Logger);
	  } else if (isit("vrpn_Imager_Stream_Buffer")) {
            CHECK(setup_ImageStream);
	  } else if (isit("vrpn_Xkeys_Desktop")) {
            CHECK(setup_Xkeys_Desktop);
	  } else if (isit("vrpn_Xkeys_Pro")) {
            CHECK(setup_Xkeys_Pro);
	  } else if (isit("vrpn_Xkeys_Joystick")) {
            CHECK(setup_Xkeys_Joystick);
	  } else if (isit("vrpn_Xkeys_Jog_And_Shuttle")) {
            CHECK(setup_Xkeys_Jog_And_Shuttle);
	  } else if (isit("vrpn_3DConnexion_Navigator")) {
            CHECK(setup_3DConnexion_Navigator);
	  } else if (isit("vrpn_3DConnexion_Traveler")) {
            CHECK(setup_3DConnexion_Traveler);
	  } else if (isit("vrpn_3DConnexion_SpaceExplorer")) {
            CHECK(setup_3DConnexion_SpaceExplorer);
	  } else if (isit("vrpn_3DConnexion_SpaceMouse")) {
            CHECK(setup_3DConnexion_SpaceMouse);
	  } else if (isit("vrpn_3DConnexion_SpaceBall5000")) {
            CHECK(setup_3DConnexion_SpaceBall5000);
          } else if (isit("vrpn_Tracker_MotionNode")) {
            CHECK(setup_Tracker_MotionNode);
	  } else if (isit("vrpn_WiiMote")) {
            CHECK(setup_WiiMote);
// BUW additions
          } else if (isit("vrpn_Atmel")) {
            CHECK(setup_Atmel);
          } else if (isit ("vrpn_inertiamouse")) {
            CHECK(setup_inertiamouse);
          } else if (isit("vrpn_Event_Mouse")) {
            CHECK(setup_Event_Mouse);
// end of BUW additions
	 } else {	// Never heard of it
		sscanf(line,"%511s",s1);	// Find out the class name
		fprintf(stderr,"vrpn_server: Unknown Device: %s\n",s1);
		if (d_bail_on_open_error) { d_doing_okay = false; return; }
		else { continue; }	// Skip this line
	  }
	}
      }

    // Close the configuration file
    fclose(config_file);

#ifdef  SGI_BDBOX
    fprintf(stderr, "sgibox: %p\n", vrpn_special_sgibox);
#endif
}

vrpn_Generic_Server_Object::~vrpn_Generic_Server_Object()
{
  closeDevices();
}

void  vrpn_Generic_Server_Object::mainloop( void )
{
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

  // Let all the analog outputs do their thing
  for (i=0; i< num_analogouts; i++) {
	  analogouts[i]->mainloop();
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
#ifdef VRPN_INCLUDE_TIMECODE_SERVER
  for (i=0; i < num_generators; i++) {
	  timecode_generators[i]->mainloop();
  }
#endif
#ifdef VRPN_USE_PHANTOM_SERVER
  // Let all the Phantoms do their thing
  for (i=0; i< num_phantoms; i++) {
      phantoms[i]->mainloop();
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

  // Let all the rumblepads do their thing
  for (i = 0; i < num_RumblePads; i++) {
    RumblePads[i]->mainloop();
  }

  // Let all the Xbox controller do their thing
  for (i = 0; i < num_XInputPads; i++) {
    XInputPads[i]->mainloop();
  }
#endif

#ifdef	_WIN32
  // Let all the win32 Joysticks do their thing
  for (i=0; i < num_Win32Joys; i++) {
	  win32joys[i]->mainloop();
  }
  // Let all the win32 mouse keyboards do their thing
  for (i=0; i < num_Keyboards; i++)
	  Keyboards[i]->mainloop();
#endif

  // Let all the DTracks do their thing
  for (i = 0; i < num_DTracks; i++) {
    DTracks[i]->mainloop();
  }

  // Let all the Orbs do their thing
  for (i = 0; i < num_GlobalHapticsOrbs; i++) {
    ghos[i]->mainloop();
  }

  // Let all the Posers do their thing
  for (i=0; i< num_posers; i++) {
	  posers[i]->mainloop();
  }

  // Let all the Mouses (Mice) do their thing
  for (i=0; i< num_mouses; i++) {
	  mouses[i]->mainloop();
  }

  // Let all the Loggers do their thing
  for (i=0; i< num_loggers; i++) {
	  loggers[i]->mainloop();
  }

  // Let all the ImageStreams do their thing
  for (i=0; i< num_imagestreams; i++) {
	  imagestreams[i]->mainloop();
  }

  // Let all the WiiMotes do their thing
#ifdef	VRPN_USE_WIIUSE
  for (i=0; i< num_wiimotes; i++) {
	  wiimotes[i]->mainloop();
  }
#endif
}

