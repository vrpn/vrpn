#include <stdlib.h>                 // for strtol, atoi, strtod
#include <string.h>                 // for strcmp, strlen, strtok, etc
#include "vrpn_MainloopContainer.h" // for vrpn_MainloopContainer
#include <locale>                   // To enable setting parsing for .cfg file
#include <string>

#include "timecode_generator_server/vrpn_timecode_generator.h"
#include "vrpn_3DConnexion.h" // for vrpn_3DConnexion_Navigator, etc
#include "vrpn_3DMicroscribe.h"
#include "vrpn_3Space.h"     // for vrpn_Tracker_3Space
#include "vrpn_5DT16.h"      // for vrpn_5dt16, etc
#include "vrpn_Adafruit.h"   // for vrpn_Adafruit_10DOF
#include "vrpn_ADBox.h"      // for vrpn_ADBox
#include "vrpn_Analog_5dt.h" // for vrpn_5dt
#include "vrpn_Analog_5dtUSB.h"
#include "vrpn_Analog.h"             // for vrpn_Analog, etc
#include "vrpn_Analog_Output.h"      // for vrpn_Analog_Output
#include "vrpn_Analog_Radamec_SPI.h" // for vrpn_Radamec_SPI
#include "vrpn_Analog_USDigital_A2.h"
#include "vrpn_Atmel.h" // for VRPN_ATMEL_MODE_NA, etc
#include "vrpn_Auxiliary_Logger.h"
#include "vrpn_BiosciencesTools.h" // for vrpn_BiosciencesTools
#include "vrpn_Button.h"           // for vrpn_Button, etc
#include "vrpn_Button_NI_DIO24.h"
#include "vrpn_Button_USB.h"
#include "vrpn_CerealBox.h"                 // for vrpn_CerealBox
#include "vrpn_CHProducts_Controller_Raw.h" // for vrpn_CHProducts_Fighterstick_USB
#include "vrpn_Connection.h"
#include "vrpn_Contour.h"  // for vrpn_Contour_ShuttleXpress, etc.
#include "vrpn_DevInput.h" // for vrpn_DevInput
#include "vrpn_Dial.h"     // for vrpn_Dial, etc
#include "vrpn_DirectXFFJoystick.h"
#include "vrpn_DirectXRumblePad.h"
#include "vrpn_DreamCheeky.h"    // for vrpn_DreamCheeky_Drum_Kit
#include "vrpn_Dyna.h"           // for vrpn_Tracker_Dyna, etc
#include "vrpn_Event_Mouse.h"    // for vrpn_Event_Mouse
#include "vrpn_Flock.h"          // for vrpn_Tracker_Flock, etc
#include "vrpn_Flock_Parallel.h" // for vrpn_Tracker_Flock_Parallel
#include "vrpn_Freespace.h"
#include "vrpn_Futaba.h" // for vrpn_Futaba_InterLink_Elite, etc.
#include "vrpn_Generic_server_object.h"
#include "vrpn_GlobalHapticsOrb.h"     // for vrpn_GlobalHapticsOrb
#include "vrpn_Griffin.h"              // for vrpn_Griffin_PowerMate, etc.
#include "vrpn_IDEA.h"                 // for vrpn_IDEA
#include "vrpn_Imager_Stream_Buffer.h" // for vrpn_Imager_Stream_Buffer
#include "vrpn_ImmersionBox.h"         // for vrpn_ImmersionBox
#include "vrpn_inertiamouse.h"         // for vrpn_inertiamouse
#include "vrpn_JoyFly.h"               // for vrpn_Tracker_JoyFly
#include "vrpn_Joylin.h"               // for vrpn_Joylin
#include "vrpn_Joywin32.h"
#include "vrpn_Keyboard.h"                // for vrpn_Keyboard
#include "vrpn_Laputa.h"                  // for vrpn_Laputa
#include "vrpn_LUDL.h"                    // for vrpn_LUDL_USBMAC6000
#include "vrpn_Logitech_Controller_Raw.h" // for vrpn_Logitech_Extreme_3D_Pro, etc.
#include "vrpn_Magellan.h"                // for vrpn_Magellan
#include "vrpn_Microsoft_Controller_Raw.h" // for vrpn_Microsoft_Controller_Raw_Xbox_S, vrpn_Microsoft_Controller_Raw_Xbox_360, etc.
#include "vrpn_Mouse.h"                    // for vrpn_Button_SerialMouse, etc
#include "vrpn_NationalInstruments.h"
#include "vrpn_nikon_controls.h"   // for vrpn_Nikon_Controls
#include "vrpn_nVidia_shield_controller.h"
#include "vrpn_Oculus.h"           // for vrpn_Oculus
#include "vrpn_OmegaTemperature.h" // for vrpn_OmegaTemperature
#include "vrpn_OzzMaker.h"   // for vrpn_OzzMaker_BerryIMU
#include "vrpn_Phantom.h"
#include "vrpn_Poser_Analog.h"          // for vrpn_Poser_AnalogParam, etc
#include "vrpn_Poser.h"                 // for vrpn_Poser
#include "vrpn_Poser_Tek4662.h"         // for vrpn_Poser_Tek4662
#include "vrpn_raw_sgibox.h"            // for vrpn_raw_SGIBox, for access to the SGI button & dial box connected to the serial port of an linux PC
#include "vrpn_Retrolink.h"             // for vrpn_Retrolink_GameCube, etc.
#include "vrpn_Saitek_Controller_Raw.h" // for vrpn_Saitek_ST290_Pro, etc.
#include "vrpn_sgibox.h" //for access to the B&D box connected to an SGI via the IRIX GL drivers
#include "vrpn_Sound.h"             // for vrpn_Sound
#include "vrpn_Spaceball.h"         // for vrpn_Spaceball
#include "vrpn_Streaming_Arduino.h" // for vrpn_Streaming_Arduino
#include "vrpn_Tng3.h"              // for vrpn_Tng3
#include "vrpn_Tracker_3DMouse.h"   // for vrpn_Tracker_3DMouse
#include "vrpn_Tracker_AnalogFly.h" // for vrpn_Tracker_AnalogFlyParam, etc
#include "vrpn_Tracker_ButtonFly.h" // for vrpn_TBF_axis, etc
#include "vrpn_Tracker_Crossbow.h"  // for vrpn_Tracker_Crossbow
#include "vrpn_Tracker_DTrack.h"    // for vrpn_Tracker_DTrack
#include "vrpn_Tracker_Fastrak.h"   // for vrpn_Tracker_Fastrak
#include "vrpn_Tracker_GameTrak.h"  // for vrpn_Tracker_GameTrak
#include "vrpn_Tracker_GPS.h"       // for vrpn_Tracker_GPS
#include "vrpn_Tracker.h"           // for vrpn_Tracker, etc
#include "vrpn_Tracker_IMU.h"       // for vrpn_IMU_Magnetometer, etc
#include "vrpn_Tracker_isense.h"
#include "vrpn_Tracker_Isotrak.h" // for vrpn_Tracker_Isotrak
#include "vrpn_Tracker_JsonNet.h"
#include "vrpn_Tracker_Liberty.h"   // for vrpn_Tracker_Liberty
#include "vrpn_Tracker_LibertyHS.h" // for vrpn_Tracker_LibertyHS
#include "vrpn_Tracker_MotionNode.h"
#include "vrpn_Tracker_NDI_Polaris.h" // for vrpn_Tracker_NDI_Polaris
#include "vrpn_Tracker_NovintFalcon.h"
#include "vrpn_Tracker_OSVRHackerDevKit.h" // for vrpn_Tracker_OSVRHackerDevKit
#include "vrpn_Tracker_PDI.h"
#include "vrpn_Tracker_PhaseSpace.h"
#include "vrpn_Tracker_RazerHydra.h"      // for vrpn_Tracker_RazerHydra
#include "vrpn_Tracker_Filter.h"          // for vrpn_Tracker_FilterOneEuro
#include "vrpn_Tracker_SpacePoint.h"      // for vrpn_Tracker_SpacePoint
#include "vrpn_Tracker_ThalmicLabsMyo.h"  // for vrpn_Tracker_ThalmicLabsMyo
#include "vrpn_Tracker_TrivisioColibri.h" // added by David Borland
#include "vrpn_Tracker_Colibri.h"         // added by Dmitry Mastykin
#include "vrpn_Tracker_ViewPoint.h"       // added by David Borland
#include "vrpn_Tracker_WiimoteHead.h"     // for vrpn_Tracker_WiimoteHead
#include "vrpn_Tracker_Wintracker.h"      // for vrpn_Tracker_Wintracker
#include "vrpn_Tracker_zSight.h"          // added by David Borland
#include "vrpn_UNC_Joystick.h"            // for vrpn_Joystick
#include "vrpn_VPJoystick.h"              // for vrpn_VPJoystick
#include "vrpn_Wanda.h"                   // for vrpn_Wanda
#include "vrpn_WiiMote.h"
#include "vrpn_XInputGamepad.h"
#include "vrpn_Xkeys.h"      // for vrpn_Xkeys_Desktop, etc
#include "vrpn_YEI_3Space.h" // for vrpn_YEI_3Space_Sensor, etc
#include "vrpn_Vality.h"
#include "vrpn_Zaber.h"      // for vrpn_Zaber

class VRPN_API vrpn_Connection;

/// File-static constant of max line size.
static const int LINESIZE = 512;

#define VRPN_CONFIG_NEXT() pch += strlen(pch) + 1

// BUW additions
/* some helper variables to configure the vrpn_Atmel server */
namespace setup_vrpn_Atmel {
    int channel_mode[vrpn_CHANNEL_MAX];
    int channel_count = 0;
} // namespace setup_vrpn_Atmel

#ifdef SGI_BDBOX
vrpn_SGIBox *vrpn_special_sgibox;
#endif

void vrpn_Generic_Server_Object::closeDevices(void)
{
    _devices->clear();

    if (verbose) {
        fprintf(stderr, "\nAll devices closed...\n");
    }
}

template <typename T>
inline int
vrpn_Generic_Server_Object::templated_setup_device_name_only(char *&pch,
                                                             char *line, FILE *)
{
    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s", s2) != 1) {
        fprintf(stderr, "Bad line: %s\n", line);
        return -1;
    }
    T *device = new T(s2, connection);
    if (device == NULL) {
        fprintf(stderr, "Can't create new device from line %s\n", line);
        return -1;
    }
    if (verbose) {
        printf("Opening %s\n", line);
    }
    _devices->add(device);

    return 0; // successful completion
}

template <typename T>
inline int vrpn_Generic_Server_Object::templated_setup_HID_device_name_only(
    char *&pch, char *line, FILE *)
{
    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s", s2) != 1) {
        fprintf(stderr, "Bad line: %s\n", line);
        return -1;
    }
#ifdef VRPN_USE_HID
    T *device = new T(s2, connection);
    if (device == NULL) {
        fprintf(stderr, "Can't create new device from line %s\n", line);
        return -1;
    }
    if (verbose) {
        printf("Opening %s\n", line);
    }
    _devices->add(device);

    return 0; // successful completion
#else
    fprintf(stderr, "HID support required for device requested by line %s:\n",
            line);
    return -1;
#endif
}

// setup_raw_SGIBox
// uses globals:  num_sgiboxes, sgiboxes[], verbose
// imports from main:  pch
// returns nonzero on error

int vrpn_Generic_Server_Object::setup_raw_SGIBox(char *&pch, char *line,
                                                 FILE * /*config_file*/)
{

    char s2[LINESIZE], s3[LINESIZE];

    // Line will be: vrpn_raw_SGIBox NAME PORT [list of buttons to toggle]
    int tbutton; // Button to toggle
    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s %511s", s2, s3) != 2) {
        fprintf(stderr, "Bad vrpn_raw_SGIBox line: %s\n", line);
        return -1;
    }

    // Open the raw SGI box
    if (verbose) {
        printf("Opening vrpn_raw_SGIBox %s on serial port %s\n", s2, s3);
    }
    vrpn_raw_SGIBox *device =
        _devices->add(new vrpn_raw_SGIBox(s2, connection, s3));

    // setting listed buttons to toggles instead of default momentary
    // pch=s3;
    pch += strlen(s2) + 1; // advance past the name and port
    pch += strlen(s3) + 1;
    while (sscanf(pch, "%511s", s2) == 1) {
        pch += strlen(s2) + 1;
        tbutton = atoi(s2);
        // set the button to be a toggle,
        // and set the state of that toggle
        // to 'off'
        device->set_toggle(tbutton, vrpn_BUTTON_TOGGLE_OFF);
        // vrpnButton class will make sure I don't set
        // an invalid button number
        printf("\tButton %d is toggle\n", tbutton);
    }

    return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_SGIBox(char *&pch, char *line,
                                             FILE * /*config_file*/)
{

    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s", s2) != 1) {
        fprintf(stderr, "Bad vrpn_SGIBox line: %s\n", line);
        return -1;
    }

    // Open the sgibox
    if (verbose) {
        printf("Opening vrpn_SGIBox on host %s\n", s2);
    }
#ifdef SGI_BDBOX
    vrpn_special_sgibox = _devices->add(new vrpn_SGIBox(s2, connection));

    int tbutton;
    // setting listed buttons to toggles instead of default momentary
    pch += strlen(s2) + 1;
    while (sscanf(pch, "%s", s2) == 1) {
        pch += strlen(s2) + 1;
        tbutton = atoi(s2);
        vrpn_special_sgibox->set_toggle(tbutton, vrpn_BUTTON_TOGGLE_OFF);
        // vrpnButton class will make sure I don't set
        // an invalid button number
        printf("Button %d toggles\n", tbutton);
    }
    printf("Opening vrpn_SGIBox on host %s done\n", s2);

    return 0; // successful completion
#else
    fprintf(stderr, "vrpn_server: Can't open SGIbox: not an SGI!  Try "
                    "raw_SGIbox instead.\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Timecode_Generator(char *&pch, char *line,
                                                         FILE * /*config_file*/)
{
    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s", s2) != 1) {
        fprintf(stderr, "Bad vrpn_Timecode_Generator line: %s\n", line);
        return -1;
    }

#ifdef VRPN_INCLUDE_TIMECODE_SERVER
    // open the timecode generator
    if (verbose) {
        printf("Opening vrpn_Timecode_Generator on host %s\n", s2);
    }
    _devices->add(new vrpn_Timecode_Generator(s2, connection));
    return 0; // successful completion
#else
    fprintf(stderr, "vrpn_server: Can't open Timecode Generator: "
                    "INCLUDE_TIMECODE_GENERATOR not defined in "
                    "vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Phantom(char *&pch, char *line,
                                              FILE * /*config_file*/)
{
    char s2[512]; // String parameters
    int i1;       // Integer parameters
    float f1;     // Float parameters
    // Jean SIMARD <jean.simard@limsi.fr>
    // Add the variable for the configuration name of the PHANToM interface
    char sconf[512];

    VRPN_CONFIG_NEXT();

    // Jean SIMARD <jean.simard@limsi.fr>
    // Modify the analyse of the configuration name of 'vrpn.cfg'
    // The new version use the advantages of 'strtok' function
    if (!(sscanf(strtok(pch, " \t"), "%511s", s2) &&
          sscanf(strtok(NULL, " \t"), "%d", &i1) &&
          sscanf(strtok(NULL, " \t"), "%f", &f1) &&
          sscanf(strtok(NULL, "\n"), "%511[^\n]", sconf))) {
        fprintf(stderr, "Bad vrpn_Phantom line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_PHANTOM_SERVER
    // Jean SIMARD <jean.simard@limsi.fr>
    // Put a more verbose version when a PHANToM connection is opened.
    if (verbose) {
        printf("Opening vrpn_Phantom:\n");
        printf("\tVRPN name: %s\n", s2);
        printf("\tConfiguration name: \"%s\"\n", sconf);
        printf("\tCalibration: %s\n", ((i1 == 0) ? "no" : "yes"));
        printf("\tFrequence: %.3f\n", f1);
    }

    // i1 is a boolean that tells whether to let the user establish the reset
    // position or not.
    if (i1) {
        printf("Initializing phantom, you have 10 seconds to establish reset "
               "position\n");
        vrpn_SleepMsecs(10000);
    }

    // Jean SIMARD <jean.simard@limsi.fr>
    // Modification of the call of the constructor
    _devices->add(new vrpn_Phantom(s2, connection, f1, sconf));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open Phantom server: "
                    "VRPN_USE_PHANTOM_SERVER not defined in "
                    "vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_JoyFly(char *&pch, char *line,
                                             FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE], s4[LINESIZE];

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%511s%511s", s2, s3, s4) != 3) {
        fprintf(stderr, "Bad vrpn_JoyFly line: %s\n", line);
        return -1;
    }

#ifdef _WIN32
    fprintf(stderr, "JoyFly tracker not yet defined for NT\n");
    return -1;
#else

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_JoyFly:  "
               "%s on server %s with config_file %s\n",
               s2, s3, s4);

    // HACK HACK HACK
    // Check for illegal character leading '*' to see if it's local
    if (s3[0] == '*') {
        _devices->add(
            new vrpn_Tracker_JoyFly(s2, connection, &s3[1], s4, connection));
    }
    else {
        _devices->add(new vrpn_Tracker_JoyFly(s2, connection, s3, s4));
    }

    return 0;
#endif
}

// This function will read one line of the vrpn_AnalogFly configuration
// (matching
// one axis) and fill in the data for that axis. The axis name, the file to read
// from, and the axis to fill in are passed as parameters. It returns 0 on
// success
// and -1 on failure.

int vrpn_Generic_Server_Object::get_AFline(char *line, vrpn_TAF_axis *axis)
{
    char axis_name[LINESIZE];
    char *name =
        new char[LINESIZE]; // We need this to stay around for the param
    int channel;
    float offset, thresh, power, scale;

    // Get the values from the line
    if (sscanf(line, "%511s%511s%d%g%g%g%g", axis_name, name, &channel,
               &offset, &thresh, &scale, &power) != 7) {
        fprintf(stderr, "AnalogFly Axis: Bad axis line\n");
        delete[] name;
        return -1;
    }

    if (strcmp(name, "NULL") == 0) {
        delete [] name;
        axis->name = NULL;
    }
    else {
        axis->name = name;
    }
    axis->channel = channel;
    axis->offset = offset;
    axis->thresh = thresh;
    axis->scale = scale;
    axis->power = power;

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_AnalogFly(char *&pch, char *line,
                                                        FILE *config_file)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;
    float f1;
    vrpn_Tracker_AnalogFlyParam p;
    bool absolute;
    bool worldFrame = VRPN_FALSE;

    VRPN_CONFIG_NEXT();

    if (sscanf(pch, "%511s%g%511s", s2, &f1, s3) != 3) {
        fprintf(stderr, "Bad vrpn_Tracker_AnalogFly line: %s\n", line);
        return -1;
    }

    // See if this should be absolute or differential
    if (strcmp(s3, "absolute") == 0) {
        absolute = vrpn_true;
    }
    else if (strcmp(s3, "differential") == 0) {
        absolute = vrpn_false;
    }
    else {
        fprintf(
            stderr,
            "vrpn_Tracker_AnalogFly: Expected 'absolute' or 'differential'\n");
        fprintf(stderr, "   but got '%s'\n", s3);
        return -1;
    }

    if (verbose) {
        printf("Opening vrpn_Tracker_AnalogFly: "
               "%s with update rate %g\n",
               s2, f1);
    }

    // Scan the following lines in the configuration file to fill
    // in the start-up parameters for the different axes.

    // parse lines until an empty line is encountered
    while (1) {
        char line[LINESIZE];

        // Read in the line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            perror("AnalogFly Can't read line!");
            return -1;
        }

        // if it is an empty line, finish parsing
        if (line[0] == '\n') {
            break;
        }

        // get the first token
        char tok[LINESIZE];
        sscanf(line, "%511s", tok);

        if (strcmp(tok, "X") == 0) {
            if (get_AFline(line, &p.x)) {
                fprintf(stderr, "Can't read X line for AnalogFly\n");
                return -1;
            }
        }
        else if (strcmp(tok, "Y") == 0) {
            if (get_AFline(line, &p.y)) {
                fprintf(stderr, "Can't read Y line for AnalogFly\n");
                return -1;
            }
        }
        else if (strcmp(tok, "Z") == 0) {
            if (get_AFline(line, &p.z)) {
                fprintf(stderr, "Can't read Z line for AnalogFly\n");
                return -1;
            }
        }
        else if (strcmp(tok, "RX") == 0) {
            if (get_AFline(line, &p.sx)) {
                fprintf(stderr, "Can't read RX line for AnalogFly\n");
                return -1;
            }
        }
        else if (strcmp(tok, "RY") == 0) {
            if (get_AFline(line, &p.sy)) {
                fprintf(stderr, "Can't read RY line for AnalogFly\n");
                return -1;
            }
        }
        else if (strcmp(tok, "RZ") == 0) {
            if (get_AFline(line, &p.sz)) {
                fprintf(stderr, "Can't read RZ line for AnalogFly\n");
                return -1;
            }
        }
        else if (strcmp(tok, "RESET") == 0) {
            // Read the reset line
            if (sscanf(line, "RESET %511s%d", s3, &i1) != 2) {
                fprintf(stderr, "Bad RESET line in AnalogFly: %s\n", line);
                return -1;
            }

            if (strcmp(s3, "NULL") != 0) {
                p.reset_name = strdup(s3);
                p.reset_which = i1;
            }
        }
        else if (strcmp(tok, "CLUTCH") == 0) {
            if (sscanf(line, "CLUTCH %511s%d", s3, &i1) != 2) {
                fprintf(stderr, "Bad CLUTCH line in AnalogFly: %s\n", line);
                return -1;
            }

            if (strcmp(s3, "NULL") != 0) {
                p.clutch_name = strdup(s3);
                p.clutch_which = i1;
            }
        }
        else if (strcmp(tok, "WORLDFRAME") == 0) {
            if (verbose) {
                printf("Enabling world-frame mode\n");
            }
            worldFrame = VRPN_TRUE;
        }
    }

    _devices->add(new vrpn_Tracker_AnalogFly(s2, connection, &p, f1, absolute,
                                             false, worldFrame));
    if (p.x.name != NULL) { delete [] p.x.name; }
    if (p.y.name != NULL) { delete [] p.y.name; }
    if (p.z.name != NULL) { delete [] p.z.name; }
    if (p.sx.name != NULL) { delete [] p.sx.name; }
    if (p.sy.name != NULL) { delete [] p.sy.name; }
    if (p.sz.name != NULL) { delete [] p.sz.name; }

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_ButtonFly(char *&pch, char *line,
                                                        FILE *config_file)
{
    char s2[LINESIZE], s3[LINESIZE];
    float f1;
    vrpn_Tracker_ButtonFlyParam p;

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%g", s2, &f1) != 2) {
        fprintf(stderr, "Bad vrpn_Tracker_ButtonFly line: %s\n", line);
        return -1;
    }

    if (verbose) {
        printf("Opening vrpn_Tracker_ButtonFly "
               "%s with update rate %g\n",
               s2, f1);
    }

    // Scan the following lines in the configuration file to fill
    // in the start-up parameters for the different axes and the
    // analog velocity and rotational velocity setters.  The last
    // line is "end".

    while (1) {
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr, "Ran past end of config file in ButtonFly\n");
            return -1;
        }
        if (sscanf(line, "%511s", s3) != 1) {
            fprintf(stderr, "Bad line in config file in ButtonFly\n");
            return -1;
        }
        if (strcmp(s3, "end") == 0) {
            break; // Done with reading the parameters
        }
        else if (strcmp(s3, "absolute") == 0) {
            char name[200]; //< Name of the button device to read from
            int which;      //< Which button to read from
            float x, y, z, rx, ry, rz; //< Position and orientation
            vrpn_TBF_axis axis;        //< Axis to add to the ButtonFly

            if (sscanf(line, "absolute %199s%d%g%g%g%g%g%g", name, &which, &x,
                       &y, &z, &rx, &ry, &rz) != 8) {
                fprintf(stderr, "ButtonFly: Bad absolute line\n");
                return -1;
            }

            float vec[3], rot[3];
            vec[0] = x;
            vec[1] = y;
            vec[2] = z;
            rot[0] = rx;
            rot[1] = ry;
            rot[2] = rz;
            axis.absolute = true;
            memcpy(axis.name, name, sizeof(axis.name));
            axis.channel = which;
            memcpy(axis.vec, vec, sizeof(axis.vec));
            memcpy(axis.rot, rot, sizeof(axis.rot));

            if (!p.add_axis(axis)) {
                fprintf(
                    stderr,
                    "ButtonFly: Could not add absolute axis to parameters\n");
                return -1;
            }
        }
        else if (strcmp(s3, "differential") == 0) {
            char name[200]; //< Name of the button device to read from
            int which;      //< Which button to read from
            float x, y, z, rx, ry, rz; //< Position and orientation
            vrpn_TBF_axis axis;        //< Axis to add to the ButtonFly

            if (sscanf(line, "differential %199s%d%g%g%g%g%g%g", name, &which,
                       &x, &y, &z, &rx, &ry, &rz) != 8) {
                fprintf(stderr, "ButtonFly: Bad differential line\n");
                return -1;
            }

            float vec[3], rot[3];
            vec[0] = x;
            vec[1] = y;
            vec[2] = z;
            rot[0] = rx;
            rot[1] = ry;
            rot[2] = rz;
            axis.absolute = false;
            memcpy(axis.name, name, sizeof(axis.name));
            axis.channel = which;
            memcpy(axis.vec, vec, sizeof(axis.vec));
            memcpy(axis.rot, rot, sizeof(axis.rot));

            if (!p.add_axis(axis)) {
                fprintf(stderr, "ButtonFly: Could not add differential axis to "
                                "parameters\n");
                return -1;
            }
        }
        else if (strcmp(s3, "vel_scale") == 0) {
            char name[200]; //< Name of the analog device to read from
            int channel;    //< Which analog channel to read from
            float offset, scale, power; //< Parameters to apply

            if (sscanf(line, "vel_scale %199s%d%g%g%g", name, &channel, &offset,
                       &scale, &power) != 5) {
                fprintf(stderr, "ButtonFly: Bad vel_scale line\n");
                return -1;
            }

            memcpy(p.vel_scale_name, name, sizeof(p.vel_scale_name));
            p.vel_scale_channel = channel;
            p.vel_scale_offset = offset;
            p.vel_scale_scale = scale;
            p.vel_scale_power = power;
        }
        else if (strcmp(s3, "rot_scale") == 0) {
            char name[200]; //< Name of the analog device to read from
            int channel;    //< Which analog channel to read from
            float offset, scale, power; //< Parameters to apply

            if (sscanf(line, "rot_scale %199s%d%g%g%g", name, &channel, &offset,
                       &scale, &power) != 5) {
                fprintf(stderr, "ButtonFly: Bad rot_scale line\n");
                return -1;
            }

            memcpy(p.rot_scale_name, name, sizeof(p.rot_scale_name));
            p.rot_scale_channel = channel;
            p.rot_scale_offset = offset;
            p.rot_scale_scale = scale;
            p.rot_scale_power = power;
        }
    }

    _devices->add(new vrpn_Tracker_ButtonFly(s2, connection, &p, f1));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Joystick(char *&pch, char *line,
                                               FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;

    float fhz;
    // Get the arguments
    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%511s%d %f", s2, s3, &i1, &fhz) != 4) {
        fprintf(stderr, "Bad vrpn_Joystick line: %s\n", line);
        return -1;
    }

    // Open the joystick server
    if (verbose)
        printf("Opening vrpn_Joystick:  "
               "%s on port %s baud %d, min update rate = %.2f\n",
               s2, s3, i1, fhz);
    _devices->add(new vrpn_Joystick(s2, connection, s3, i1, fhz));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Example_Button(char *&pch, char *line,
                                                     FILE * /*config_file*/)
{
    char s2[LINESIZE];
    int i1;
    float f1;

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, device_name, number_of_buttone, toggle_rate)
    if (sscanf(pch, "%511s%d%g", s2, &i1, &f1) != 3) {
        fprintf(stderr, "Bad vrpn_Button_Example line: %s\n", line);
        return -1;
    }

    // Open the button
    if (verbose)
        printf(
            "Opening vrpn_Button_Example: %s with %d sensors, toggle rate %f\n",
            s2, i1, f1);
    _devices->add(new vrpn_Button_Example_Server(s2, connection, i1, f1));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Example_Dial(char *&pch, char *line,
                                                   FILE * /*config_file*/)
{
    char s2[LINESIZE];
    int i1;
    float f1, f2;

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, dial_name, dials, spin_rate, update_rate)
    if (sscanf(pch, "%511s%d%g%g", s2, &i1, &f1, &f2) != 4) {
        fprintf(stderr, "Bad vrpn_Dial_Example line: %s\n", line);
        return -1;
    }

    // Open the dial
    if (verbose)
        printf("Opening vrpn_Dial_Example: %s with %d sensors, spinrate %f, "
               "update %f\n",
               s2, i1, f1, f2);
    _devices->add(new vrpn_Dial_Example_Server(s2, connection, i1, f1, f2));

    return 0;
}

int vrpn_Generic_Server_Object::setup_CerealBox(char *&pch, char *line,
                                                FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1, i2, i3, i4;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, serialbox_name, port, baud, numdig,
    // numana, numenc)
    if (sscanf(pch, "%511s%511s%d%d%d%d", s2, s3, &i1, &i2, &i3, &i4) != 6) {
        fprintf(stderr, "Bad vrpn_Cereal line: %s\n", line);
        return -1;
    }

    // Open the box
    if (verbose)
        printf("Opening vrpn_Cereal: %s on port %s, baud %d, %d dig, "
               " %d ana, %d enc\n",
               s2, s3, i1, i2, i3, i4);
    _devices->add(new vrpn_CerealBox(s2, connection, s3, i1, i2, i3, i4));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Magellan(char *&pch, char *line,
                                               FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE], s4[LINESIZE];
    int i1;
    int ret;
    bool altreset = false;

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, magellan_name, port, baud, [optionally
    // "altreset"]
    if ((ret = sscanf(pch, "%511s%511s%d%511s", s2, s3, &i1, s4)) < 3) {
        fprintf(stderr, "Bad vrpn_Magellan line: %s\n", line);
        return -1;
    }

    // See if we are using alternate reset line
    if (ret == 4) {
        if (strcmp(s4, "altreset") == 0) {
            altreset = true;
        }
        else {
            fprintf(stderr, "Bad vrpn_Magellan line: %s\n", line);
            return -1;
        }
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_Magellan: %s on port %s, baud %d\n", s2, s3, i1);
    }
    _devices->add(new vrpn_Magellan(s2, connection, s3, i1, altreset));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Spaceball(char *&pch, char *line,
                                                FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, magellan_name, port, baud
    if (sscanf(pch, "%511s%511s%d", s2, s3, &i1) != 3) {
        fprintf(stderr, "Bad vrpn_Spaceball line: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose)
        printf("Opening vrpn_Spaceball: %s on port %s, baud %d\n", s2, s3, i1);
    _devices->add(new vrpn_Spaceball(s2, connection, s3, i1));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Radamec_SPI(char *&pch, char *line,
                                                  FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, Radamec_name, port, baud
    if (sscanf(pch, "%511s%511s%d", s2, s3, &i1) != 3) {
        fprintf(stderr, "Bad vrpn_Radamec_SPI line: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose)
        printf("Opening vrpn_Radamec_SPI: %s on port %s, baud %d\n", s2, s3,
               i1);
    _devices->add(new vrpn_Radamec_SPI(s2, connection, s3, i1));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Zaber(char *&pch, char *line,
                                            FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, Radamec_name, port, baud
    if (sscanf(pch, "%511s%511s", s2, s3) != 2) {
        fprintf(stderr, "Bad vrpn_Zaber: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_Zaber: %s on port %s\n", s2, s3);
    }
    _devices->add(new vrpn_Zaber(s2, connection, s3));

    return 0;
}

int vrpn_Generic_Server_Object::setup_BiosciencesTools(char *&pch, char *line,
                                                       FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;
    float f1, f2;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, Radamec_name, port, baud
    if (sscanf(pch, "%511s%511s%g%g%i", s2, s3, &f1, &f2, &i1) != 5) {
        fprintf(stderr, "Bad vrpn_BiosciencesTools: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_BiosciencesTools: %s on port %s\n", s2, s3);
        printf("    Temperatures: %g %g, control %d\n", f1, f2, i1);
    }
    _devices->add(
        new vrpn_BiosciencesTools(s2, connection, s3, f1, f2, (i1 != 0)));

    return 0;
}

int vrpn_Generic_Server_Object::setup_OmegaTemperature(char *&pch, char *line,
                                                       FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;
    float f1, f2;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, Radamec_name, port, baud
    if (sscanf(pch, "%511s%511s%g%g%i", s2, s3, &f1, &f2, &i1) != 5) {
        fprintf(stderr, "Bad vrpn_OmegaTemperature: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_OmegaTemperature: %s on port %s\n", s2, s3);
        printf("    Temperatures: %g %g, control %d\n", f1, f2, i1);
    }
#if defined(VRPN_USE_MODBUS) && defined(VRPN_USE_WINSOCK2)
    _devices->add(
        new vrpn_OmegaTemperature(s2, connection, s3, f1, f2, (i1 != 0)));
#else
    fprintf(stderr, "setup_OmegaTemperature: Modbus or Winsock2 support not "
                    "configured in VRPN, edit vrpn_Configure.h and rebuild\n");
    return -1;
#endif

    return 0;
}

int vrpn_Generic_Server_Object::setup_IDEA(char *&pch, char *line,
                                           FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int run_speed, start_speed, end_speed, accel_rate, decel_rate;
    int run_current, hold_current, accel_current, decel_current;
    int delay, step, high_limit, low_limit;
    int output_1, output_2, output_3, output_4;
    double initial_move, fractional_c_a;
    double reset_location;

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, Radamec_name, port, baud
    if (sscanf(pch, "%511s%511s%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%lf%lf%lf", s2,
               s3, &run_speed, &start_speed, &end_speed, &accel_rate,
               &decel_rate, &run_current, &hold_current, &accel_current,
               &decel_current, &delay, &step, &high_limit, &low_limit,
               &output_1, &output_2, &output_3, &output_4, &initial_move,
               &fractional_c_a, &reset_location) != 22) {
        fprintf(stderr, "Bad vrpn_IDEA: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_IDEA: %s on port %s\n", s2, s3);
    }
    _devices->add(new vrpn_IDEA(
        s2, connection, s3, run_speed, start_speed, end_speed, accel_rate,
        decel_rate, run_current, hold_current, accel_current, decel_current,
        delay, step, high_limit, low_limit, output_1, output_2, output_3,
        output_4, initial_move, fractional_c_a, reset_location));

    return 0;
}

int vrpn_Generic_Server_Object::setup_NationalInstrumentsOutput(
    char *&pch, char *line, FILE * /*config_file*/)
{

    fprintf(stderr, "Warning: vrpn_NI_Analog_Output is deprecated: use "
                    "vrpn_National_Instruments instead\n");
    char s2[LINESIZE], s3[LINESIZE];
    int i1, i2;
    float f1, f2;

    VRPN_CONFIG_NEXT();
    // Get the arguments (vrpn_name, NI_board_type, num_channels, polarity,
    // min_voltage, max_voltage
    if (sscanf(pch, "%511s%511s%d%d%f%f", s2, s3, &i1, &i2, &f1, &f2) != 6) {
        fprintf(stderr, "Bad vrpn_NI_Analog_Output: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_NATIONAL_INSTRUMENTS
    // Open the device
    if (verbose) {
        printf("Opening vrpn_NI_Analog_Output: %s with %d channels\n", s2, i1);
    }
    _devices->add(new vrpn_Analog_Output_Server_NI(s2, connection, s3, i1,
                                                   i2 != 0, f1, f2));

    return 0;
#else
    fprintf(
        stderr,
        "Attempting to use National Instruments board, but not compiled in\n");
    fprintf(
        stderr,
        "  (Define VRPN_USE_NATIONAL_INSTRUMENTS in vrpn_Configuration.h\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_NationalInstruments(
    char *&pch, char *line, FILE * /*config_file*/)
{

    char s2[LINESIZE], s3[LINESIZE];
    int num_in_channels, in_polarity, in_mode, in_range, in_drive_ais, in_gain;
    int num_out_channels, out_polarity;
    float minimum_delay, min_out_voltage, max_out_voltage;

    VRPN_CONFIG_NEXT();
    // Get the arguments (vrpn_name, NI_board_type,
    //    num_in_channels, minimum_delay, in_polarity, in_mode, in_range,
    //    in_drive_ais, in_gain
    //    num_out_channels, out_polarity, min_out_voltage, max_out_voltage
    if (sscanf(pch, "%511s%511s%d%f%d%d%d%d%d%d%d%f%f", s2, s3,
               &num_in_channels, &minimum_delay, &in_polarity, &in_mode,
               &in_range, &in_drive_ais, &in_gain, &num_out_channels,
               &out_polarity, &min_out_voltage, &max_out_voltage) != 13) {
        fprintf(stderr, "Bad vrpn_National_Instruments_Server: %s\n", line);
        return -1;
    }

#if defined(VRPN_USE_NATIONAL_INSTRUMENTS) ||                                  \
    defined(VRPN_USE_NATIONAL_INSTRUMENTS_MX)
    // Open the device
    if (verbose) {
        printf("Opening vrpn_National_Instruments_Server: %s with %d in and %d "
               "out channels\n",
               s2, num_in_channels, num_out_channels);
        printf("  MinDelay %f, In polarity %d, In mode %d, In range %d\n",
               minimum_delay, in_polarity, in_mode, in_range);
        printf("  In driveAIS %d, In gain %d\n", in_drive_ais, in_gain);
        printf("  Out polarity %d, Min out voltage %f, Max out voltage %f\n",
               out_polarity, min_out_voltage, max_out_voltage);
    }
    _devices->add(new vrpn_National_Instruments_Server(
        s2, connection, s3, num_in_channels, num_out_channels, minimum_delay,
        in_polarity != 0, in_mode, in_range, in_drive_ais != 0, in_gain,
        out_polarity != 0, min_out_voltage, max_out_voltage));

    return 0;
#else
    fprintf(
        stderr,
        "Attempting to use National Instruments board, but not compiled in\n");
    fprintf(
        stderr,
        "  (Define VRPN_USE_NATIONAL_INSTRUMENTS in vrpn_Configuration.h\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_ImmersionBox(char *&pch, char *line,
                                                   FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1, i2, i3, i4;
    VRPN_CONFIG_NEXT();
    // Get the arguments (class, iboxbox_name, port, baud, numdig,
    // numana, numenc)
    if (sscanf(pch, "%511s%511s%d%d%d%d", s2, s3, &i1, &i2, &i3, &i4) != 6) {
        fprintf(stderr, "Bad vrpn_ImmersionBox line: %s\n", line);
        return -1;
    }

    // Open the box
    if (verbose)
        printf("Opening vrpn_ImmersionBox: %s on port %s, baud %d, %d digital, "
               " %d analog, %d encoders\n",
               s2, s3, i1, i2, i3, i4);

    _devices->add(new vrpn_ImmersionBox(s2, connection, s3, i1, i2, i3, i4));
    return 0;
}

int vrpn_Generic_Server_Object::setup_5dt(char *&pch, char *line,
                                          FILE * /*config_file*/)
{
    char name[LINESIZE], device[LINESIZE];
    int baud_rate, mode, tenbytes;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, 5DT_name, port, baud
    if (sscanf(pch, "%511s%511s%d%d%d", name, device, &baud_rate, &mode,
               &tenbytes) != 5) {
        fprintf(stderr, "Bad vrpn_5dt line: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_5dt: %s on port %s, baud %d\n", name, device,
               baud_rate);
    }
    _devices->add(
        new vrpn_5dt(name, connection, device, baud_rate, mode, tenbytes != 0));

    return 0;
}

int vrpn_Generic_Server_Object::setup_5dt16(char *&pch, char *line,
                                            FILE * /*config_file*/)
{
    char name[LINESIZE], device[LINESIZE];
    int baud_rate;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, 5DT_name, port, baud
    if (sscanf(pch, "%511s%511s%d", name, device, &baud_rate) != 3) {
        fprintf(stderr, "Bad vrpn_5dt16 line: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_5dt16: %s on port %s, baud %d\n", name, device,
               baud_rate);
    }
    _devices->add(new vrpn_5dt16(name, connection, device, baud_rate));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Button_USB(char *&pch, char *line,
                                                 FILE * /*config_file*/)
{
    char name[LINESIZE], deviceName[LINESIZE];

    VRPN_CONFIG_NEXT();
    // Get the arguments (button_name)
    if (sscanf(pch, "%511s%511s", name, deviceName) != 2) {
        fprintf(stderr, "Bad vrpn_Button_USB line: %s\n", line);
        return -1;
    }

    // Open the button
    if (verbose) {
        printf("Opening vrpn_Button_USB: %s \n", name);
    }
#ifdef _WIN32
    _devices->add(new vrpn_Button_USB(name, deviceName, connection));
    return 0;
#else
    printf("vrpn_Button_USB only compiled for Windows.\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Button_5DT_Server(char *&pch, char *line,
                                                        FILE * /*config_file*/)
{
    char name[LINESIZE], deviceName[LINESIZE];
    double center[16];

    VRPN_CONFIG_NEXT();
    // Get the arguments (button_name)
    if (sscanf(pch,
               "%511s%511s%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf",
               name, deviceName, &center[0], &center[1], &center[2], &center[3],
               &center[4], &center[5], &center[6], &center[7], &center[8],
               &center[9], &center[10], &center[11], &center[12], &center[13],
               &center[14], &center[15]) != 18) {
        fprintf(stderr, "Bad vrpn_Button_5dt_Server line: %s\n", line);
        return -1;
    }

    // Open the button
    if (verbose) {
        printf("Opening vrpn_Button_5dt_Server: %s \n", name);
    }
    _devices->add(
        new vrpn_Button_5DT_Server(name, deviceName, connection, center));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Wanda(char *&pch, char *line,
                                            FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;

    float fhz;
    // Get the arguments Name, Serial_Port, Baud_Rate, Min_Update_Rate
    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%511s%d %f", s2, s3, &i1, &fhz) != 4) {
        fprintf(stderr, "Bad vrpn_Wanda line: %s\n", line);
        return -1;
    }

    // Create the server
    if (verbose)
        printf("Opening vrpn_Wanda:  "
               "%s on port %s baud %d, min update rate = %.2f\n",
               s2, s3, i1, fhz);

    _devices->add(new vrpn_Wanda(s2, connection, s3, i1, fhz));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Dyna(char *&pch, char *line,
                                                   FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1, i2;
    int ret;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, sensors, port, baud)
    if ((ret = sscanf(pch, "%511s%d%511s%d", s2, &i2, s3, &i1)) != 4) {
        fprintf(stderr, "Bad vrpn_Tracker_Dyna line (ret %d): %s\n", ret, line);
        return -1;
    }

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_Dyna: %s on port %s, baud %d, "
               "%d sensors\n",
               s2, s3, i1, i2);
    _devices->add(new vrpn_Tracker_Dyna(s2, connection, i2, s3, i1));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_3DMouse(char *&pch, char *line,
                                                      FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE], s4[LINESIZE];
    int i1;
    int filtering_count = 1;
    int numparms;

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, tracker_name, port, baud, [optional
    // filtering_count])
    if ((numparms = sscanf(pch, "%511s%511s%d%511s", s2, s3, &i1, s4)) < 3) {
        fprintf(stderr, "Bad vrpn_Tracker_3DMouse line: %s\n%s %s\n", line, pch,
                s3);
        return -1;
    }

    // See if we got the optional parameter to set the filtering count
    if (numparms == 4) {
        filtering_count = atoi(s4);
        if (filtering_count < 1 || filtering_count > 5) {
            fprintf(stderr, "3DMouse: Bad filtering count optional param "
                            "(expected 1-5, got '%s')\n",
                    s4);
            return -1;
        }
    }

    // Open the tracker
    if (verbose) {
        printf("Opening vrpn_Tracker_3DMouse: %s on port %s, baud %d\n", s2, s3,
               i1);
    }

    _devices->add(
        new vrpn_Tracker_3DMouse(s2, connection, s3, i1, filtering_count));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_NovintFalcon(
    char *&pch, char *line, FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE], s4[LINESIZE], s5[LINESIZE];
    int i1;
    int numparms;

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, tracker_name, device id, grip, kinematics,
    // damp)
    if ((numparms =
             sscanf(pch, "%511s%d%511s%511s%511s", s2, &i1, s3, s4, s5)) < 2) {
        fprintf(stderr,
                "Bad vrpn_Tracker_NovintFalcon line: %s\n%s %s %s %s %s\n",
                line, pch, s2, s3, s4, s5);
        return -1;
    }

    // set damping to 0.9.
    if (numparms < 5) {
        strcpy(s5, "0.9");
    }
    // set kinematics model to "stamper", if not set
    if (numparms < 4) {
        strcpy(s4, "stamper");
    }
    // set grip to "4-button" (the default one), if not set.
    if (numparms < 3) {
        strcpy(s3, "4-button");
    }

#if defined(VRPN_USE_LIBNIFALCON)
    // Open the tracker
    if (verbose) {
        printf("Opening vrpn_Tracker_NovintFalcon: %s device: %d, grip: %s, "
               "kinematics: %s damping: %s\n",
               s2, i1, s3, s4, s5);
    }

    _devices->add(
        new vrpn_Tracker_NovintFalcon(s2, connection, i1, s3, s4, s5));
    return 0;
#else
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_Fastrak(char *&pch, char *line,
                                                      FILE *config_file)
{

    char s2[LINESIZE], s3[LINESIZE], s4[LINESIZE];
    int i1;
    int numparms;
    int do_is900_timing = 0;

    char rcmd[5000]; // Reset command to send to Fastrak
    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, port, baud, [optional IS900time])
    if ((numparms = sscanf(pch, "%511s%511s%d%511s", s2, s3, &i1, s4)) < 3) {
        fprintf(stderr, "Bad vrpn_Tracker_Fastrak line: %s\n%s %s\n", line, pch,
                s3);
        return -1;
    }

    // See if we got the optional parameter to enable IS900 timings
    if (numparms == 4) {
        if (strncmp(s4, "IS900time", strlen("IS900time")) == 0) {
            do_is900_timing = 1;
            printf(" ...using IS900 timing information\n");
        }
        else if (strcmp(s4, "/") == 0) {
            // Nothing to do
        }
        else if (strcmp(s4, "\\") == 0) {
            // Nothing to do
        }
        else {
            fprintf(stderr, "Fastrak/Isense: Bad timing optional param "
                            "(expected 'IS900time', got '%s')\n",
                    s4);
            return -1;
        }
    }

    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // Fastrak at reset time. So long as we find lines with slashes
    // at the ends, we add them to the command string to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    rcmd[0] = 0;
    while (line[strlen(line) - 2] == '\\') {
        // Read the VRPN_CONFIG_NEXT line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(
                stderr,
                "Ran past end of config file in Fastrak/Isense description\n");
            return -1;
        }

        // Copy the line into the remote command,
        // then replace \ with \015 if present
        // In any case, make sure we terminate with \015.
        strncat(rcmd, line, LINESIZE);
        if (rcmd[strlen(rcmd) - 2] == '\\') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 2] == '/') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 1] == '\n') {
            rcmd[strlen(rcmd) - 1] = '\015';
        }
        else { // Add one, we reached the EOF before CR
            rcmd[strlen(rcmd) + 1] = '\0';
            rcmd[strlen(rcmd)] = '\015';
        }
    }

    if (strlen(rcmd) > 0) {
        printf("... additional reset commands follow:\n");
        printf("%s\n", rcmd);
    }

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_Fastrak: %s on port %s, baud %d\n", s2, s3,
               i1);

    vrpn_Tracker_Fastrak *mytracker = new vrpn_Tracker_Fastrak(
        s2, connection, s3, i1, 1, 4, rcmd, do_is900_timing);
    // If the last character in the line is a front slash, '/', then
    // the following line is a command to add a Wand or Stylus to one
    // of the sensors on the tracker.  Read and parse the line after,
    // then add the devices needed to support.  Each line has two
    // arguments, the string name of the devices and the integer
    // sensor number (starting with 0) to attach the device to.
    while (line[strlen(line) - 2] == '/') {
        char lineCommand[LINESIZE];
        char lineName[LINESIZE];
        int lineSensor;

        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(
                stderr,
                "Ran past end of config file in Fastrak/Isense description\n");
            delete mytracker;
            return -1;
        }

        // Parse the line.  Both "Wand" and "Stylus" lines start with the name
        // and sensor #
        if (sscanf(line, "%511s%511s%d", lineCommand, lineName, &lineSensor) !=
            3) {
            fprintf(
                stderr,
                "Bad line in Wand/Stylus description for Fastrak/Isense (%s)\n",
                line);
            delete mytracker;
            return -1;
        }

        // See which command it is and act accordingly
        if (strcmp(lineCommand, "Wand") == 0) {
            double c0min, c0lo0, c0hi0, c0max;
            double c1min, c1lo0, c1hi0, c1max;

            // Wand line has additional scale/clip information; read it in
            if (sscanf(line, "%511s%511s%d%lf%lf%lf%lf%lf%lf%lf%lf",
                       lineCommand, lineName, &lineSensor, &c0min, &c0lo0,
                       &c0hi0, &c0max, &c1min, &c1lo0, &c1hi0, &c1max) != 11) {
                fprintf(
                    stderr,
                    "Bad line in Wand description for Fastrak/Isense (%s)\n",
                    line);
                delete mytracker;
                return -1;
            }

            if (mytracker->add_is900_analog(lineName, lineSensor, c0min, c0lo0,
                                            c0hi0, c0max, c1min, c1lo0, c1hi0,
                                            c1max)) {
                fprintf(stderr,
                        "Cannot set Wand analog for Fastrak/Isense (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            if (mytracker->add_is900_button(lineName, lineSensor, 6)) {
                fprintf(stderr,
                        "Cannot set Wand buttons for Fastrak/Isense (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            printf(" ...added Wand (%s) to sensor %d\n", lineName, lineSensor);
        }
        else if (strcmp(lineCommand, "Stylus") == 0) {
            if (mytracker->add_is900_button(lineName, lineSensor, 2)) {
                fprintf(stderr,
                        "Cannot set Stylus buttons for Fastrak/Isense (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            printf(" ...added Stylus (%s) to sensor %d\n", lineName,
                   lineSensor);
        }
        else if (strcmp(lineCommand, "FTStylus") == 0) {
            if (mytracker->add_fastrak_stylus_button(lineName, lineSensor, 1)) {
                fprintf(stderr, "Cannot set Stylus buttons for Fastrak (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            printf(" ...added FTStylus (%s) to sensor %d\n", lineName,
                   lineSensor);
        }
        else {
            fprintf(stderr, "Unknown command in Wand/Stylus description for "
                            "Fastrak/Isense (%s)\n",
                    lineCommand);
            delete mytracker;
            return -1;
        }
    }

    _devices->add(mytracker);

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Isotrak(char *&pch, char *line,
                                                      FILE *config_file)
{

    char s2[LINESIZE], s3[LINESIZE], s4[LINESIZE];
    int i1;
    int numparms;

    char rcmd[5000]; // Reset command to send to Fastrak
    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, port, baud, [optional IS900time])
    if ((numparms = sscanf(pch, "%511s%511s%d%511s", s2, s3, &i1, s4)) < 3) {
        fprintf(stderr, "Bad vrpn_Tracker_Isotrak line: %s\n%s %s\n", line, pch,
                s3);
        return -1;
    }

    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // Fastrak at reset time. So long as we find lines with slashes
    // at the ends, we add them to the command string to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    rcmd[0] = 0;
    while (line[strlen(line) - 2] == '\\') {
        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr,
                    "Ran past end of config file in Isotrak description\n");
            return -1;
        }

        // Copy the line into the remote command,
        // then replace \ with \015 if present
        // In any case, make sure we terminate with \015.
        strncat(rcmd, line, LINESIZE);
        if (rcmd[strlen(rcmd) - 2] == '\\') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 2] == '/') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 1] == '\n') {
            rcmd[strlen(rcmd) - 1] = '\015';
        }
        else { // Add one, we reached the EOF before CR
            rcmd[strlen(rcmd) + 1] = '\0';
            rcmd[strlen(rcmd)] = '\015';
        }
    }

    if (strlen(rcmd) > 0) {
        printf("... additional reset commands follow:\n");
        printf("%s\n", rcmd);
    }

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Isotrak: %s on port %s, baud %d\n", s2, s3, i1);

    vrpn_Tracker_Isotrak *mytracker =
        new vrpn_Tracker_Isotrak(s2, connection, s3, i1, 1, 4, rcmd);
    // If the last character in the line is a front slash, '/', then
    // the following line is a command to add a Wand or Stylus to one
    // of the sensors on the tracker.  Read and parse the line after,
    // then add the devices needed to support.  Each line has two
    // arguments, the string name of the devices and the integer
    // sensor number (starting with 0) to attach the device to.
    while (line[strlen(line) - 2] == '/') {
        char lineCommand[LINESIZE];
        char lineName[LINESIZE];
        int lineSensor;

        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(
                stderr,
                "Ran past end of config file in Fastrak/Isense description\n");
            delete mytracker;
            return -1;
        }

        // Parse the line.  Both "Wand" and "Stylus" lines start with the name
        // and sensor #
        if (sscanf(line, "%511s%511s%d", lineCommand, lineName, &lineSensor) !=
            3) {
            fprintf(
                stderr,
                "Bad line in Wand/Stylus description for Fastrak/Isense (%s)\n",
                line);
            delete mytracker;
            return -1;
        }

        // CBO: Added support to Isotrak stylus
        if (strcmp(lineCommand, "Stylus") == 0) {
            if (mytracker->add_stylus_button(lineName, lineSensor)) {
                fprintf(stderr, "Cannot set Stylus buttons for Isotrak (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            printf(" ...added Stylus (%s) to sensor %d\n", lineName,
                   lineSensor);
        }
        else {
            fprintf(stderr,
                    "Unknown command in Stylus description for Isotrak (%s)\n",
                    lineCommand);
            delete mytracker;
            return -1;
        }
    }

    _devices->add(mytracker);

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Liberty(char *&pch, char *line,
                                                      FILE *config_file)
{

    char s2[LINESIZE], s3[LINESIZE];
    int i1, i2;
    vrpn_Tracker_Liberty *mytracker;
    int numparms;

    char rcmd[5000]; // Reset command to send to Liberty
    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, port, baud, [whoami_len])
    numparms = sscanf(pch, "%511s%511s%d%d", s2, s3, &i1, &i2);
    if (numparms < 3) {
        fprintf(stderr, "Bad vrpn_Tracker_Liberty line: %s\n%s %s\n", line, pch,
                s3);
        return -1;
    }

    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // Liberty at reset time. So long as we find lines with slashes
    // at the ends, we add them to the command string to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    rcmd[0] = 0;
    while (line[strlen(line) - 2] == '\\') {
        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr,
                    "Ran past end of config file in Liberty description\n");
            return -1;
        }

        // Copy the line into the remote command,
        // then replace \ with \015 if present
        // In any case, make sure we terminate with \015.
        strncat(rcmd, line, LINESIZE);
        if (rcmd[strlen(rcmd) - 2] == '\\') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 2] == '/') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 1] == '\n') {
            rcmd[strlen(rcmd) - 1] = '\015';
        }
        else { // Add one, we reached the EOF before CR
            rcmd[strlen(rcmd) + 1] = '\0';
            rcmd[strlen(rcmd)] = '\015';
        }
    }

    if (strlen(rcmd) > 0) {
        printf("... additional reset commands follow:\n");
        printf("%s\n", rcmd);
    }

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_Liberty: %s on port %s, baud %d\n", s2, s3,
               i1);

    if (numparms == 3) {
        mytracker =
            new vrpn_Tracker_Liberty(s2, connection, s3, i1, 0, 8, rcmd);
    }
    else {
        mytracker =
            new vrpn_Tracker_Liberty(s2, connection, s3, i1, 0, 8, rcmd, i2);
    }

    // If the last character in the line is a front slash, '/', then
    // the following line is a command to add a Stylus to one
    // of the sensors on the tracker.  Read and parse the line after,
    // then add the devices needed to support.  Each line has two
    // arguments, the string name of the devices and the integer
    // sensor number (starting with 0) to attach the device to.
    while (line[strlen(line) - 2] == '/') {
        char lineCommand[LINESIZE];
        char lineName[LINESIZE];
        int lineSensor;

        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr,
                    "Ran past end of config file in Liberty description\n");
            delete mytracker;
            return -1;
        }

        // Parse the line.
        if (sscanf(line, "%511s%511s%d", lineCommand, lineName, &lineSensor) !=
            3) {
            fprintf(stderr, "Bad line in Stylus description for Liberty (%s)\n",
                    line);
            delete mytracker;
            return -1;
        }

        if (strcmp(lineCommand, "Stylus") == 0) {
            if (mytracker->add_stylus_button(lineName, lineSensor, 2)) {
                fprintf(stderr, "Cannot set Stylus buttons for Liberty (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            printf(" ...added Stylus (%s) to sensor %d\n", lineName,
                   lineSensor);
        }
        else {
            fprintf(stderr,
                    "Unknown command inStylus description for Liberty (%s)\n",
                    lineCommand);
            delete mytracker;
            return -1;
        }
    }
    _devices->add(mytracker);

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_LibertyHS(char *&pch, char *line,
                                                        FILE *config_file)
{
    char s2[LINESIZE];
    int i1, i2, i3;
    int numparms;

    char rcmd[5000]; // Reset command to send to LibertyHS
    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, num_sensors, baud, [whoami_len])
    numparms = sscanf(pch, "%511s%d%d%d", s2, &i1, &i2, &i3);
    if (numparms < 3) {
        fprintf(stderr, "Bad vrpn_Tracker_LibertyHS line: %s\n", line);
        return -1;
    }

    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // LibertyHS at reset time. So long as we find lines with slashes
    // at the ends, we add them to the command string to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    rcmd[0] = 0;
    while (line[strlen(line) - 2] == '\\') {
        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr,
                    "Ran past end of config file in LibertyHS description\n");
            return -1;
        }

        // Copy the line into the remote command,
        // then replace \ with \015 if present
        // In any case, make sure we terminate with \015.
        strncat(rcmd, line, LINESIZE);
        if (rcmd[strlen(rcmd) - 2] == '\\') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 2] == '/') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 1] == '\n') {
            rcmd[strlen(rcmd) - 1] = '\015';
        }
        else { // Add one, we reached the EOF before CR
            rcmd[strlen(rcmd) + 1] = '\0';
            rcmd[strlen(rcmd)] = '\015';
        }
    }

    if (strlen(rcmd) > 0) {
        printf("... additional reset commands follow:\n");
        printf("%s\n", rcmd);
    }

#if defined(VRPN_USE_LIBUSB_1_0)
    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_LibertyHS: %s on USB port, baud %d\n", s2,
               i2);

    if (numparms == 3) {
        _devices->add(
            new vrpn_Tracker_LibertyHS(s2, connection, i2, 0, i1, 1, rcmd));
    }
    else {
        _devices->add(
            new vrpn_Tracker_LibertyHS(s2, connection, i2, 0, i1, 1, rcmd, i3));
    }
    return 0;
#else
    printf("Can't create new vrpn_Tracker_LibertyHS: Server not compiled with "
           "VRPN_USE_LIBUSB_1_0 defined.\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_3Space(char *&pch, char *line,
                                                     FILE * /*config_file*/)
{

    char s2[LINESIZE], s3[LINESIZE];
    int i1;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, port, baud)
    if (sscanf(pch, "%511s%511s%d", s2, s3, &i1) != 3) {
        fprintf(stderr, "Bad vrpn_Tracker_3Space line: %s\n", line);
        return -1;
    }

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_3Space: %s on port %s, baud %d\n", s2, s3,
               i1);
    _devices->add(new vrpn_Tracker_3Space(s2, connection, s3, i1));
    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Flock(char *&pch, char *line,
                                                    FILE * /*config_file*/)
{

    char s2[LINESIZE], s3[LINESIZE];
    int i1, i2, i3;
    char useERT[LINESIZE];
    strcpy(useERT, "y");
    char hemi[LINESIZE];
    strcpy(hemi, "+z");
    bool invertQuaternion;

    VRPN_CONFIG_NEXT();
    // Get the arguments (tracker_name, sensors, port, baud, invert_quat,
    // useERT, active_hemisphere)
    int nb = sscanf(pch, "%511s%d%511s%d%d%511s%511s", s2, &i1, s3, &i2, &i3,
                    useERT, hemi);
    if ((nb != 5) && (nb != 6) && (nb != 7)) {
        fprintf(stderr, "Bad vrpn_Tracker_Flock line: %s\n", line);
        return -1;
    }

    // Interpret the active hemisphere parameter
    int hemi_id;
    if (hemi[0] == '-') {
        if (hemi[1] == 'x' || hemi[1] == 'X') {
            hemi_id = vrpn_Tracker_Flock::HEMI_MINUSX;
        }
        else if (hemi[1] == 'y' || hemi[1] == 'Y') {
            hemi_id = vrpn_Tracker_Flock::HEMI_MINUSY;
        }
        else if (hemi[1] == 'z' || hemi[1] == 'Z') {
            hemi_id = vrpn_Tracker_Flock::HEMI_MINUSZ;
        }
        else {
            fprintf(stderr, "Bad vrpn_Tracker_Flock line: %s\n", line);
            return -1;
        }
    }
    else if (hemi[0] == '+') {
        if (hemi[1] == 'x' || hemi[1] == 'X') {
            hemi_id = vrpn_Tracker_Flock::HEMI_PLUSX;
        }
        else if (hemi[1] == 'y' || hemi[1] == 'Y') {
            hemi_id = vrpn_Tracker_Flock::HEMI_PLUSY;
        }
        else if (hemi[1] == 'z' || hemi[1] == 'Z') {
            hemi_id = vrpn_Tracker_Flock::HEMI_PLUSZ;
        }
        else {
            fprintf(stderr, "Bad vrpn_Tracker_Flock line: %s\n", line);
            return -1;
        }
    }
    else {
        fprintf(stderr, "Bad vrpn_Tracker_Flock line: %s\n", line);
        return -1;
    }

    // Open the tracker
    bool buseERT = true;
    if ((useERT[0] == 'n') || (useERT[0] == 'N')) {
        buseERT = false;
    }
    invertQuaternion = (i3 != 0);
    if (verbose) {
        printf("Opening vrpn_Tracker_Flock: "
               "%s (%d sensors, on port %s, baud %d) %s ERT\n",
               s2, i1, s3, i2, buseERT ? "with" : "without");
    }
    _devices->add(new vrpn_Tracker_Flock(s2, connection, i1, s3, i2, 1, buseERT,
                                         invertQuaternion, hemi_id));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Flock_Parallel(
    char *&pch, char *line, FILE * /*config_file*/)
{

    char s2[LINESIZE], s3[LINESIZE];
    int i1, i2, i3;
    bool invertQuaternion;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, sensors, port, baud,
    // invertQuaternion
    // and parallel sensor ports )

    if (sscanf(pch, "%511s%d%511s%d%d", s2, &i1, s3, &i2, &i3) != 5) {
        fprintf(stderr, "Bad vrpn_Tracker_Flock_Parallel line: %s\n", line);
        return -1;
    }

    // set up strtok to get the variable num of port names
    char rgch[24];
    sprintf(rgch, "%d", i2);
    char *pch2 = strstr(pch, rgch);
    strtok(pch2, " \t");
    // pch points to baud, next strtok will give invertQuaternion
    strtok(NULL, " \t");
    // pch points to invertQuaternion, next strtok will give first port name

    char *rgs[VRPN_FLOCK_MAX_SENSORS];
    // get sensor ports
    for (int iSlaves = 0; iSlaves < i1; iSlaves++) {
        rgs[iSlaves] = new char[LINESIZE];
        if (!(pch2 = strtok(NULL, " \t"))) {
            fprintf(stderr, "Bad vrpn_Tracker_Flock_Parallel line: %s\n", line);
            return -1;
        }
        else {
            sscanf(pch2, "%511s", rgs[iSlaves]);
        }
    }

    // Open the tracker
    invertQuaternion = (i3 != 0);
    if (verbose)
        printf("Opening vrpn_Tracker_Flock_Parallel: "
               "%s (%d sensors, on port %s, baud %d)\n",
               s2, i1, s3, i2);
    _devices->add(new vrpn_Tracker_Flock_Parallel(s2, connection, i1, s3, i2,
                                                  rgs, invertQuaternion));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_NULL(char *&pch, char *line,
                                                   FILE * /*config_file*/)
{

    char s2[LINESIZE];
    int i1;
    float f1;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, sensors, rate)
    if (sscanf(pch, "%511s%d%g", s2, &i1, &f1) != 3) {
        fprintf(stderr, "Bad vrpn_Tracker_NULL line: %s\n", line);
        return -1;
    }

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_NULL: %s with %d sensors, rate %f\n", s2,
               i1, f1);
    _devices->add(new vrpn_Tracker_NULL(s2, connection, i1, f1));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_Spin(char *&pch, char *line,
  FILE * /*config_file*/)
{

  char s2[LINESIZE];
  int i1;
  float report, x, y, z, spin;

  VRPN_CONFIG_NEXT();
  // Get the arguments (class, tracker_name, sensors, rate)
  if (sscanf(pch, "%511s%d%g%g%g%g%g", s2, &i1, &report, &x, &y, &z, &spin) != 7) {
    fprintf(stderr, "Bad vrpn_Tracker_Spin line: %s\n", line);
    return -1;
  }

  // Open the tracker
  if (verbose)
    printf("Opening vrpn_Tracker_Spin: %s with %d sensors, rate %f\n", s2,
    i1, report);
  _devices->add(new vrpn_Tracker_Spin(s2, connection, i1, report, x, y, z, spin));

  return 0;
}

int vrpn_Generic_Server_Object::setup_Button_Python(char *&pch, char *line,
                                                    FILE * /*config_file*/)
{

    char s2[LINESIZE];
    int i1, i2;

    VRPN_CONFIG_NEXT();
    i2 = 0; // Set it to use the default value if we don't read a value from the
            // file.
    // Get the arguments (class, button_name, which_lpt,
    // optional_hex_port_number)
    if (sscanf(pch, "%511s%d%x", s2, &i1, &i2) < 2) {
        fprintf(stderr, "Bad vrpn_Button_Python line: %s\n", line);
        return -1;
    }

    // Open the button
    if (verbose) printf("Opening vrpn_Button_Python: %s on port %d\n", s2, i1);
    _devices->add(new vrpn_Button_Python(s2, connection, i1, i2));

    return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_Button_SerialMouse(char *&pch, char *line,
                                                         FILE * /*config_file*/)
{

    char s2[LINESIZE];
    char s3[LINESIZE];
    char s4[32];
    vrpn_MOUSETYPE mType = MAX_MOUSE_TYPES;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, button_name, portname, type)
    if (sscanf(pch, "%511s%511s%31s", s2, s3, s4) != 3) {
        fprintf(stderr, "Bad vrpn_Button_SerialMouse line: %s\n", line);
        return -1;
    }

    if (strcmp(s4, "mousesystems") == 0) {
        mType = MOUSESYSTEMS;
    }
    else if (strcmp(s4, "3button") == 0) {
        mType = THREEBUTTON_EMULATION;
    }

    if (mType == MAX_MOUSE_TYPES) {
        fprintf(stderr, "bad vrpn_SerialMouse_Button type\n");
        return -1;
    }

    // Open the button
    if (verbose) {
        printf("Opening vrpn_SerialMouse_Button: %s on port %s\n", s2, s3);
    }
    _devices->add(new vrpn_Button_SerialMouse(s2, connection, s3, 1200, mType));
    return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_Button_PinchGlove(char *&pch, char *line,
                                                        FILE * /*config_file*/)
{

    char name[LINESIZE], port[LINESIZE];
    int baud;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, button_name, port, baud)
    if (sscanf(pch, "%511s%511s%d", name, port, &baud) != 3) {
        fprintf(stderr, "Bad vrpn_Button_PinchGlove line: %s\n", line);
        return -1;
    }

    // Open the button
    if (verbose) {
        printf("Opening vrpn_Button_PinchGlove: %s on port %s at %d baud\n",
               name, port, baud);
    }
    _devices->add(new vrpn_Button_PinchGlove(name, connection, port, baud));

    return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_DevInput(char *&pch, char *line,
                                               FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE], s4[LINESIZE];
    int int_param = 0;
    VRPN_CONFIG_NEXT();

    // Get the arguments (class, dev_input_name)
    if (sscanf(pch, "%511s \"%[^\"]\" %s %d", s2, s3, s4, &int_param) != 4) {
        if (sscanf(pch, "%511s \"%[^\"]\" %s", s2, s3, s4) != 3) {
            fprintf(stderr, "Bad vrpn_DevInput line: %s\n", line);
            return -1;
        }
    }

#ifdef VRPN_USE_DEV_INPUT
    _devices->add(new vrpn_DevInput(s2, connection, s3, s4, int_param));
    return 0;
#else
    fprintf(stderr, "vrpn_DevInput support not compiled in\n");
    return -1;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_Joylin(char *&pch, char *line,
                                             FILE * /*config_file*/)
{
    char s2[LINESIZE];
    char s3[LINESIZE];

    // Get the arguments
    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%511s", s2, s3) != 2) {
        fprintf(stderr, "Bad vrpn_Joylin line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_JOYLIN
    // Open the joystick server
    if (verbose) {
        printf("Opening vrpn_Joylin: %s on port %s\n", s2, s3);
    }
    _devices->add(new vrpn_Joylin(s2, connection, s3));
    return 0;
#else
    fprintf(stderr, "vrpn_Joylin support not compiled in\n");
    return -1;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_Joywin32(char *&pch, char *line,
                                               FILE * /*config_file*/)
{
    char s2[LINESIZE];
    int joyId;
    int readRate;
    int mode;
    int deadZone;

    // Get the arguments
    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%d%d%d%d", s2, &joyId, &readRate, &mode, &deadZone) !=
        5) {
        fprintf(stderr, "Bad vrpn_Joywin32 line: %s\n", line);
        return -1;
    }
#if defined(_WIN32)

    // Open the joystick server
    if (verbose) {
        printf("Opening vrpn_Joywin32: %s (device %d), baud rate:%d, mode:%d, "
               "dead zone:%d\n",
               s2, joyId, readRate, mode, deadZone);
    }
    _devices->add(
        new vrpn_Joywin32(s2, connection, joyId, readRate, mode, deadZone));
    return 0;
#else
    fprintf(stderr, "Joywin32 is for use under Windows only");
    return -2;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_StreamingArduino(char *&pch, char *line,
                                           FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int numAna,baud;
    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tng3_name, port, numana, baud)
    // Baud is optional, and may not be listed.  If not, we default
    // to 115200.
    baud = 115200;
    if (sscanf(pch, "%511s%511s%d%d", s2, s3, &numAna, &baud) < 3) {
        fprintf(stderr, "Bad vrpn_Streaming_Arduino line: %s\n", line);
        return -1;
    }
    // Open the device
    if (verbose)
        printf("Opening vrpn_Streaming_Arduino: %s on port %s, baud %d, "
               " %d analog\n",
               s2, s3, baud, numAna);
    _devices->add(new vrpn_Streaming_Arduino(s2, connection, s3, numAna, baud));
    return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_Tng3(char *&pch, char *line,
  FILE * /*config_file*/)
{
  char s2[LINESIZE], s3[LINESIZE];
  int i1, i2;
  VRPN_CONFIG_NEXT();
  // Get the arguments (class, tng3_name, port, numdig, numana)
  if (sscanf(pch, "%511s%511s%d%d", s2, s3, &i1, &i2) != 4) {
    fprintf(stderr, "Bad vrpn_Tng3 line: %s\n", line);
    return -1;
  }
  // Open the box
  if (verbose)
    printf("Opening vrpn_Tng3: %s on port %s, baud %d, %d digital, "
    " %d analog\n",
    s2, s3, 19200, i1, i2);
  _devices->add(new vrpn_Tng3(s2, connection, s3, 19200, i1, i2));
  return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_Mouse(char *&pch, char *line,
                                            FILE * /*config_file*/)
{
    char s2[LINESIZE];
    VRPN_CONFIG_NEXT();

    // Get the arguments (class, mouse_name)
    if (sscanf(pch, "%511s", s2) != 1) {
        fprintf(stderr, "Bad vrpn_Mouse line: %s\n", line);
        return -1;
    }

    // Open the box
    if (verbose) {
        printf("Opening vrpn_Mouse: %s\n", s2);
    }

    try {
        _devices->add(new vrpn_Mouse(s2, connection));
    }
    catch (...) {
        fprintf(stderr, "could not create vrpn_Mouse\n");
#ifdef linux
        fprintf(stderr, "- Is the GPM server running?\n");
        fprintf(stderr,
                "- Are you running on a linux console (not an xterm)?\n");
#endif
        return -1;
    }
    return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_Tracker_Crossbow(char *&pch, char *line,
                                                       FILE * /*config_file*/)
{
    char port[LINESIZE], name[LINESIZE];
    long baud;
    float gRange, aRange;

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, tracker_name, port, baud, g-range, a-range)
    // g-range is the linear acceleration sensitivity in Gs
    // a-range is the angular velocity sensitivity in degrees per second
    if (sscanf(pch, "%511s%511s%ld%f%f", name, port, &baud, &gRange, &aRange) <
        5) {
        fprintf(stderr, "Bad vrpn_Tracker_Crossbow line: %s\n%s %s\n", line,
                pch, port);
        return -1;
    }

    if (verbose)
        printf("Opening vrpn_Tracker_Crossbow: %s on %s with baud %ld, G-range "
               "%f, and A-range %f\n",
               name, port, baud, gRange, aRange);

    _devices->add(new vrpn_Tracker_Crossbow(name, connection, port, baud,
                                            gRange, aRange));

    return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_3DMicroscribe(char *&pch, char *line,
                                                    FILE * /*config_file*/)
{
    char name[LINESIZE], device[LINESIZE];
    int baud_rate;
    float x, y, z, s;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, 5DT_name, port, baud, xoff, yoff, zoff, scale)
    if (sscanf(pch, "%511s%511s%d%f%f%f%f", name, device, &baud_rate, &x, &y,
               &z, &s) != 7) {
        fprintf(stderr, "Bad vrpn_3dMicroscribe line: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose) {
        printf("Opening 3dMicroscribe: %s on port %s, baud %d\n", name, device,
               baud_rate);
    }
#ifdef VRPN_USE_MICROSCRIBE
    _devices->add(new vrpn_3DMicroscribe(name, connection, device, baud_rate, x,
                                         y, z, s));
    return 0;
#else
    fprintf(stderr, "3DMicroscribe support not configured in VRPN, edit "
                    "vrpn_Configure.h and rebuild\n");
    return -1;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_Tracker_InterSense(char *&pch, char *line,
                                                         FILE *config_file)
{
    char trackerName[LINESIZE];
    char commStr[LINESIZE];
    int commPort;

    char s4[LINESIZE];
    char s5[LINESIZE];
    char s6[LINESIZE];
    int numparms;
    int do_is900_timing = 0;
    int reset_at_start = 0; // nahon@virtools.com

    char rcmd[5000]; // Reset command to send to Intersense

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, tracker_name, port, [optional IS900time])
    sscanf(line, "vrpn_Tracker_InterSense %s %s", trackerName, commStr);
    if ((numparms = sscanf(pch, "%511s%511s%511s%511s%511s", trackerName,
                           commStr, s4, s5, s6)) < 2) {
        fprintf(stderr, "Bad vrpn_Tracker_InterSense line: %s\n%s %s\n", line,
                pch, trackerName);
        return -1;
    }

    if (!strcmp(commStr, "COM1")) {
        commPort = 1;
    }
    else if (!strcmp(commStr, "COM2")) {
        commPort = 2;
    }
    else if (!strcmp(commStr, "COM3")) {
        commPort = 3;
    }
    else if (!strcmp(commStr, "COM4")) {
        commPort = 4;
    }
    else if (!strcmp(commStr, "AUTO")) {
        commPort = 0; // the intersense SDK will find the tracker on any COM or
                      // USB port.
    }
    else {
        fprintf(stderr, "Error determining COM port: %s not should be either "
                        "COM1, COM2, COM3, COM4 or AUTO",
                commStr);
        return -1;
    }
    int found_a_valid_option = 0;
    // See if we got the optional parameter to enable IS900 timings
    if (numparms >= 3) {
        if ((strncmp(s4, "IS900time", strlen("IS900time")) == 0) ||
            (strncmp(s5, "IS900time", strlen("IS900time")) == 0) ||
            (strncmp(s6, "IS900time", strlen("IS900time")) == 0)) {
            do_is900_timing = 1;
            found_a_valid_option = 1;
            printf(" ...using IS900 timing information\n");
        }
        if ((strncmp(s4, "ResetAtStartup", strlen("ResetAtStartup")) == 0) ||
            (strncmp(s5, "ResetAtStartup", strlen("ResetAtStartup")) == 0) ||
            (strncmp(s6, "ResetAtStartup", strlen("ResetAtStartup")) == 0)) {
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
            fprintf(stderr, "InterSense: Bad optional param (expected "
                            "'IS900time', 'ResetAtStartup' and/or "
                            "'SensorsStartsAtOne', got '%s %s %s')\n",
                    s4, s5, s6);
            return -1;
        }
    }

    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // Fastrak at reset time. So long as we find lines with slashes
    // at the ends, we add them to the command string to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    rcmd[0] = '\0';
    while (line[strlen(line) - 2] == '\\') {
        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr,
                    "Ran past end of config file in Isense description\n");
            return -1;
        }

        // Copy the line into the remote command,
        // then replace \ with \015 if present
        // In any case, make sure we terminate with \015.
        strncat(rcmd, line, LINESIZE);
        if (rcmd[strlen(rcmd) - 2] == '\\') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 2] == '/') {
            rcmd[strlen(rcmd) - 2] = '\015';
            rcmd[strlen(rcmd) - 1] = '\0';
        }
        else if (rcmd[strlen(rcmd) - 1] == '\n') {
            rcmd[strlen(rcmd) - 1] = '\015';
        }
        else { // Add one, we reached the EOF before CR
            rcmd[strlen(rcmd) + 1] = '\0';
            rcmd[strlen(rcmd)] = '\015';
        }
    }

    if (strlen(rcmd) > 0) {
        printf("... additional reset commands follow:\n");
        printf("%s\n", rcmd);
    }

#ifdef VRPN_INCLUDE_INTERSENSE

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_InterSense: %s on port %s\n", trackerName,
               commStr);

    vrpn_Tracker_InterSense *mytracker =
        new vrpn_Tracker_InterSense(trackerName, connection, commPort, rcmd,
                                    do_is900_timing, reset_at_start);

    // If the last character in the line is a front slash, '/', then
    // the following line is a command to add a Wand or Stylus to one
    // of the sensors on the tracker.  Read and parse the line after,
    // then add the devices needed to support.  Each line has two
    // arguments, the string name of the devices and the integer
    // sensor number (starting with 0) to attach the device to.
    while (line[strlen(line) - 2] == '/') {
        char lineCommand[LINESIZE];
        char lineName[LINESIZE];
        int lineSensor;

        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr,
                    "Ran past end of config file in InterSense description\n");
            delete mytracker;
        }

        // Parse the line.  Both "Wand" and "Stylus" lines start with the name
        // and sensor #
        if (sscanf(line, "%511s%511s%d", lineCommand, lineName, &lineSensor) !=
            3) {
            fprintf(stderr,
                    "Bad line in Wand/Stylus description for InterSense (%s)\n",
                    line);
            delete mytracker;
            return -1;
        }

        // See which command it is and act accordingly
        if (strcmp(lineCommand, "Wand") == 0) {
            double c0min, c0lo0, c0hi0, c0max;
            double c1min, c1lo0, c1hi0, c1max;

            // Wand line has additional scale/clip information; read it in
            if (sscanf(line, "%511s%511s%d%lf%lf%lf%lf%lf%lf%lf%lf",
                       lineCommand, lineName, &lineSensor, &c0min, &c0lo0,
                       &c0hi0, &c0max, &c1min, &c1lo0, &c1hi0, &c1max) != 11) {
                fprintf(stderr,
                        "Bad line in Wand description for InterSense (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }

            if (mytracker->add_is900_analog(lineName, lineSensor, c0min, c0lo0,
                                            c0hi0, c0max, c1min, c1lo0, c1hi0,
                                            c1max)) {
                fprintf(stderr, "Cannot set Wand analog for InterSense (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            if (mytracker->add_is900_button(lineName, lineSensor, 6)) {
                fprintf(stderr, "Cannot set Wand buttons for InterSense (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            printf(" ...added Wand (%s) to sensor %d\n", lineName, lineSensor);
        }
        else if (strcmp(lineCommand, "Stylus") == 0) {
            if (mytracker->add_is900_button(lineName, lineSensor, 2)) {
                fprintf(stderr,
                        "Cannot set Stylus buttons for InterSense (%s)\n",
                        line);
                delete mytracker;
                return -1;
            }
            printf(" ...added Stylus (%s) to sensor %d\n", lineName,
                   lineSensor);
        }
        else {
            fprintf(stderr, "Unknown command in Wand/Stylus description for "
                            "InterSense (%s)\n",
                    lineCommand);
            delete mytracker;
            return -1;
        }
    }

    _devices->add(mytracker);

    return 0;

#else
    fprintf(stderr, "vrpn_server: Can't open Intersense native server: "
                    "VRPN_INCLUDE_INTERSENSE not defined in "
                    "vrpn_Configure.h!\n");
    return -1;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_DirectXFFJoystick(char *&pch, char *line,
                                                        FILE * /*config_file*/)
{
    char s2[LINESIZE];
    float f1, f2;

    VRPN_CONFIG_NEXT();
    // Get the arguments (joystick_name, read update rate, force update rate)
    if (sscanf(pch, "%511s%g%g", s2, &f1, &f2) != 3) {
        fprintf(stderr, "Bad vrpn_DirectXFFJoystick line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_DIRECTINPUT
    // Open the joystick
    if (verbose) {
        printf(
            "Opening vrpn_DirectXFFJoystick: %s, read rate %g, force rate %g\n",
            s2, f1, f2);
    }
    _devices->add(new vrpn_DirectXFFJoystick(s2, connection, f1, f2));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open DirectXFFJoystick: "
                    "VRPN_USE_DIRECTINPUT not defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_RumblePad(char *&pch, char *line,
                                                FILE * /*config_file*/)
{
    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    // Get the arguments (joystick_name, read update rate, force update rate)
    if (sscanf(pch, "%511s", s2) != 1) {
        fprintf(stderr, "Bad vrpn_DirectXRumblePad line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_DIRECTINPUT
    // Open the joystick
    if (verbose) {
        printf("Opening vrpn_DirectXRumblePad: %s\n", s2);
    }
    _devices->add(new vrpn_DirectXRumblePad(s2, connection));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open RumblePad: VRPN_USE_DIRECTINPUT "
                    "not defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

//================================
int vrpn_Generic_Server_Object::setup_XInputPad(char *&pch, char *line,
                                                FILE * /*config_file*/)
{
    char s2[LINESIZE];
    unsigned controller;

    VRPN_CONFIG_NEXT();
    // Get the arguments (joystick_name, controller index)
    if (sscanf(pch, "%511s%u", s2, &controller) != 2) {
        fprintf(stderr, "Bad vrpn_XInputGamepad line: %s\n", line);
        return -1;
    }

#if defined(VRPN_USE_DIRECTINPUT) && defined(VRPN_USE_WINDOWS_XINPUT)
    // Open the joystick
    if (verbose) {
        printf("Opening vrpn_XInputGamepad: %s\n", s2);
    }
    _devices->add(new vrpn_XInputGamepad(s2, connection, controller));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open XInputGamepad: "
                    "VRPN_USE_DIRECTINPUT and/or VRPN_USE_WINDOWS_XINPUT not "
                    "defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_GlobalHapticsOrb(char *&pch, char *line,
                                                       FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;

    VRPN_CONFIG_NEXT();
    // Get the arguments (orb_name, port name, baud rate)
    if (sscanf(pch, "%511s%s%d", s2, s3, &i1) != 3) {
        fprintf(stderr, "Bad vrpn_GlobalHapticsOrb line: %s\n", line);
        return -1;
    }

    // Open the orb
    if (verbose) {
        printf("Opening vrpn_GlobalHapticsOrb: %s, port %s, baud rate %d\n", s2,
               s3, i1);
    }
    _devices->add(new vrpn_GlobalHapticsOrb(s2, connection, s3, i1));

    return 0;
}

//================================
int vrpn_Generic_Server_Object::setup_ADBox(char *&pch, char *line,
                                            FILE * /*config_file*/)
{

    char name[LINESIZE], port[LINESIZE];
    int baud;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, button_name, port, baud)
    if (sscanf(pch, "%511s%511s%d", name, port, &baud) != 3) {
        fprintf(stderr, "Bad vrpn_ADBox line: %s\n", line);
        return -1;
    }

    // Open the button
    if (verbose) {
        printf("Opening vrpn_ADBox: %s on port %s at %d baud\n", name, port,
               baud);
    }

    _devices->add(new vrpn_ADBox(name, connection, port, baud));

    return 0;
}

int vrpn_Generic_Server_Object::setup_VPJoystick(char *&pch, char *line,
                                                 FILE * /*config_file*/)
{
    char name[LINESIZE], port[LINESIZE];
    int baud;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, button_name, port, baud)
    if (sscanf(pch, "%511s%511s%d", name, port, &baud) != 3) {
        fprintf(stderr, "Bad vrpn_VPJoystick line: %s\n", line);
        return -1;
    }

    // Open the button
    if (verbose) {
        printf("Opening vrpn_VPJoystick: %s on port %s at %d baud\n", name,
               port, baud);
    }

    _devices->add(new vrpn_VPJoystick(name, connection, port, baud));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_JsonNet(char *&pch, char *line,
                                                      FILE * /*config_file*/)
{
    /*
     * Parses the section of the configuration file related to the device
      #DeviceClass DeviceName ListenPort
      Tracker_JsonNet GGG 7777
     *
     * Create the device object
     */
    char name[LINESIZE];
    int port;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, device_name, port)
    if (sscanf(pch, "%511s%d", name, &port) != 2) {
        fprintf(stderr, "Bad vrpn_Tracker_JsonNet line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_JSONNET
    // Open vrpn_Tracker_JsonNet:

    if (verbose) {
        printf("Opening vrpn_Tracker_JsonNet: %s at port %d\n", name, port);
    }

    _devices->add(new vrpn_Tracker_JsonNet(name, connection, port));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open vrpn_Tracker_JsonNet: "
                    "VRPN_USE_JSONNET not defined in vrpn_Configure.h!\n");
    return -1;
#endif // VRPN_USE_JsonNet
}

int vrpn_Generic_Server_Object::setup_DTrack(char *&pch, char *line,
                                             FILE * /*config_file*/)
{
    char *s2;
    char *str[LINESIZE];
    char *s;
    char sep[] = " ,\t,\n";
    int count = 0;
    int dtrackPort, isok;
    float timeToReachJoy;
    int nob, nof, nidbf;
    int idbf[512];
    bool actTracing, act3DOFout;

    VRPN_CONFIG_NEXT();

    // Get the arguments:

    str[count] = strtok(pch, sep);
    while (str[count] != NULL) {
        count++;
        // Get next token:
        str[count] = strtok(NULL, sep);
    }

    if (count < 2) {
        fprintf(stderr, "Bad vrpn_Tracker_DTrack line: %s\n", line);
        return -1;
    }

    s2 = str[0];
    dtrackPort = (int)strtol(str[1], &s, 0);

    // tracing/3dof (optional; always last arguments):

    actTracing = false;
    act3DOFout = false;

    isok = 1;
    while (isok && count > 2) {
        isok = 0;

        if (!strcmp(str[count - 1], "-")) {
            actTracing = true;
            count--;
            isok = 1;
        }
        else if (!strcmp(str[count - 1], "3d")) {
            act3DOFout = true;
            count--;
            isok = 1;
        }
    }

    // time needed to reach the maximum value of the joystick (optional):

    timeToReachJoy = 0;

    if (count > 2) {
        timeToReachJoy = (float)strtod(str[2], &s);
    }

    // number of bodies and flysticks (optional):

    nob = nof = -1;

    if (count > 3) {
        if (count < 5) { // there must be two additional arguments at least
            fprintf(stderr, "Bad vrpn_Tracker_DTrack line: %s\n", line);
            return -1;
        }

        nob = (int)strtol(str[3], &s, 0);
        nof = (int)strtol(str[4], &s, 0);
    }

    // renumbering of targets (optional):

    nidbf = 0;

    if (count > 5) {
        if (count <
            5 + nob + nof) { // there must be an argument for each target
            fprintf(stderr, "Bad vrpn_Tracker_DTrack line: %s\n", line);
            return -1;
        }

        for (int i = 0; i < nob + nof; i++) {
            idbf[i] = (int)strtol(str[5 + i], &s, 0);
            nidbf++;
        }
    }

    // Open vrpn_Tracker_DTrack:

    int *pidbf = NULL;

    if (nidbf > 0) {
        pidbf = idbf;
    }

    if (verbose) {
        printf(
            "Opening vrpn_Tracker_DTrack: %s at port %d, timeToReachJoy %.2f",
            s2, dtrackPort, timeToReachJoy);
        if (nob >= 0 && nof >= 0) {
            printf(", fixNtargets %d %d", nob, nof);
        }
        if (nidbf > 0) {
            printf(", fixId");
            for (int i = 0; i < nidbf; i++) {
                printf(" %d", idbf[i]);
            }
        }
        printf("\n");
    }

#ifndef sgi

    _devices->add(new vrpn_Tracker_DTrack(s2, connection, dtrackPort,
                                          timeToReachJoy, nob, nof, pidbf,
                                          act3DOFout, actTracing));

    return 0;
#else
    fprintf(stderr, "vrpn_Tracker_DTrack not supported on this architecture\n");
    return -1;
#endif
}

// This function will read one line of the vrpn_Poser_Analog configuration
// (matching
// one axis) and fill in the data for that axis. The axis name, the file to read
// from, and the axis to fill in are passed as parameters. It returns 0 on
// success
// and -1 on failure.

int vrpn_Generic_Server_Object::get_poser_axis_line(FILE *config_file,
                                                    const char *axis_name,
                                                    vrpn_PA_axis *axis,
                                                    vrpn_float64 *min,
                                                    vrpn_float64 *max)
{
    char line[LINESIZE];
    char _axis_name[LINESIZE];
    char *name = new char[LINESIZE]; // We need this to stay around for the param
    int channel;
    float offset, scale;

    // Read in the line
    if (fgets(line, LINESIZE, config_file) == NULL) {
        perror("Poser Analog Axis: Can't read axis");
        return -1;
    }

    // Get the values from the line
    if (sscanf(line, "%511s%511s%d%g%g%lg%lg", _axis_name, name, &channel,
               &offset, &scale, min, max) != 7) {
        fprintf(stderr, "Poser Analog Axis: Bad axis line\n");
        return -1;
    }

    // Check to make sure the name of the line matches
    if (strcmp(_axis_name, axis_name) != 0) {
        fprintf(stderr, "Poser Analog Axis: wrong axis: wanted %s, got %s)\n",
                axis_name, name);
        return -1;
    }

    // Fill in the values if we didn't get the name "NULL". Otherwise, just
    // leave them as they are, and they will have no effect.
    if (strcmp(name, "NULL") != 0) {
        axis->ana_name = name;
        axis->channel = channel;
        axis->offset = offset;
        axis->scale = scale;
    } else {
        delete [] name;
    }

    return 0;
}

int vrpn_Generic_Server_Object::setup_Poser_Analog(char *&pch, char *line,
                                                   FILE *config_file)
{
    char s2[LINESIZE];
    int i1;
    vrpn_Poser_AnalogParam p;

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%d", s2, &i1) != 2) {
        fprintf(stderr, "Bad vrpn_Poser_Analog line: %s\n", line);
        return -1;
    }

    if (verbose) {
        printf("Opening vrpn_Poser_Analog: "
               "%s\n",
               s2);
    }

    // Scan the following lines in the configuration file to fill
    // in the start-up parameters for the different axes.

    if (get_poser_axis_line(config_file, "X", &p.x, &p.pos_min[0],
                            &p.pos_max[0])) {
        fprintf(stderr, "Can't read X line for Poser Analog\n");
        return -1;
    }
    if (get_poser_axis_line(config_file, "Y", &p.y, &p.pos_min[1],
                            &p.pos_max[1])) {
        fprintf(stderr, "Can't read y line for Poser Analog\n");
        return -1;
    }
    if (get_poser_axis_line(config_file, "Z", &p.z, &p.pos_min[2],
                            &p.pos_max[2])) {
        fprintf(stderr, "Can't read Z line for Poser Analog\n");
        return -1;
    }
    if (get_poser_axis_line(config_file, "RX", &p.rx, &p.pos_rot_min[0],
                            &p.pos_rot_max[0])) {
        fprintf(stderr, "Can't read RX line for Poser Analog\n");
        return -1;
    }
    if (get_poser_axis_line(config_file, "RY", &p.ry, &p.pos_rot_min[1],
                            &p.pos_rot_max[1])) {
        fprintf(stderr, "Can't read RY line for Poser Analog\n");
        return -1;
    }
    if (get_poser_axis_line(config_file, "RZ", &p.rz, &p.pos_rot_min[2],
                            &p.pos_rot_max[2])) {
        fprintf(stderr, "Can't read RZ line for Poser Analog\n");
        return -1;
    }

    _devices->add(new vrpn_Poser_Analog(s2, connection, &p, i1 != 0));
    if (p.x.ana_name != NULL) { delete [] p.x.ana_name; }
    if (p.y.ana_name != NULL) { delete [] p.y.ana_name; }
    if (p.z.ana_name != NULL) { delete [] p.z.ana_name; }
    if (p.rx.ana_name != NULL) { delete [] p.rx.ana_name; }
    if (p.ry.ana_name != NULL) { delete [] p.ry.ana_name; }
    if (p.rz.ana_name != NULL) { delete [] p.rz.ana_name; }

    return 0;
}

int vrpn_Generic_Server_Object::setup_nikon_controls(char *&pch, char *line,
                                                     FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];

    // Get the arguments
    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%511s", s2, s3) != 2) {
        fprintf(stderr, "Bad vrpn_nikon_controls line: %s\n", line);
        return -1;
    }

    // Open the server
    if (verbose) printf("Opening vrpn_nikon_control %s on port %s \n", s2, s3);
    _devices->add(new vrpn_Nikon_Controls(s2, connection, s3));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Poser_Tek4662(char *&pch, char *line,
                                                    FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int i1;

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%511s%d", s2, s3, &i1) != 3) {
        fprintf(stderr, "Bad vrpn_Poser_Tek4662 line: %s\n", line);
        return -1;
    }

    if (verbose) {
        printf("Opening vrpn_Poser_Tek4662 %s on port %s at baud %d\n", s2, s3,
               i1);
    }

    _devices->add(new vrpn_Poser_Tek4662(s2, connection, s3, i1));

    return 0;
}

// ----------------------------------------------------------------------
// BUW additions
// ----------------------------------------------------------------------

/******************************************************************************/
/* setup Atmel microcontroller */
/******************************************************************************/
int vrpn_Generic_Server_Object::setup_Atmel(char *&pch, char *line,
                                            FILE * /*config_file*/)
{
    char name[LINESIZE];
    char port[LINESIZE];
    int baud = 0;
    int channel_count = 0;

    VRPN_CONFIG_NEXT();

    // first line
    if (setup_vrpn_Atmel::channel_count == 0) {

        // Get the arguments
        if (sscanf(pch, "%511s%511s%d%d", name, port, &baud, &channel_count) !=
            4) {
            fprintf(stderr, "Bad vrpn_Atmel line: %s\n", line);
            return -1;
        }
        else {
            if (verbose) {
                printf("name: %s\n", name);
                printf("port: %s\n", port);
                printf("baud: %d\n", baud);
                printf("channel_count: %d\n", channel_count);
            }
        }

        setup_vrpn_Atmel::channel_count = channel_count;

        // init the channel_mode array
        for (int k = 0; k < channel_count; ++k) {
            setup_vrpn_Atmel::channel_mode[k] = VRPN_ATMEL_MODE_NA;
        }

        return 0;

    } // end of handline first line

    //*********************************************************

    //*********************************************************

    int channel;
    char mode[3];

    // Get the arguments
    if (sscanf(pch, "%d%2s", &channel, mode) != 2) {
        fprintf(stderr, "Bad vrpn_Atmel line: %s\n", line);
        return -1;
    }
    else {
        if (verbose) {
            printf("channel: %d - mode: %s\n", channel, mode);
        }
    }

    // check if it is a valid channel
    if (channel >= setup_vrpn_Atmel::channel_count) {
        fprintf(stderr, "channel value out of range\n\n");
        return -1;
    }

    //**************************************************
    // last line of vrpn_Atmel
    if (channel == -1) {

#ifndef _WIN32
        // here we use a factory interface because a lot of init things have to
        // be done
        _devices->add(vrpn_Atmel::Create(name, connection, port, baud,
                                         channel_count,
                                         setup_vrpn_Atmel::channel_mode));

        // reset the params so that another atmel can be configured
        setup_vrpn_Atmel::channel_count = 0;

        printf("\nAtmel %s started.\n\n", name);

        // the Analog_Output is handled implict by analog like done in Zaber

        return 0;
#else
        fprintf(stderr, "vrpn_Generic_Server_Object::setup_Atmel(): Not "
                        "implemented on this architecture\n");
        return -1;
#endif
    }

    // check if it is a valid channel
    if (channel < 0) {
        fprintf(stderr, "channel value out of range\n\n");
        return -1;
    }

    // channel init line

    //**************************************************
    // set the mode array
    int mode_int;

#define VRPN_ATMEL_IS_MODE(s) !strcmp(pch = strtok(mode, " \t"), s)

    // convert the char * in an integer
    if (VRPN_ATMEL_IS_MODE("RW")) {
        mode_int = VRPN_ATMEL_MODE_RW;
    }
    else if (VRPN_ATMEL_IS_MODE("RO")) {
        mode_int = VRPN_ATMEL_MODE_RO;
    }
    else if (VRPN_ATMEL_IS_MODE("WO")) {
        mode_int = VRPN_ATMEL_MODE_WO;
    }
    else if (VRPN_ATMEL_IS_MODE("NA")) {
        mode_int = VRPN_ATMEL_MODE_NA;
    }
    else {
        fprintf(stderr, "unknown io-mode: %s\n\n", mode);
        return -1;
    }

    // write it to the array
    setup_vrpn_Atmel::channel_mode[channel] = mode_int;

#undef VRPN_ATMEL_IS_MODE

    return 0;
}

/******************************************************************************/
/* setup mouse connected via event interface */
/******************************************************************************/
int vrpn_Generic_Server_Object::setup_Event_Mouse(char *&pch, char *line,
                                                  FILE * /*config_file*/)
{

    char name[LINESIZE], port[LINESIZE];

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, button_name, port)
    if (sscanf(pch, "%511s%511s", name, port) != 2) {
        fprintf(stderr, "Bad vrpn_Event_Mouse line: %s\n", line);
        return -1;
    }

    // Open the button
    if (verbose) {
        printf("Opening vrpn_Event_Mouse: %s on port %s\n", name, port);
    }

    _devices->add(new vrpn_Event_Mouse(name, connection, port));

    return 0;
}

/*
 *  inertiamouse config file setup routine
 *
 */
int vrpn_Generic_Server_Object::setup_inertiamouse(char *&pch, char *line,
                                                   FILE * /*config_file*/)
{
    char name[LINESIZE], port[LINESIZE];
    int baud;
    int ret;

    VRPN_CONFIG_NEXT();

    // Get the arguments (class, magellan_name, port, baud
    if ((ret = sscanf(pch, "%511s%511s%d", name, port, &baud)) < 3) {
        fprintf(stderr, "Bad vrpn_inertiamouse line: %s\n", line);
        return -1;
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_inertiamouse: %s on port %s, baud %d\n", name,
               port, baud);
    }
    _devices->add(vrpn_inertiamouse::create(name, connection, port, baud));

    return 0;
}

// ----------------------------------------------------------------------

int vrpn_Generic_Server_Object::setup_Analog_USDigital_A2(
    char *&pch, char *line, FILE * /*config_file*/)
{
    char A2name[LINESIZE];
    int comPort, numChannels, numArgs, reportChange;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, USD_A2_name, comPort, numChannels,
    // [reportChange]
    numArgs = sscanf(pch, "%511s%d%d%d", A2name, &comPort, &numChannels,
                     &reportChange);
    if (numArgs != 3 && numArgs != 4) {
        fprintf(stderr, "Bad vrpn_Analog_USDigital_A2 line: %s\n", line);
        return -1;
    }

    // Handle optional parameter
    if (numArgs == 3) {
        reportChange = 0;
    }

    // Make sure the parameters look OK
    if (comPort < 0 || comPort > 100) {
        fprintf(stderr,
                "Invalid COM port %d in vrpn_Analog_USDigital_A2 line: %s\n",
                comPort, line);
        return -1;
    }
    if (numChannels < 1 ||
        (vrpn_uint32)numChannels >
            vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_CHANNEL_MAX ||
        numChannels > vrpn_CHANNEL_MAX) {
        fprintf(stderr, "Invalid number of channels %d in "
                        "vrpn_Analog_USDigital_A2 line: %s\n",
                numChannels, line);
        return -1;
    }

#ifdef VRPN_USE_USDIGITAL
    // Open the device
    if (verbose)
        printf("Opening vrpn_Analog_USDigital_A2: %s on port %d (%u=search for "
               "port), with %d channels, reporting %s\n",
               A2name, comPort,
               vrpn_Analog_USDigital_A2::vrpn_Analog_USDigital_A2_FIND_PORT,
               numChannels, (reportChange == 0) ? "always" : "on change");
    _devices->add(new vrpn_Analog_USDigital_A2(
        A2name, connection, (vrpn_uint32)comPort, (vrpn_uint32)numChannels,
        (reportChange != 0)));

    return 0;
#else
    printf("Warning:  Server not compiled with  VRPN_USE_USDIGITAL defined.\n");
    return -1;
#endif
} //  setup_USDigital_A2

int vrpn_Generic_Server_Object::setup_Button_NI_DIO24(char *&pch, char *line,
                                                      FILE * /*config_file*/)
{
    char DIO24name[LINESIZE];
    int numChannels;
    int numArgs;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, D24_name, numChannels)
    numArgs = sscanf(pch, "%511s%d", DIO24name, &numChannels);
    if (numArgs != 1 && numArgs != 2) {
        fprintf(stderr, "Bad vrpn_Button_NI_DIO24 line: %s\n", line);
        return -1;
    }

    //  Do error checking on numChannels
    if (numArgs > 1 &&
        (numChannels < 1 ||
         numChannels > vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24_CHANNEL_MAX ||
         numChannels > vrpn_CHANNEL_MAX)) {
        fprintf(
            stderr,
            "Invalid number of channels %d in vrpn_Button_NI_DIO24 line: %s\n",
            numChannels, line);
        return -1;
    }

    //  if numChannels is wrong, use default
    if (numArgs < 2 || numChannels < 1) {
        numChannels = vrpn_Button_NI_DIO24::vrpn_Button_NI_DIO24_CHANNEL_MAX;
    }

#ifdef VRPN_USE_NATIONAL_INSTRUMENTS_MX
    // Open the device
    if (verbose)
        printf("Opening vrpn_Button_NI_DIO24: %s with up to %d buttons\n",
               DIO24name, numChannels);

    _devices->add(new vrpn_Button_NI_DIO24(DIO24name, connection, numChannels));

    return 0;
#else
    printf("Warning:  Server not compiled with "
           "VRPN_USE_NATIONAL_INSTRUMENTS_MX defined.\n");
    return -1;
#endif

} //  setup_Button_NI_DIO24

int vrpn_Generic_Server_Object::setup_Tracker_OSVRHackerDevKit(char *&pch, char
                                                               *line, FILE
                                                               *)
{
    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    int ret = sscanf(pch, "%511s", s2);
    if (ret != 1) {
        fprintf(stderr, "Bad OSVR Hacker Dev Kit line: %s\n", line);
        return -1;
    }

    // Open the OSVR Hacker Dev Kit
    if (verbose) {
        printf("Opening vrpn_Tracker_OSVRHackerDevKit\n");
    }

#ifdef VRPN_USE_HID
    // Open the tracker
    _devices->add(new vrpn_Tracker_OSVRHackerDevKit(s2, NULL, connection));
#else
    fprintf(stderr,
            "OSVRHackerDevKit driver works only with VRPN_USE_HID defined!\n");
#endif
    return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Tracker_PhaseSpace(char *&pch, char *line,
                                                         FILE *config_file)
{
#ifdef VRPN_INCLUDE_PHASESPACE

  char trackerName[LINESIZE];
  char device[LINESIZE];
  float framerate = 0;
  int readflag = 0;
  int slaveflag = 0;

  VRPN_CONFIG_NEXT();

  vrpn_Tracker_PhaseSpace* pstracker = NULL;

  if (sscanf(line, "vrpn_Tracker_PhaseSpace %s", trackerName) == 1) {
    pstracker = new vrpn_Tracker_PhaseSpace (trackerName, connection);
  } else {
    fprintf (stderr, "Bad vrpn_Tracker_PhaseSpace line: %s\nProper format is:  vrpn_Tracker_Phasespace trackerName\n", line);
    return -1;
  }

  // parse config file
  printf("Parsing config file...\n\n");
  if(!pstracker->load(config_file)) {
    fprintf(stderr, "Error parsing config file.\n");
    delete pstracker;
    return -1;
  }

  if(!pstracker->InitOWL()) {
    fprintf(stderr, "owl init error.\n");
    delete pstracker;
    return -1;
  }

  // start streaming data
  if (!pstracker->enableStreaming(true)) {
    delete pstracker;
    return -1;
  }

  _devices->add(pstracker);
  return 0;

#else
  fprintf (stderr, "vrpn_server: Can't open PhaseSpace OWL server: VRPN_INCLUDE_PHASESPACE not defined in vrpn_Configure.h!\n");
  return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_RazerHydra(char *&pch, char *line,
                                                         FILE *)
{
    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    int ret = sscanf(pch, "%511s", s2);
    if (ret != 1) {
        fprintf(stderr, "Bad Razer Hydra line: %s\n", line);
        return -1;
    }

    // Open the Razer Hydra
    if (verbose) {
        printf("Opening vrpn_Tracker_RazerHydra\n");
    }

#ifdef VRPN_USE_HID
    // Open the tracker
    _devices->add(new vrpn_Tracker_RazerHydra(s2, connection));
#else
    fprintf(stderr,
            "RazerHydra driver works only with VRPN_USE_HID defined!\n");
#endif
    return 0; // successful completion
}

//================================
int vrpn_Generic_Server_Object::setup_Tracker_ThalmicLabsMyo(char * &pch, char * line, FILE *)
{
  char s2[LINESIZE];
  int useLockI;
  int armSide;
  
  VRPN_CONFIG_NEXT();
  int ret = sscanf (pch, "%511s %d %d", s2,&useLockI, &armSide);
  if (ret != 3) {
      fprintf(stderr, "Bad ThalmicLabsMyo line: %s\n", line);
      return -1;
  }
  bool useLock = useLockI > 0;
#ifdef VRPN_INCLUDE_THALMICLABSMYO
  // Open the Myo
  if (verbose) {
      printf ("Opening vrpn_Tracker_ThalmicLabsMyo %s, use lock is %n", s2,useLock);
  }
  // Open the tracker
  _devices->add(new vrpn_Tracker_ThalmicLabsMyo(s2, connection,useLock,(vrpn_Tracker_ThalmicLabsMyo::ARMSIDE)armSide));
#else
  fprintf(stderr,
          "ThalmicLabsMyo driver works only with VRPN_INCLUDE_THALMICLABSMYO defined!\n");
  useLock = !useLock;	// Unused parameter, removing warning.
#endif
  return 0;  // successful completion
}

int vrpn_Generic_Server_Object::setup_Tracker_NDI_Polaris(char *&,
                                                          char *line,
                                                          FILE *config_file)
{
    char trackerName[LINESIZE];
    char device[LINESIZE];
    int numRigidBodies;
    char *rigidBodyFileNames[VRPN_GSO_MAX_NDI_POLARIS_RIGIDBODIES];

    // get tracker name and device
    if (sscanf(line, "vrpn_Tracker_NDI_Polaris %s %s %d", trackerName, device,
               &numRigidBodies) < 3) {
        fprintf(stderr, "Bad vrpn_Tracker_NDI_Polaris line: %s\n", line);
        return -1;
    }
    printf("DEBUG Tracker_NDI_Polaris: num of rigidbodies %d\n",
           numRigidBodies);

    // parse the filename for each rigid body
    int rbNum;
    for (rbNum = 0; rbNum < numRigidBodies; rbNum++) {
        if (fgets(line, LINESIZE, config_file) ==
            NULL) { // advance to next line of config file
            perror("NDI_Polaris RigidBody can't read line!");
            return -1;
        }
        rigidBodyFileNames[rbNum] =
            new char[LINESIZE]; // allocate string for filename
        if (sscanf(line, "%s", rigidBodyFileNames[rbNum]) != 1) {
            fprintf(stderr, "Tracker_NDI_Polaris: error reading .rom filename "
                            "#%d from config file in line: %s\n",
                    rbNum, line);
            return -1;
        }
        else {
            printf("DEBUG Tracker_NDI_Polaris: filename >%s<\n",
                   rigidBodyFileNames[rbNum]);
        }
    }

    _devices->add(new vrpn_Tracker_NDI_Polaris(
        trackerName, connection, device, numRigidBodies,
        (const char **)rigidBodyFileNames));

    // free the .rom filename strings
    for (rbNum = 0; rbNum < numRigidBodies; rbNum++) {
        delete [] (rigidBodyFileNames[rbNum]);
    }
    return (0); // success
}

int vrpn_Generic_Server_Object::setup_Logger(char *&pch, char *line,
                                             FILE * /*config_file*/)
{

    char s2[LINESIZE], s3[LINESIZE];

    // Line will be: vrpn_Auxiliary_Logger_Server_Generic NAME CONNECTION_TO_LOG
    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s %511s", s2, s3) != 2) {
        fprintf(stderr, "Bad vrpn_Auxiliary_Logger_Server_Generic line: %s\n",
                line);
        return -1;
    }

    // Open the logger
    if (verbose) {
        printf("Opening vrpn_Auxiliary_Logger_Server_Generic %s\n", s2);
    }
    _devices->add(new vrpn_Auxiliary_Logger_Server_Generic(s2, s3, connection));

    return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_ImageStream(char *&pch, char *line,
                                                  FILE * /*config_file*/)
{

    char s2[LINESIZE], s3[LINESIZE];

    // Line will be: vrpn_Imager_Stream_Buffer NAME IMAGER_SERVER_TO_LOG
    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s %511s", s2, s3) != 2) {
        fprintf(stderr, "Bad vrpn_Imager_Stream_Buffer line: %s\n", line);
        return -1;
    }

    // Open the stream buffer
    if (verbose) {
        printf("Opening vrpn_Imager_Stream_Buffer %s\n", s2);
    }
    _devices->add(new vrpn_Imager_Stream_Buffer(s2, s3, connection));

    return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_WiiMote(char *&pch, char *line,
                                              FILE * /*config_file*/)
{
    char sBDADDR[LINESIZE];
    char s2[LINESIZE];
    unsigned controller, useMS, useIR, reorderBtns;

    VRPN_CONFIG_NEXT();
    // Get the arguments (wiimote_name, controller index)
    int numParms = sscanf(pch, "%511s%u %u %u %u %511s", s2, &controller,
                          &useMS, &useIR, &reorderBtns, sBDADDR);
    if (numParms < 5) {
        fprintf(stderr, "Bad vrpn_WiiMote line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_WIIUSE
    // Open the WiiMote
    if (verbose) {
        printf("Opening vrpn_WiiMote: %s\n", s2);
    }
    if (numParms == 5) {
        _devices->add(new vrpn_WiiMote(s2, connection, controller, useMS, useIR,
                                       reorderBtns));
    }
    else {
        _devices->add(new vrpn_WiiMote(s2, connection, controller, useMS, useIR,
                                       reorderBtns, sBDADDR));
    }

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open WiiMote: VRPN_USE_WIIUSE not "
                    "defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_WiimoteHead(
    char *&pch, char *line, FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    float f1, f2;
    int numparms;

    VRPN_CONFIG_NEXT();

    // Get the arguments (tracker_name, wiimote_name, min_update_rate,
    // led_distance)
    if ((numparms = sscanf(pch, "%511s%511s%f%f", s2, s3, &f1, &f2)) < 2) {
        fprintf(stderr,
                "Bad vrpn_Tracker_WiimoteHead line: %s\n%s %s %s %f %f\n", line,
                pch, s2, s3, f1, f2);
        return -1;
    }

    // set LED distance to .205, if not set
    if (numparms < 4) {
        f2 = .205f;
    }
    // set min update rate to 60, if not set
    if (numparms < 3) {
        f1 = 60.0f;
    }

    // Open the tracker
    if (verbose) {
        printf("Opening vrpn_Tracker_WiimoteHead: %s wiimote: %s, updaterate: "
               "%f, leddistance: %f\n",
               s2, s3, f1, f2);
    }

    _devices->add(new vrpn_Tracker_WiimoteHead(s2, connection, s3, f1, f2));
    return 0;
}

int vrpn_Generic_Server_Object::setup_Freespace(char *&pch, char *line,
                                                FILE * /*config_file*/)
{
    char s2[LINESIZE];
    unsigned controller, sendbody, senduser;

    VRPN_CONFIG_NEXT();
    // Get the arguments (wiimote_name, controller index)
    if (sscanf(pch, "%511s%u%u%u", s2, &controller, &sendbody, &senduser) !=
        4) {
        fprintf(stderr, "Bad vrpn_Freespace line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_FREESPACE
    // Open the Freespace if we can.
    try {
        _devices->add(vrpn_Freespace::create(s2, connection, controller,
                                             (sendbody != 0), (senduser != 0)));
    }
    catch (vrpn_MainloopObject::CannotWrapNullPointerIntoMainloopObject &) {
        fprintf(stderr, "vrpn_server: Can't open Freespace: driver could not "
                        "connect as configured!\n");
        return -1;
    }

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open Freespace: VRPN_USE_FREESPACE not "
                    "defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_SpacePoint(char *&pch, char *line,
                                                 FILE * /*config_file*/)
{

    char s2[LINESIZE];
    int idx = 0;

    VRPN_CONFIG_NEXT();

    if (sscanf(pch, "%511s%d", s2, &idx) < 1) {
        fprintf(stderr, "Bad SpacePoint line: %s\n", line);
        return -1;
    }


// Open the SpacePoint

#ifdef VRPN_USE_HID
    // Open the tracker
    if (verbose) {
        printf("Opening vrpn_Tracker_SpacePoint #%d %s\n", idx, s2);
    }

    _devices->add(new vrpn_Tracker_SpacePoint(s2, connection, idx));
#else
    fprintf(stderr,
            "SpacePoint driver works only with VRPN_USE_HID defined!\n");
#endif

    return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Wintracker(char *&pch, char *line,
                                                 FILE * /*config_file*/)
{

    char name[LINESIZE];
    char s0[LINESIZE], s1[LINESIZE], s2[LINESIZE];
    char ext[LINESIZE];
    char hemi[LINESIZE];

    VRPN_CONFIG_NEXT();

    if (sscanf(pch, "%511s%511s%511s%511s%511s%511s", name, s0, s1, s2, ext,
               hemi) != 6) {
        fprintf(stderr, "Bad Wintracker line: %s\n", line);
        fprintf(stderr, "NAME: %s\n", name);
        return -1;
    }

// Open the Wintracker

#ifdef VRPN_USE_HID
    // Open the tracker
    if (verbose) {
        printf("Parameters ->  name:%c, s0: %c, s1:  %c, s2: %c,ext: %c, hemi: "
               "%c\n",
               name[0], s0[0], s1[0], s2[0], ext[0], hemi[0]);
        printf("Opening vrpn_Tracker_Wintracker %s\n", name);
    }

    _devices->add(new vrpn_Tracker_Wintracker(name, connection, s0[0], s1[0],
                                              s2[0], ext[0], hemi[0]));
#else
    fprintf(stderr,
            "Wintracker driver works only with VRPN_USE_HID defined!\n");
#endif

    return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Tracker_GameTrak(char *pch, char *line,
                                                       FILE *config_file)
{
    char s2[LINESIZE];
    char s3[LINESIZE];

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%511s", s2, s3) != 2) {
        fprintf(stderr, "Bad GameTrak line: %s\n", line);
        return -1;
    }

    // read axis mapping line if present
    if (fgets(line, LINESIZE, config_file) == NULL) {
        perror("GameTrak Can't read line!");
        return -1;
    }

    // if it is an empty line, finish parsing
    int mapping[] = {0, 1, 2, 3, 4, 5};
    if (line[0] != '\n') {
        // get the first token
        char tok[LINESIZE];
        sscanf(line, "%s", tok);

        if (strcmp(tok, "axis_mapping") == 0) {
            sscanf(line, "%s %d %d %d %d %d %d", tok, &mapping[0], &mapping[1],
                   &mapping[2], &mapping[3], &mapping[4], &mapping[5]);
        }
        else {
            fprintf(
                stderr,
                "Incorrect GameTrak line %s (did you forget an empty line?)\n",
                line);
            return -1;
        }
    }

    // Open the GameTrak

    // Open the tracker
    if (verbose) {
        printf("Opening vrpn_Tracker_GameTrak %s by using joystick device %s\n",
               s2, s3);
        printf("GameTrak axis mapping: %d %d %d %d %d %d\n", mapping[0],
               mapping[1], mapping[2], mapping[3], mapping[4], mapping[5]);
    }

    _devices->add(new vrpn_Tracker_GameTrak(s2, connection, s3, mapping));

    return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Tracker_MotionNode(char *&pch, char *line,
                                                         FILE * /*config_file*/)
{
    char name[LINESIZE];
    unsigned num_sensors = 0;
    char address[LINESIZE];
    unsigned port = 0;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, sensors, rate)
    if (4 !=
        sscanf(pch, "%511s%u%511s%u", name, &num_sensors, address, &port)) {
        fprintf(stderr, "Bad vrpn_Tracker_MotionNode line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_MOTIONNODE
    // Open the tracker
    if (verbose) {
        printf("Opening vrpn_Tracker_MotionNode: %s with %u sensors, address "
               "%s, port %u\n",
               name, num_sensors, address, port);
    }

    _devices->add(new vrpn_Tracker_MotionNode(name, connection, num_sensors,
                                              address, port));
    return 0;
#else
    fprintf(stderr, "vrpn_Tracker_MotionNode: Not compiled in (add "
                    "VRPN_USE_MOTIONNODE to vrpn_Configure.h and recompile)\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_GPS(char *&pch, char *line,
                                                  FILE * /*config_file*/)
{
    unsigned num_sensors = 0;
    char address[LINESIZE] = "/dev/tty.someserialport";

    char trackerName[512];
    int baud;
    int useUTM = 0;

    int argCount = 0;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name, sensors, rate)
    argCount = sscanf(pch, "%511s%511s%u", trackerName, address, &baud);
    // printf("tracker GPS values: %s, %s, %d\n", trackerName, address, baud);
    if (3 != argCount) {
        fprintf(stderr, "Bad vrpn_Tracker_GPS line:\n %s\n\targCount is %d\n",
                line, argCount);
        return -1;
    }

    // Open the tracker
    if (verbose) {
        printf("Opening vrpn_Tracker_GPS: %s with %u sensors, address %s, port "
               "%u\n",
               trackerName, num_sensors, address, baud);
    }

    //_devices->add(new vrpn_Tracker_GPS(name, connection, num_sensors, address,
    // port));
    _devices->add(new vrpn_Tracker_GPS(trackerName, connection, address, baud,
                                       useUTM, 0));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_TrivisioColibri(
    char *&pch, char *line, FILE * /*config_file*/)
{
    char s2[LINESIZE];
    int numSensors, Hz, bufLen;

    VRPN_CONFIG_NEXT();
    // Get the arguments
    if (sscanf(pch, "%511s%d%d%d", s2, &numSensors, &Hz, &bufLen) != 4) {
        fprintf(stderr, "Bad vrpn_Tracker_TrivisioColibri line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_TRIVISIOCOLIBRI
    // Open the Trivisio Colibri if we can.
    if (verbose) {
        printf("Opening vrpn_Tracker_TrivisioColibri: %s with %d sensors, %d "
               "Hz, and %d bufLen",
               s2, numSensors, Hz, bufLen);
    }

    _devices->add(new vrpn_Tracker_TrivisioColibri(s2, connection, numSensors,
                                                   Hz, bufLen));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open vrpn_Tracker_TrivisioColibri: "
                    "VRPN_USE_TRIVISIOCOLIBRI not defined in "
                    "vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_Colibri (char * & pch, char * line, FILE * /*config_file*/)
{
  char s2 [LINESIZE];
  char s3 [LINESIZE];
  int Hz;
  int report_a_w;

  VRPN_CONFIG_NEXT();
  // Get the arguments
  if (sscanf (pch, "%511s%511s%d%d", s2, s3, &Hz, &report_a_w) != 4) {
    fprintf (stderr, "Bad vrpn_Tracker_Colibri line: %s\n", line);
    return -1;
  }

#ifdef VRPN_USE_COLIBRIAPI
  // Open the Trivisio Colibri if we can.
  if (verbose) {
    printf ("Opening vrpn_Tracker_Colibri: %s with %d Hz", s2, Hz);
  }

  _devices->add(new vrpn_Tracker_Colibri(s2, connection,
                s3[0] == '*' ? NULL : s3, Hz, report_a_w));

  return 0;
#else
  fprintf (stderr, "vrpn_server: Can't open vrpn_Tracker_Colibri: VRPN_USE_COLIBRIAPI not defined in vrpn_Configure.h!\n");
  return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_LUDL_USBMAC6000(char *&pch, char *line,
                                                      FILE * /*config_file*/)
{
    char s2[LINESIZE];
    int recenter;

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%i", s2, &recenter) != 2) {
        fprintf(stderr, "Bad LUDL_USBMAC6000 line: %s\n", line);
        return -1;
    }

#if defined(VRPN_USE_LIBUSB_1_0)

    // Open the LUDL_USBMAC6000

    // Open the button
    if (verbose) {
        printf("Opening vrpn_LUDL_USBMAC6000 as device %s\n", s2);
    }
    _devices->add(new vrpn_LUDL_USBMAC6000(s2, connection, recenter != 0));

    return 0; // successful completion

#else
    fprintf(stderr,
            "vrpn_LUDL_USBMAC6000 requires VRPN built with LIBUSB_1_0.\n");
    return -1;
#endif
}

template <class T>
int setup_Analog_5dtUSB(const char *specialization, bool verbose,
                        vrpn_Connection *connection,
                        vrpn_MainloopContainer *_devices, char *&pch,
                        char *line)
{
    char s2[LINESIZE];
    // Get the arguments
    if (sscanf(pch, "%511s", s2) != 1) {
        fprintf(stderr, "Bad vrpn_Analog_5dtUSB_%s line: %s\n", specialization,
                line);
        return -1;
    }
#if defined(VRPN_USE_HID)

    // Open the device
    if (verbose) {
        printf("Opening vrpn_Analog_5dtUSB_%s as device %s\n", specialization,
               s2);
    }
    _devices->add(new T(s2, connection));
    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open vrpn_Analog_5dtUSB_%s: "
                    "VRPN_USE_HID not defined in vrpn_Configure.h!\n",
            specialization);
    return -1;
#endif
}

#if !defined(VRPN_USE_HID)
// Have to make these types exist for the sake of simplifying the calls below.
typedef int vrpn_Analog_5dtUSB_Glove5Left;
typedef int vrpn_Analog_5dtUSB_Glove5Right;
typedef int vrpn_Analog_5dtUSB_Glove14Left;
typedef int vrpn_Analog_5dtUSB_Glove14Right;
#endif
int vrpn_Generic_Server_Object::setup_Analog_5dtUSB_Glove5Left(
    char *&pch, char *line, FILE * /*config_file*/)
{
    VRPN_CONFIG_NEXT();
    return setup_Analog_5dtUSB<vrpn_Analog_5dtUSB_Glove5Left>(
        "Glove5Left", verbose, connection, _devices, pch, line);
}

int vrpn_Generic_Server_Object::setup_Analog_5dtUSB_Glove5Right(
    char *&pch, char *line, FILE * /*config_file*/)
{
    VRPN_CONFIG_NEXT();
    return setup_Analog_5dtUSB<vrpn_Analog_5dtUSB_Glove5Right>(
        "Glove5Right", verbose, connection, _devices, pch, line);
}

int vrpn_Generic_Server_Object::setup_Analog_5dtUSB_Glove14Left(
    char *&pch, char *line, FILE * /*config_file*/)
{
    VRPN_CONFIG_NEXT();
    return setup_Analog_5dtUSB<vrpn_Analog_5dtUSB_Glove14Left>(
        "Glove14Left", verbose, connection, _devices, pch, line);
}

int vrpn_Generic_Server_Object::setup_Analog_5dtUSB_Glove14Right(
    char *&pch, char *line, FILE * /*config_file*/)
{
    VRPN_CONFIG_NEXT();
    return setup_Analog_5dtUSB<vrpn_Analog_5dtUSB_Glove14Right>(
        "Glove14Right", verbose, connection, _devices, pch, line);
}

int vrpn_Generic_Server_Object::setup_Tracker_FilterOneEuro(
    char *&pch, char *line, FILE * /*config_file*/)
{
    char s2[LINESIZE], s3[LINESIZE];
    int sensors;
    double vecMinCutoff, vecBeta, vecDerivativeCutoff;
    double quatMinCutoff, quatBeta, quatDerivativeCutoff;

    VRPN_CONFIG_NEXT();
    if (sscanf(pch, "%511s%511s%d%lf%lf%lf%lf%lf%lf", s2, s3, &sensors,
               &vecMinCutoff, &vecBeta, &vecDerivativeCutoff, &quatMinCutoff,
               &quatBeta, &quatDerivativeCutoff) != 9) {
        fprintf(stderr, "Bad FilterOneEuro line: %s\n", line);
        return -1;
    }

    // Open the Filter
    if (verbose) {
        printf("Opening vrpn_Tracker_FilterOneEuro as device %s\n", s2);
    }
    _devices->add(new vrpn_Tracker_FilterOneEuro(
        s2, connection, s3, sensors, vecMinCutoff, vecBeta, vecDerivativeCutoff,
        quatMinCutoff, quatBeta, quatDerivativeCutoff));

    return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Tracker_zSight(char *&pch, char *line,
                                                     FILE * /*config_file*/)
{
    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    // Get the arguments
    if (sscanf(pch, "%511s", s2) != 1) {
        fprintf(stderr, "Bad vrpn_Tracker_zSight line: %s\n", line);
        return -1;
    }

#if defined(_WIN32) && defined(VRPN_USE_DIRECTINPUT) &&                        \
    defined(VRPN_HAVE_ATLBASE)
    // Open the zSight if we can.
    if (verbose) {
        printf("Opening vrpn_Tracker_zSight: %s", s2);
    }

    _devices->add(new vrpn_Tracker_zSight(s2, connection));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open vrpn_Tracker_zSight: "
                    "VRPN_USE_DIRECTINPUT and/or VRPN_HAVE_ATLBASE not defined "
                    "in vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_ViewPoint(char *&pch, char *line,
                                                        FILE * /*config_file*/)
{
    char s2[LINESIZE]; // Get the arguments
    int smoothedData;

    VRPN_CONFIG_NEXT();
    // Get the arguments
    if (sscanf(pch, "%511s%d", s2, &smoothedData) != 2) {
        fprintf(stderr, "Bad vrpn_Tracker_ViewPoint line: %s\n", line);
        return -1;
    }

#ifdef VRPN_USE_VIEWPOINT
    // Open the ViewPoint EyeTracker if we can.
    if (verbose) {
        printf("Opening vrpn_Tracker_ViewPoint: %s", s2);
    }

    bool smooth = smoothedData == 1;
    _devices->add(new vrpn_Tracker_ViewPoint(s2, connection, smooth));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open vrpn_Tracker_ViewPoint: "
                    "VRPN_USE_VIEWPOINT not defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_G4(char *&pch, char *line,
                                                 FILE *config_file)
{
    char name[LINESIZE], filepath[LINESIZE];
    int numparms;
    int Hz = 120;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, tracker_name)
    numparms = sscanf(pch, "%511s%d", name, &Hz);
    if (numparms == 0) {
        return -1;
    }

    if (name[strlen(name) - 1] == '\\') {
        name[strlen(name) - 1] = '\0';
    }

    printf("\n%s Connecting with .g4c file located at:\n", name);

    // get the filepath to the .g4c file
    if (fgets(line, LINESIZE, config_file) == NULL) {
        fprintf(stderr, "Ran past end of config file in G4 description\n");
        return -1;
    }

    filepath[0] = 0;
    strncat(filepath, line, LINESIZE - 1);

    if (filepath[strlen(filepath) - 2] == '\\') {
        filepath[strlen(filepath) - 2] = '\0';
    }
    else {
        filepath[strlen(filepath) - 1] = '\0';
    }

    printf("%s\n", filepath);

#ifdef VRPN_USE_PDI
    char rcmd[5000];
    vrpn_Tracker_G4_HubMap *pHMap = NULL;
    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // G4 at reset time. So long as we find lines with slashes
    // at the ends, we add them to the command string to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    rcmd[0] = 0;
    while (line[strlen(line) - 2] == '\\') {
        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(stderr, "Ran past end of config file in G4 description\n");
            return -1;
        }

        // G4DigIO name hubID #buttons
        if (strncmp(line, "G4DigIO", strlen("G4DigIO")) == 0) {
            int nHub = 0;
            int nButtons = 0;
            char DigIOName[VRPN_G4_HUB_NAME_SIZE];

            if (3 != sscanf(line, "G4DigIO %64s %d %d", DigIOName, &nHub,
                            &nButtons)) {
                fprintf(stderr, "Invalid G4DigIO argument list: %s\n", line);
            }
            else {
                printf("\nCreating G4DigIO %s on Hub %d with inputs 0-%d\n",
                       DigIOName, nHub, nButtons - 1);

                if (pHMap == NULL) {
                    pHMap = new vrpn_Tracker_G4_HubMap();
                }

                pHMap->Add(nHub);
                pHMap->ButtonInfo(nHub, DigIOName, nButtons);
            }
        }
        else if (strncmp(line, "G4PowerTrak", strlen("G4PowerTrak")) == 0) {
            int nHub = 0;
            char PowerTrakName[VRPN_G4_HUB_NAME_SIZE];

            if (2 !=
                sscanf(line, "G4PowerTrak %64s %d", PowerTrakName, &nHub)) {
                fprintf(stderr, "Invalid G4PowerTrak argument list: %s\n",
                        line);
            }
            else {
                printf(
                    "\nCreating G4PowerTrak %s on Hub %d with buttons 0-%d\n",
                    PowerTrakName, nHub, VRPN_G4_POWERTRAK_BUTTON_COUNT - 1);

                if (pHMap == NULL) {
                    pHMap = new vrpn_Tracker_G4_HubMap();
                }

                pHMap->Add(nHub);
                pHMap->ButtonInfo(nHub, PowerTrakName,
                                  VRPN_G4_POWERTRAK_BUTTON_COUNT);
            }
        }
        else {
            // Copy the line into the remote command,
            // then replace \ with \0
            strncat(rcmd, line, LINESIZE);
        }
    }

    if (strlen(rcmd) > 0) {
        printf("Additional reset commands found\n");
    }

    _devices->add(
        new vrpn_Tracker_G4(name, connection, filepath, Hz, rcmd, pHMap));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open vrpn_Tracker_G4: VRPN_USE_PDI not "
                    "defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_FastrakPDI(char *&pch, char *line,
                                                         FILE *config_file)
{
    char name[LINESIZE];
    int Hz = 120;
    char rcmd[5000]; // reset commands to send to Liberty
    unsigned int nStylusMap = 0;
    VRPN_CONFIG_NEXT();
    // Get the arguments (class(already taken), tracker_name, reports per
    // second)
    sscanf(pch, "%511s%d", name, &Hz);

    // remove the '\' from the end of the name, if it has one
    if (name[strlen(name) - 1] == '\\') {
        name[strlen(name) - 1] = '\0';
    }

    printf("New FastrakPDI of name: %s\r\n", name);
    printf(" ...additional reset commands follow:\r\n");

    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // FastrakPDI at reset time. So long as we find lines with slashes
    // at the ends, we add them to the command string to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    rcmd[0] = 0;
    while (line[strlen(line) - 2] == '\\') {
        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(
                stderr,
                "Ran past end of config file in FastrakPDI description\r\n");
            return -1;
        }

        if (strncmp(line, "PDIStylus", strlen("PDIStylus")) == 0) {
            int nStylus = 0;
            sscanf(line, "PDIStylus %d", &nStylus);
            if (!(nStylus > 0)) {
                fprintf(stderr,
                        "PDIStylus command invalid station number: %s\r\n",
                        line);
                return -1;
            }
            else {
                printf("Creating PDIStylus button on station %d \r\n", nStylus);
                nStylusMap |= (1 << (nStylus - 1));
            }
        }
        // Copy the line into rcmd if it is not a comment, or the tracker name
        // line
        if (line[0] != '#' && (line[0] != 'v' && line[1] != 'r')) {
            strncat(rcmd, line, LINESIZE);
        }
    }

    if (rcmd[0] == 0) {
        printf(" no additional commands found\r\n");
    }

#ifdef VRPN_USE_PDI
    _devices->add(
        new vrpn_Tracker_FastrakPDI(name, connection, Hz, rcmd, nStylusMap));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open vrpn_Tracker_FastrakPDI: "
                    "VRPN_USE_PDI not defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_Tracker_LibertyPDI(char *&pch, char *line,
                                                         FILE *config_file)
{
    char name[LINESIZE];
    int Hz = 60;
    unsigned int nStylusMap = 0;
    char rcmd[5000]; // reset commands to send to Liberty

    VRPN_CONFIG_NEXT();
    // Get the arguments (class(already taken), tracker_name, reports per
    // second)
    sscanf(pch, "%511s%d", name, &Hz);

    // remove the '\' from the end of the name, if it has one
    if (name[strlen(name) - 1] == '\\') {
        name[strlen(name) - 1] = '\0';
    }

    printf("New LibertyPDI of name: %s\r\n", name);
    printf(" ...additional reset commands follow:\r\n");

    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // LibertyPDI at reset time. So long as we find lines with slashes
    // at the ends, we add them to the command string to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    rcmd[0] = 0;
    while (line[strlen(line) - 2] == '\\') {
        // Read the next line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(
                stderr,
                "Ran past end of config file in LibertyPDI description\r\n");
            return -1;
        }

        if (strncmp(line, "PDIStylus", strlen("PDIStylus")) == 0) {
            int nStylus = 0;
            sscanf(line, "PDIStylus %d", &nStylus);
            if (!(nStylus > 0)) {
                fprintf(stderr,
                        "PDIStylus command invalid station number: %s\r\n",
                        line);
                return -1;
            }
            else {
                printf("Creating PDIStylus button on station %d \r\n", nStylus);
                nStylusMap |= (1 << (nStylus - 1));
            }
        }
        // Copy the line into rcmd if it is not a comment, or the tracker name
        // line
        else if (line[0] != '#' && line[0] != 'v' && line[1] != 'r') {
            strncat(rcmd, line, LINESIZE);
        }
    }

    if (rcmd[0] == 0) {
        printf(" no additional commands found\r\n");
    }

#ifdef VRPN_USE_PDI
    _devices->add(
        new vrpn_Tracker_LibertyPDI(name, connection, Hz, rcmd, nStylusMap));

    return 0;
#else
    fprintf(stderr, "vrpn_server: Can't open vrpn_Tracker_LibertyPDI: "
                    "VRPN_USE_PDI not defined in vrpn_Configure.h!\n");
    return -1;
#endif
}

int vrpn_Generic_Server_Object::setup_YEI_3Space_Sensor(char *&pch, char *line,
                                                        FILE *config_file)
{
    char name[LINESIZE], device[LINESIZE];
    int baud_rate, calibrate_gyros, tare;
    double frames_per_second;
    float red_LED, green_LED, blue_LED;
    int LED_mode;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, name, port, baud, calibrate_gyros, tare,
    // frames_per_second
    if (sscanf(pch, "%511s%511s%d%d%d%lf%f%f%f%d", name, device, &baud_rate,
               &calibrate_gyros, &tare, &frames_per_second, &red_LED,
               &green_LED, &blue_LED, &LED_mode) != 10) {
        fprintf(stderr, "Bad vrpn_YEI_3Space_Sensor line: %s\n", line);
        return -1;
    }

    std::string port = device;
#ifdef _WIN32
    // Use the Win32 device namespace for COM ports, if they aren't already.
    // This is because devices with port numbers greater than 9 must have names
    // like \\.\COM14. This same name type can be used for lower-numbered ports
    // as well, so we go ahead and put it in no matter what.
    // Have to double the backslashes because they're escape characters.
    if (port.find('\\') == std::string::npos) {
        port = "\\\\.\\" + port;
    }
#endif

    // Allocate space to store pointers to reset commands.  Initialize
    // all of them to NULL pointers, indicating no commands.
    const int MAX_RESET_COMMANDS = 1024;
    int num_reset_commands = 0;
    const char *reset_commands[MAX_RESET_COMMANDS+1];
    for (int i = 0; i < MAX_RESET_COMMANDS; i++) {
      reset_commands[i] = NULL;
    }

    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // YEI at reset time. So long as we find lines with backslashes
    // at the ends, we add them to the command list to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    while (line[strlen(line) - 2] == '\\') {
        // Read the VRPN_CONFIG_NEXT line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(
                stderr,
                "Ran past end of config file in YEI description\n");
            return -1;
        }

        // Copy the first string from the line into a new character
        // array and then store this into the reset_commands list
        // if we haven't run out of room for them.
        if (num_reset_commands < MAX_RESET_COMMANDS) {
          char command[LINESIZE];
          sscanf(line, "%s", command);
          char *command_copy = new char[strlen(command)+1];
          if (command_copy == NULL) {
            fprintf(stderr, "Out of memory in YEI description\n");
            return -1;
          }
          strcpy(command_copy, command);
          reset_commands[num_reset_commands++] = command_copy;
        }
    }

    // Open the device
    if (verbose) {
        printf("Opening vrpn_YEI_3Space_Sensor: %s on port %s, baud %d\n", name,
               port.c_str(), baud_rate);
        if (num_reset_commands > 0) {
          printf("... additional reset commands follow:\n");
          for (int i = 0; i < num_reset_commands; i++) {
            printf("  %s\n", reset_commands[i]);
          }
        }
    }
    _devices->add(new vrpn_YEI_3Space_Sensor(
        name, connection, port.c_str(), baud_rate, calibrate_gyros != 0, tare != 0,
        frames_per_second, red_LED, green_LED, blue_LED, LED_mode,
        reset_commands));

    // Free up any additional-reset command data.
    for (int i = 0; i < MAX_RESET_COMMANDS; i++) {
      if (reset_commands[i] != NULL) {
        delete [] reset_commands[i];
        reset_commands[i] = NULL;
      }
    }

    return 0;
}

int vrpn_Generic_Server_Object::setup_YEI_3Space_Sensor_Wireless(char *&pch, char *line,
                                                        FILE *config_file)
{
    char name[LINESIZE], device[LINESIZE];
    int logical_id, baud_rate, calibrate_gyros, tare;
    double frames_per_second;
    float red_LED, green_LED, blue_LED;
    int LED_mode;

    VRPN_CONFIG_NEXT();
    // Get the arguments (class, name, port, baud, calibrate_gyros, tare,
    // frames_per_second
    if (sscanf(pch, "%511s%d%511s%d%d%d%lf%f%f%f%d", name, &logical_id,
               device, &baud_rate,
               &calibrate_gyros, &tare, &frames_per_second, &red_LED,
               &green_LED, &blue_LED, &LED_mode) != 11) {
        fprintf(stderr, "Bad setup_YEI_3Space_Sensor_Wireless line: %s\n", line);
        return -1;
    }

    std::string port = device;
#ifdef _WIN32
    // Use the Win32 device namespace for COM ports, if they aren't already.
    // This is because devices with port numbers greater than 9 must have names
    // like \\.\COM14. This same name type can be used for lower-numbered ports
    // as well, so we go ahead and put it in no matter what.
    // Have to double the backslashes because they're escape characters.
    if (port.find('\\') == std::string::npos) {
        port = "\\\\.\\" + port;
    }
#endif

    // Allocate space to store pointers to reset commands.  Initialize
    // all of them to NULL pointers, indicating no commands.
    const int MAX_RESET_COMMANDS = 1024;
    int num_reset_commands = 0;
    const char *reset_commands[MAX_RESET_COMMANDS+1];
    for (int i = 0; i < MAX_RESET_COMMANDS; i++) {
      reset_commands[i] = NULL;
    }
    
    // If the last character in the line is a backslash, '\', then
    // the following line is an additional command to send to the
    // YEI at reset time. So long as we find lines with backslashes
    // at the ends, we add them to the command list to send. Note
    // that there is a newline at the end of the line, following the
    // backslash.
    while (line[strlen(line) - 2] == '\\') {
        // Read the VRPN_CONFIG_NEXT line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            fprintf(
                stderr,
                "Ran past end of config file in YEI description\n");
            return -1;
        }

        // Copy the first string from the line into a new character
        // array and then store this into the reset_commands list
        // if we haven't run out of room for them.
        if (num_reset_commands < MAX_RESET_COMMANDS) {
          char command[LINESIZE];
          sscanf(line, "%s", command);
          char *command_copy = new char[strlen(command)+1];
          if (command_copy == NULL) {
            fprintf(stderr, "Out of memory in YEI description\n");
            return -1;
          }
          strcpy(command_copy, command);
          reset_commands[num_reset_commands++] = command_copy;
        }
    }

    // Open the device
    if (verbose) {
        printf("Opening setup_YEI_3Space_Sensor_Wireless: %s on port %s, baud %d\n", name,
            port.c_str(), baud_rate);
        if (num_reset_commands > 0) {
          printf("... additional reset commands follow:\n");
          for (int i = 0; i < num_reset_commands; i++) {
            printf("  %s\n", reset_commands[i]);
          }
        }
    }
    vrpn_YEI_3Space_Sensor_Wireless *dev = new vrpn_YEI_3Space_Sensor_Wireless(
        name, connection,
        logical_id, port.c_str(), baud_rate, calibrate_gyros != 0, tare != 0,
        frames_per_second, red_LED, green_LED, blue_LED, LED_mode,
        reset_commands);
    _devices->add(dev);

    // Free up any additional-reset command data.
    for (int i = 0; i < MAX_RESET_COMMANDS; i++) {
      if (reset_commands[i] != NULL) {
        delete [] reset_commands[i];
        reset_commands[i] = NULL;
      }
    }

    // If the last character in the line is a slash, '/', then
    // the following line is an additional sensor description. Note
    // that there is a newline at the end of the line, following the
    // backslash.  Re-parse the whole thing, using the second-sensor
    // description format and the serial port descriptor from the first
    // device.
    int serial_fd = dev->get_serial_file_descriptor();
    while (line[strlen(line) - 2] == '/') {
      // Read the VRPN_CONFIG_NEXT line
      if (fgets(line, LINESIZE, config_file) == NULL) {
          fprintf(
              stderr,
              "Ran past end of config file in YEI description\n");
          return -1;
      }

      // Get the arguments (class, name, port, baud, calibrate_gyros, tare,
      // frames_per_second.
      char classname[LINESIZE]; // We need to read the class, since we've not pre-parsed
      if (sscanf(line, "%511s%511s%d%d%d%lf%f%f%f%d", classname, name, &logical_id,
                 &calibrate_gyros, &tare, &frames_per_second, &red_LED,
                 &green_LED, &blue_LED, &LED_mode) != 10) {
          fprintf(stderr, "Bad setup_YEI_3Space_Sensor_Wireless line: %s\n", line);
          return -1;
      }

      // If the last character in the line is a backslash, '\', then
      // the following line is an additional command to send to the
      // YEI at reset time. So long as we find lines with backslashes
      // at the ends, we add them to the command list to send. Note
      // that there is a newline at the end of the line, following the
      // backslash.
      num_reset_commands = 0;
      while (line[strlen(line) - 2] == '\\') {
          // Read the VRPN_CONFIG_NEXT line
          if (fgets(line, LINESIZE, config_file) == NULL) {
              fprintf(
                  stderr,
                  "Ran past end of config file in YEI description\n");
              return -1;
          }

          // Copy the first string from the line into a new character
          // array and then store this into the reset_commands list
          // if we haven't run out of room for them.
          if (num_reset_commands < MAX_RESET_COMMANDS) {
            char command[LINESIZE];
            sscanf(line, "%s", command);
            char *command_copy = new char[strlen(command)+1];
            if (command_copy == NULL) {
              fprintf(stderr, "Out of memory in YEI description\n");
              return -1;
            }
            strcpy(command_copy, command);
            reset_commands[num_reset_commands++] = command_copy;
          }
      }

      // Open the device
      if (verbose) {
          printf("Opening setup_YEI_3Space_Sensor_Wireless: %s on the same port, baud %d\n",
              name, baud_rate);
          if (num_reset_commands > 0) {
            printf("... additional reset commands follow:\n");
            for (int i = 0; i < num_reset_commands; i++) {
              printf("  %s\n", reset_commands[i]);
            }
          }
      }
      _devices->add(new vrpn_YEI_3Space_Sensor_Wireless(
          name, connection,
          logical_id, serial_fd, calibrate_gyros != 0, tare != 0,
          frames_per_second, red_LED, green_LED, blue_LED, LED_mode,
          reset_commands));

      // Free up any additional-reset command data.
      for (int i = 0; i < MAX_RESET_COMMANDS; i++) {
        if (reset_commands[i] != NULL) {
          delete [] reset_commands[i];
          reset_commands[i] = NULL;
        }
      }
    } // End of while new devices ('/' at and of line)

    return 0;
}

int vrpn_Generic_Server_Object::setup_Tracker_DeadReckoning_Rotation(char *&pch, char *line,
    FILE * /*config_file*/)
{
    char name[LINESIZE], origName[LINESIZE];
    int numSensors;
    float predictionTime;

    VRPN_CONFIG_NEXT();
    // Get the arguments (tracker_name, sensors, rate)
    if (sscanf(pch, "%511s%511s%d%g", name, origName, &numSensors, &predictionTime) != 4) {
        fprintf(stderr, "Bad vrpn_Tracker_DeadReckoning_Rotation line: %s\n", line);
        return -1;
    }

    // Open the tracker
    if (verbose)
        printf("Opening vrpn_Tracker_DeadReckoning_Rotation: %s from %s with %d sensors, prediction time %f\n", name,
        origName, numSensors, predictionTime);
    _devices->add(new vrpn_Tracker_DeadReckoning_Rotation(name, connection, origName,
        numSensors, predictionTime));

    return 0;
}

int vrpn_Generic_Server_Object::setup_Oculus_DK1(char *&pch, char *line, FILE *)
{
  char s2[LINESIZE];

  VRPN_CONFIG_NEXT();
  int ret = sscanf(pch, "%511s", s2);
  if (ret != 1) {
    fprintf(stderr, "Bad Oculus_DK1 line: %s\n", line);
    return -1;
  }

  // Open the Oculus DK2
  if (verbose) {
    printf("Opening vrpn_Oculus_DK1\n");
  }

#ifdef VRPN_USE_HID
  // Open the tracker
  _devices->add(new vrpn_Oculus_DK1(s2, connection));
#else
  fprintf(stderr,
    "Oculus_DK1 driver works only with VRPN_USE_HID defined!\n");
#endif
  return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Oculus_DK2_LEDs(char *&pch, char *line, FILE *)
{
  char s2[LINESIZE];

  VRPN_CONFIG_NEXT();
  int ret = sscanf(pch, "%511s", s2);
  if (ret != 1) {
    fprintf(stderr, "Bad Oculus_DK2_LEDs line: %s\n", line);
    return -1;
  }

  // Open the Oculus DK2
  if (verbose) {
    printf("Opening vrpn_Oculus_DK2\n");
  }

#ifdef VRPN_USE_HID
  // Open the tracker
  _devices->add(new vrpn_Oculus_DK2_LEDs(s2, connection));
#else
  fprintf(stderr,
    "Oculus_DK2 driver works only with VRPN_USE_HID defined!\n");
#endif
  return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Oculus_DK2_inertial(char *&pch, char *line, FILE *)
{
  char s2[LINESIZE];

  VRPN_CONFIG_NEXT();
  int ret = sscanf(pch, "%511s", s2);
  if (ret != 1) {
    fprintf(stderr, "Bad Oculus_DK2_inertial line: %s\n", line);
    return -1;
  }

  // Open the Oculus DK2
  if (verbose) {
    printf("Opening vrpn_Oculus_inertial\n");
  }

#ifdef VRPN_USE_HID
  // Open the tracker
  _devices->add(new vrpn_Oculus_DK2_inertial(s2, connection));
#else
  fprintf(stderr,
    "Oculus_DK2 driver works only with VRPN_USE_HID defined!\n");
#endif
  return 0; // successful completion
}

// This function will read one line of the vrpn_IMU_Axis_Param configuration
// (matching one axis) and fill in the data for that axis. The axis name,
// the file to read from, and the axis to fill in are passed as parameters.
// It returns 0 on success and -1 on failure.

int vrpn_Generic_Server_Object::get_IMU_Param_Line(char *line, vrpn_IMU_Axis_Params *params)
{
    char name[LINESIZE];
    int channels[3];
    float offsets[3];
    float scales[3];

    // Get the values from the line
    if (sscanf(line, "%511s%d%g%g%d%g%g%d%g%g", name,
               &channels[0], &offsets[0], &scales[0],
               &channels[1], &offsets[1], &scales[1],
               &channels[2], &offsets[2], &scales[2]
              ) != 10) {
        fprintf(stderr, "IMU_Param Axis: Bad param line: %s\n", line);
        return -1;
    }

    params->name = name;
    for (size_t i = 0; i < 3; i++) {
      params->channels[i] = channels[i];
      params->offsets[i] = offsets[i];
      params->scales[i] = scales[i];
    }

    return 0;
}

int vrpn_Generic_Server_Object::setup_IMU_Magnetometer(char *&pch, char *line,
                                                        FILE *config_file)
{
    char s2[LINESIZE];
    float f1;
    vrpn_IMU_Axis_Params params;

    VRPN_CONFIG_NEXT();

    if (sscanf(pch, "%511s%g", s2, &f1) != 2) {
        fprintf(stderr, "Bad vrpn_IMU_Magnetometer line: %s\n", line);
        return -1;
    }

    if (verbose) {
        printf("Opening vrpn_IMU_Magnetometer: "
               "%s with update rate %g\n",
               s2, f1);
    }

    // Scan the following line in the configuration file to fill
    // in the start-up parameters for the axes
    {
        char line[LINESIZE];

        // Read in the line
        if (fgets(line, LINESIZE, config_file) == NULL) {
            perror("IMU_Magnetometer Can't read axis parameter line!");
            return -1;
        }

        // Parse the line
        if (get_IMU_Param_Line(line, &params)) {
                fprintf(stderr, "Can't read params line for IMU_Magnetometer\n");
                return -1;
        }
    }

    _devices->add(new vrpn_IMU_Magnetometer(s2, connection, params, f1, false));
    return 0;
}

int vrpn_Generic_Server_Object::setup_IMU_SimpleCombiner(char *&pch, char *line,
  FILE *config_file)
{
  char s2[LINESIZE];
  float f1;
  vrpn_IMU_Axis_Params accel, rotate;
  char magname[LINESIZE];

  VRPN_CONFIG_NEXT();

  if (sscanf(pch, "%511s%g", s2, &f1) != 2) {
    fprintf(stderr, "Bad vrpn_IMU_SimpleCombiner line: %s\n", line);
    return -1;
  }

  if (verbose) {
    printf("Opening vrpn_IMU_SimpleCombiner: "
      "%s with update rate %g\n",
      s2, f1);
  }

  // Scan the following line in the configuration file to fill
  // in the start-up parameters for the accelerometer
  {
    char line[LINESIZE];

    // Read in the line
    if (fgets(line, LINESIZE, config_file) == NULL) {
      perror("vrpn_IMU_SimpleCombiner Can't read axis accelerometer line!");
      return -1;
    }

    // Parse the line
    if (get_IMU_Param_Line(line, &accel)) {
      fprintf(stderr, "Can't read accelerometer line for vrpn_IMU_SimpleCombiner\n");
      return -1;
    }
  }

  // Scan the following line in the configuration file to fill
  // in the start-up parameters for the rotational linear measurement
  {
    char line[LINESIZE];

    // Read in the line
    if (fgets(line, LINESIZE, config_file) == NULL) {
      perror("vrpn_IMU_SimpleCombiner Can't read axis rotation line!");
      return -1;
    }

    // Parse the line
    if (get_IMU_Param_Line(line, &rotate)) {
      fprintf(stderr, "Can't read rotation line for vrpn_IMU_SimpleCombiner\n");
      return -1;
    }
  }

  // Scan the following line in the configuration file to fill
  // in the name for the magnetometer
  {
    char line[LINESIZE];

    // Read in the line
    if (fgets(line, LINESIZE, config_file) == NULL) {
      perror("vrpn_IMU_SimpleCombiner Can't read axis rotation line!");
      return -1;
    }

    if (sscanf(line, "%511s", magname) != 1) {
      fprintf(stderr, "Bad vrpn_IMU_SimpleCombiner magnetometer name: %s\n", line);
      return -1;
    }
  }

  vrpn_Tracker_IMU_Params params;
  params.d_acceleration = accel;
  params.d_rotational_vel = rotate;
  params.d_magnetometer_name = magname;
  if (params.d_magnetometer_name == "NULL") {
    params.d_magnetometer_name.clear();
  }
  _devices->add(new vrpn_IMU_SimpleCombiner(s2, connection, &params, f1, false));
  return 0;
}

int vrpn_Generic_Server_Object::setup_nVidia_shield_USB(char *&pch, char *line, FILE *)
{
  char s2[LINESIZE];

  VRPN_CONFIG_NEXT();
  int ret = sscanf(pch, "%511s", s2);
  if (ret != 1) {
    fprintf(stderr, "Bad nVidia_shield_USB line: %s\n", line);
    return -1;
  }

  // Open the shield
  if (verbose) {
    printf("Opening vrpn_nVidia_shield_USB\n");
  }

#ifdef VRPN_USE_HID
  _devices->add(new vrpn_nVidia_shield_USB(s2, connection));
#else
  fprintf(stderr,
    "nVidia_shield_USB driver works only with VRPN_USE_HID defined!\n");
#endif
  return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_nVidia_shield_stealth_USB(char *&pch, char *line, FILE *)
{
  char s2[LINESIZE];

  VRPN_CONFIG_NEXT();
  int ret = sscanf(pch, "%511s", s2);
  if (ret != 1) {
    fprintf(stderr, "Bad nVidia_shield_stealth_USB line: %s\n", line);
    return -1;
  }

  // Open the shield
  if (verbose) {
    printf("Opening vrpn_nVidia_shield_stealth_USB\n");
  }

#ifdef VRPN_USE_HID
  _devices->add(new vrpn_nVidia_shield_stealth_USB(s2, connection));
#else
  fprintf(stderr,
    "nVidia_shield_stealth_USB driver works only with VRPN_USE_HID defined!\n");
#endif
  return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Adafruit_10DOF(char *&pch, char *line, FILE *)
{
  char vrpnName[LINESIZE], devName[LINESIZE];
  double interval;

  VRPN_CONFIG_NEXT();
  int ret = sscanf(pch, "%511s%511s%lg", vrpnName, devName, &interval);
  if (ret != 3) {
    fprintf(stderr, "Bad Adafruit_10DOF line: %s\n", line);
    return -1;
  }

  // Open the Adafruit
  if (verbose) {
    printf("Opening Adafruit_10DOF\n");
  }

#ifdef VRPN_USE_I2CDEV
  _devices->add(new vrpn_Adafruit_10DOF(vrpnName, connection, devName, interval));
#else
  fprintf(stderr,
    "Adafruit_10DOF driver only compiled in when VRPN_USE_I2CDEV is defined!\n");
#endif
  return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_OzzMaker_BerryIMU(char *&pch, char *line, FILE *)
{
  char vrpnName[LINESIZE], devName[LINESIZE];
  double interval;

  VRPN_CONFIG_NEXT();
  int ret = sscanf(pch, "%511s%511s%lg", vrpnName, devName, &interval);
  if (ret != 3) {
    fprintf(stderr, "Bad OzzMaker_BerryIMU line: %s\n", line);
    return -1;
  }

  // Open the device
  if (verbose) {
    printf("Opening OzzMaker_BerryIMU\n");
  }

#ifdef VRPN_USE_I2CDEV
  _devices->add(new vrpn_OzzMaker_BerryIMU(vrpnName, connection, devName, interval));
#else
  fprintf(stderr,
    "OzzMaker_BerryIMU driver only compiled in when VRPN_USE_I2CDEV is defined!\n");
#endif
  return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Laputa(char *&pch, char *line, FILE *)
{
  char s2[LINESIZE];

  VRPN_CONFIG_NEXT();
  int ret = sscanf(pch, "%511s", s2);
  if (ret != 1) {
    fprintf(stderr, "Bad Laputa line: %s\n", line);
    return -1;
  }

#ifdef VRPN_USE_HID
  // Open the Laputa
  if (verbose) {
    printf("Opening vrpn_Laputa\n");
  }

  // Open the tracker
  _devices->add(new vrpn_Laputa(s2, connection));
#else
  fprintf(stderr,
    "Laputa driver works only with VRPN_USE_HID defined!\n");
#endif
  return 0; // successful completion
}

int vrpn_Generic_Server_Object::setup_Vality_vGlass(char *&pch, char *line, FILE *)
{
    char s2[LINESIZE];

    VRPN_CONFIG_NEXT();
    int ret = sscanf(pch, "%511s", s2);
    if (ret != 1) {
        fprintf(stderr, "Bad Vality_vGlass line: %s\n", line);
        return -1;
    }

    // Open the Oculus DK2
    if (verbose) {
        printf("Opening Vality_vGlass\n");
    }

#ifdef VRPN_USE_HID
    // Open the tracker
    _devices->add(new vrpn_Vality_vGlass(s2, connection));
#else
    fprintf(stderr,
            "Vality_vGlass driver works only with VRPN_USE_HID defined!\n");
#endif
    return 0; // successful completion
}

#undef VRPN_CONFIG_NEXT

vrpn_Generic_Server_Object::vrpn_Generic_Server_Object(
    vrpn_Connection *connection_to_use, const char *config_file_name,
    bool be_verbose, bool bail_on_open_error)
    : connection(connection_to_use)
    , d_doing_okay(true)
    , verbose(be_verbose)
    , d_bail_on_open_error(bail_on_open_error)
    , _devices(new vrpn_MainloopContainer)

{
    FILE *config_file;

    // Open the configuration file
    if (verbose) {
        printf("Reading from config file %s\n", config_file_name);
    }
    if ((config_file = fopen(config_file_name, "r")) == NULL) {
        perror("vrpn_Generic_Server_Object::vrpn_Generic_Server_Object(): "
               "Cannot open config file");
        fprintf(stderr, "  (filename %s)\n", config_file_name);
        d_doing_okay = false;
        return;
    }

    // Store the locale that was set before we came in here.
    // The global locale is obtained by using the default
    // constructor.
    std::locale const orig_locale = std::locale();

    // Read the configuration file, creating a device for each entry.
    // Each entry is on one line, which starts with the name of the
    //   class of the object that is to be created.
    // If we fail to open a certain device, print a message and decide
    //  whether we should bail.
    {
        char line[LINESIZE]; // Line read from the input file
        char *pch;
        char scrap[LINESIZE];
        char s1[LINESIZE];
        int retval;

        // Read lines from the file until we run out
        while (fgets(line, LINESIZE, config_file) != NULL) {

            // Set the global locale to be "C", the classic one, so that
            // when we parse the configuration file it will use dots for
            // decimal points even if the local standard is commas.
            // putting them into the global locale.  We tried putting this
            // code above, outside the parsing loop, but it did not have
            // the desired effect when placed there.  This is the earliest
            // we were able to put it and have it work.
            std::locale::global(std::locale("C"));

            // Make sure the line wasn't too long
            if (strlen(line) >= LINESIZE - 1) {
                fprintf(stderr, "vrpn_Generic_Server_Object::vrpn_Generic_"
                                "Server_Object(): Line too long in config "
                                "file: %s\n",
                        line);
                if (d_bail_on_open_error) {
                    d_doing_okay = false;
                    fclose(config_file);
                    return;
                } else {
                    continue; // Skip this line
                }
            }

            // Ignore comments and empty lines.  Skip white space before comment
            // mark (#).
            if (strlen(line) < 3) {
                continue;
            }
            bool ignore = false;
            for (int j = 0; line[j] != '\0'; j++) {
                if (line[j] == ' ' || line[j] == '\t') {
                    continue;
                }
                if (line[j] == '#') {
                    ignore = true;
                }
                break;
            }
            if (ignore) {
                continue;
            }

            // copy for strtok work
            vrpn_strcpy(scrap, line);
            scrap[sizeof(scrap) - 1] = '\0';
// Figure out the device from the name and handle appropriately

// WARNING: SUBSTRINGS WILL MATCH THE EARLIER STRING, SO
// ADD AN EMPTY SPACE TO THE END OF STATIC STRINGS!!!!

#define VRPN_ISIT(s) !strcmp(pch = strtok(scrap, " \t"), s)
#define VRPN_CHECK(s)                                                          \
    try {                                                                      \
      retval = (s)(pch, line, config_file);                                    \
    } catch (const std::exception& e) {                                        \
        d_doing_okay = false;                                                  \
        fprintf(stderr, "vrpn_Generic_Server_Object::vrpn_Generic_Server_Object(): " \
          "Unexpected exception (%s)\n", e.what());                            \
        fclose(config_file);                                                   \
        return;                                                                \
    } catch (...) {                                                            \
        d_doing_okay = false;                                                  \
        fprintf(stderr, "vrpn_Generic_Server_Object::vrpn_Generic_Server_Object(): " \
          "Unexpected exception (out of memory?)\n");                          \
        fclose(config_file);                                                   \
        return;                                                                \
    }                                                                          \
    if (retval && d_bail_on_open_error) {                                      \
        d_doing_okay = false;                                                  \
        fclose(config_file);                                                   \
        return;                                                                \
    } else {                                                                   \
        continue;                                                              \
    }

            // This is a hack to get around a hard limit on how many
            // if-then-else
            // clauses you can have in the same function on Microsoft compilers.
            // We break the checking into multiple chunks as needed.
            // If any of the clauses work, we've found it.  Only if the else
            // clause
            // in the batch has not been called do we continue looking.
            bool found_it_yet = true;

            if (VRPN_ISIT("vrpn_raw_SGIBox")) {
                VRPN_CHECK(setup_raw_SGIBox);
            }
            else if (VRPN_ISIT("vrpn_SGIBOX")) {
                VRPN_CHECK(setup_SGIBox);
            }
            else if (VRPN_ISIT("vrpn_JoyFly")) {
                VRPN_CHECK(setup_JoyFly);
            }
            else if (VRPN_ISIT("vrpn_Tracker_AnalogFly")) {
                VRPN_CHECK(setup_Tracker_AnalogFly);
            }
            else if (VRPN_ISIT("vrpn_Tracker_ButtonFly")) {
                VRPN_CHECK(setup_Tracker_ButtonFly);
            }
            else if (VRPN_ISIT("vrpn_Joystick")) {
                VRPN_CHECK(setup_Joystick);
            }
            else if (VRPN_ISIT("vrpn_Joylin")) {
                VRPN_CHECK(setup_Joylin);
            }
            else if (VRPN_ISIT("vrpn_Joywin32")) {
                VRPN_CHECK(setup_Joywin32);
            }
            else if (VRPN_ISIT("vrpn_Button_Example")) {
                VRPN_CHECK(setup_Example_Button);
            }
            else if (VRPN_ISIT("vrpn_Dial_Example")) {
                VRPN_CHECK(setup_Example_Dial);
            }
            else if (VRPN_ISIT("vrpn_CerealBox")) {
                VRPN_CHECK(setup_CerealBox);
            }
            else if (VRPN_ISIT("vrpn_Magellan")) {
                VRPN_CHECK(setup_Magellan);
            }
            else if (VRPN_ISIT("vrpn_Spaceball")) {
                VRPN_CHECK(setup_Spaceball);
            }
            else if (VRPN_ISIT("vrpn_Radamec_SPI")) {
                VRPN_CHECK(setup_Radamec_SPI);
            }
            else if (VRPN_ISIT("vrpn_Zaber")) {
                VRPN_CHECK(setup_Zaber);
            }
            else if (VRPN_ISIT("vrpn_BiosciencesTools")) {
                VRPN_CHECK(setup_BiosciencesTools);
            }
            else if (VRPN_ISIT("vrpn_OmegaTemperature")) {
                VRPN_CHECK(setup_OmegaTemperature);
            }
            else if (VRPN_ISIT("vrpn_IDEA")) {
                VRPN_CHECK(setup_IDEA);
            }
            else if (VRPN_ISIT("vrpn_5dt")) {
                VRPN_CHECK(setup_5dt);
            }
            else if (VRPN_ISIT("vrpn_5dt16")) {
                VRPN_CHECK(setup_5dt16);
            }
            else if (VRPN_ISIT("vrpn_Button_5DT_Server")) {
                VRPN_CHECK(setup_Button_5DT_Server);
            }
            else if (VRPN_ISIT("vrpn_ImmersionBox")) {
                VRPN_CHECK(setup_ImmersionBox);
            }
            else if (VRPN_ISIT("vrpn_Tracker_Dyna")) {
                VRPN_CHECK(setup_Tracker_Dyna);
            }
            else if (VRPN_ISIT("vrpn_Tracker_Fastrak")) {
                VRPN_CHECK(setup_Tracker_Fastrak);
            }
            else if (VRPN_ISIT("vrpn_Tracker_NDI_Polaris")) {
                VRPN_CHECK(setup_Tracker_NDI_Polaris);
            }
            else if (VRPN_ISIT("vrpn_Tracker_Isotrak")) {
                VRPN_CHECK(setup_Tracker_Isotrak);
            }
            else if (VRPN_ISIT("vrpn_Tracker_NDI_Polaris")) {
                VRPN_CHECK(setup_Tracker_NDI_Polaris);
            }
            else if (VRPN_ISIT("vrpn_Tracker_Liberty")) {
                VRPN_CHECK(setup_Tracker_Liberty);
            }
            else if (VRPN_ISIT("vrpn_Tracker_LibertyHS")) {
                VRPN_CHECK(setup_Tracker_LibertyHS);
            }
            else if (VRPN_ISIT("vrpn_Tracker_3Space")) {
                VRPN_CHECK(setup_Tracker_3Space);
            }
            else if (VRPN_ISIT("vrpn_Tracker_Flock")) {
                VRPN_CHECK(setup_Tracker_Flock);
            }
            else if (VRPN_ISIT("vrpn_Tracker_Flock_Parallel")) {
                VRPN_CHECK(setup_Tracker_Flock_Parallel);
            }
            else if (VRPN_ISIT("vrpn_Tracker_3DMouse")) {
                VRPN_CHECK(setup_Tracker_3DMouse);
            }
            else if (VRPN_ISIT("vrpn_Tracker_NULL")) {
                VRPN_CHECK(setup_Tracker_NULL);
            }
            else if (VRPN_ISIT("vrpn_Button_Python")) {
                VRPN_CHECK(setup_Button_Python);
            }
            else if (VRPN_ISIT("vrpn_Button_PinchGlove")) {
                VRPN_CHECK(setup_Button_PinchGlove);
            }
            else if (VRPN_ISIT("vrpn_Button_SerialMouse")) {
                VRPN_CHECK(setup_Button_SerialMouse);
            }
            else if (VRPN_ISIT("vrpn_Wanda")) {
                VRPN_CHECK(setup_Wanda);
            }
            else if (VRPN_ISIT("vrpn_Mouse")) {
                VRPN_CHECK(setup_Mouse);
            }
            else if (VRPN_ISIT("vrpn_DevInput")) {
                VRPN_CHECK(setup_DevInput);
            }
            else if (VRPN_ISIT("vrpn_Streaming_Arduino")) {
                VRPN_CHECK(setup_StreamingArduino);
            }
            else if (VRPN_ISIT("vrpn_Tng3")) {
                VRPN_CHECK(setup_Tng3);
            }
            else if (VRPN_ISIT("vrpn_TimeCode_Generator")) {
                VRPN_CHECK(setup_Timecode_Generator);
            }
            else if (VRPN_ISIT("vrpn_Tracker_InterSense")) {
                VRPN_CHECK(setup_Tracker_InterSense);
            }
            else if (VRPN_ISIT("vrpn_DirectXFFJoystick")) {
                VRPN_CHECK(setup_DirectXFFJoystick);
            }
            else if (VRPN_ISIT("vrpn_DirectXRumblePad")) {
                VRPN_CHECK(setup_RumblePad);
            }
            else {
                found_it_yet = false;
            }

            if (!found_it_yet) {
                if (VRPN_ISIT("vrpn_XInputGamepad")) {
                    VRPN_CHECK(setup_XInputPad);
                }
                else if (VRPN_ISIT("vrpn_GlobalHapticsOrb")) {
                    VRPN_CHECK(setup_GlobalHapticsOrb);
                }
                else if (VRPN_ISIT("vrpn_Phantom")) {
                    VRPN_CHECK(setup_Phantom);
                }
                else if (VRPN_ISIT("vrpn_ADBox")) {
                    VRPN_CHECK(setup_ADBox);
                }
                else if (VRPN_ISIT("vrpn_VPJoystick")) {
                    VRPN_CHECK(setup_VPJoystick);
                }
                else if (VRPN_ISIT("vrpn_Tracker_DTrack")) {
                    VRPN_CHECK(setup_DTrack);
                }
                else if (VRPN_ISIT("vrpn_NI_Analog_Output")) {
                    VRPN_CHECK(setup_NationalInstrumentsOutput);
                }
                else if (VRPN_ISIT("vrpn_National_Instruments")) {
                    VRPN_CHECK(setup_NationalInstruments);
                }
                else if (VRPN_ISIT("vrpn_nikon_controls")) {
                    VRPN_CHECK(setup_nikon_controls);
                }
                else if (VRPN_ISIT("vrpn_Tek4662")) {
                    VRPN_CHECK(setup_Poser_Tek4662);
                }
                else if (VRPN_ISIT("vrpn_Poser_Analog")) {
                    VRPN_CHECK(setup_Poser_Analog);
                }
                else if (VRPN_ISIT("vrpn_Tracker_Crossbow")) {
                    VRPN_CHECK(setup_Tracker_Crossbow);
                }
                else if (VRPN_ISIT("vrpn_3DMicroscribe")) {
                    VRPN_CHECK(setup_3DMicroscribe);
                }
                else if (VRPN_ISIT("vrpn_Keyboard")) {
                    VRPN_CHECK(templated_setup_device_name_only<vrpn_Keyboard>);
                }
                else if (VRPN_ISIT("vrpn_Button_USB")) {
                    VRPN_CHECK(setup_Button_USB);
                }
                else if (VRPN_ISIT("vrpn_Analog_USDigital_A2")) {
                    VRPN_CHECK(setup_Analog_USDigital_A2);
                }
                else if (VRPN_ISIT("vrpn_Button_NI_DIO24")) {
                    VRPN_CHECK(setup_Button_NI_DIO24);
                }
                else if (VRPN_ISIT("vrpn_Tracker_PhaseSpace")) {
                    VRPN_CHECK(setup_Tracker_PhaseSpace);
                }
                else if (VRPN_ISIT("vrpn_Auxiliary_Logger_Server_Generic")) {
                    VRPN_CHECK(setup_Logger);
                }
                else if (VRPN_ISIT("vrpn_Imager_Stream_Buffer")) {
                    VRPN_CHECK(setup_ImageStream);
                }
                else if (VRPN_ISIT("vrpn_Contour_ShuttleXpress")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Contour_ShuttleXpress>);
                }
		else if (VRPN_ISIT("vrpn_Contour_ShuttlePROv2")) {
			VRPN_CHECK(templated_setup_HID_device_name_only<
				vrpn_Contour_ShuttlePROv2>);
		}
		else if (VRPN_ISIT("vrpn_Retrolink_GameCube")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Retrolink_GameCube>);
                }
                else if (VRPN_ISIT("vrpn_Retrolink_Genesis")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Retrolink_Genesis>);
                }
                else if (VRPN_ISIT("vrpn_Contour_ShuttleXpress")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Contour_ShuttleXpress>);
                }
                else if (VRPN_ISIT("vrpn_Futaba_InterLink_Elite")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Futaba_InterLink_Elite>);
                }
                else if (VRPN_ISIT("vrpn_Griffin_PowerMate")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Griffin_PowerMate>);
                }
                else if (VRPN_ISIT("vrpn_Xkeys_Desktop")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Xkeys_Desktop>);
                }
                else if (VRPN_ISIT("vrpn_Xkeys_Pro")) {
                    VRPN_CHECK(
                        templated_setup_HID_device_name_only<vrpn_Xkeys_Pro>);
                }
                else if (VRPN_ISIT("vrpn_Xkeys_Joystick")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Xkeys_Joystick>);
                }
                else if (VRPN_ISIT("vrpn_Xkeys_Joystick12")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Xkeys_Joystick12>);
                }
                else if (VRPN_ISIT("vrpn_Xkeys_Jog_And_Shuttle")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Xkeys_Jog_And_Shuttle>);
                }
                else if (VRPN_ISIT("vrpn_Xkeys_Jog_And_Shuttle12")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Xkeys_Jog_And_Shuttle12>);
                }
                else if (VRPN_ISIT("vrpn_Xkeys_Jog_And_Shuttle68")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Xkeys_Jog_And_Shuttle68>);
                }
                else if (VRPN_ISIT("vrpn_Xkeys_XK3")) {
                    VRPN_CHECK(
                        templated_setup_HID_device_name_only<vrpn_Xkeys_XK3>);
                }
                else if (VRPN_ISIT("vrpn_Logitech_Extreme_3D_Pro")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Logitech_Extreme_3D_Pro>);
                }
                else if (VRPN_ISIT("vrpn_Saitek_ST290_Pro")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Saitek_ST290_Pro>);
                }
                else if (VRPN_ISIT("vrpn_CHProducts_Fighterstick_USB")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_CHProducts_Fighterstick_USB>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_Navigator")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_Navigator>);
                }
                else if (VRPN_ISIT(
                             "vrpn_3DConnexion_Navigator_for_Notebooks")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_Navigator_for_Notebooks>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_Traveler")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_Traveler>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpaceExplorer")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_SpaceExplorer>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpaceMouse")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_SpaceMouse>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpaceMousePro")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_SpaceMousePro>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpaceMouseCompact")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_SpaceMouseCompact>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpaceMouseWireless")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_SpaceMouseWireless>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpaceMouseProWireless")) {
		                VRPN_CHECK(templated_setup_device_name_only<
				                vrpn_3DConnexion_SpaceMouseProWireless>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpaceBall5000")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_SpaceBall5000>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpacePilot")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                        vrpn_3DConnexion_SpacePilot>);
                }
                else if (VRPN_ISIT("vrpn_3DConnexion_SpacePilotPro")) {
                    VRPN_CHECK(templated_setup_device_name_only<
                               vrpn_3DConnexion_SpacePilotPro>);
                }
                else if (VRPN_ISIT("vrpn_Microsoft_SideWinder_Precision_2")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Microsoft_SideWinder_Precision_2>);
                }
                else if (VRPN_ISIT("vrpn_Microsoft_SideWinder")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Microsoft_SideWinder>);
                }
                else if (VRPN_ISIT("vrpn_Microsoft_Controller_Raw_Xbox_S")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Microsoft_Controller_Raw_Xbox_S>);
                }
                else if (VRPN_ISIT("vrpn_Microsoft_Controller_Raw_Xbox_360")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Microsoft_Controller_Raw_Xbox_360>);
                }
                else if (VRPN_ISIT("vrpn_Microsoft_Controller_Raw_Xbox_360_Wireless")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Microsoft_Controller_Raw_Xbox_360_Wireless>);
                }
                else if (VRPN_ISIT("vrpn_Afterglow_Ax1_For_Xbox_360")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_Afterglow_Ax1_For_Xbox_360>);
                }
                else if (VRPN_ISIT("vrpn_Tracker_MotionNode")) {
                    VRPN_CHECK(setup_Tracker_MotionNode);
                }
                else if (VRPN_ISIT("vrpn_Tracker_GPS")) {
                    VRPN_CHECK(setup_Tracker_GPS);
                }
                else if (VRPN_ISIT("vrpn_WiiMote")) {
                    VRPN_CHECK(setup_WiiMote);
                }
                else if (VRPN_ISIT("vrpn_Tracker_WiimoteHead")) {
                    VRPN_CHECK(setup_Tracker_WiimoteHead);
                }
                else if (VRPN_ISIT("vrpn_Freespace")) {
                    VRPN_CHECK(setup_Freespace);
                }
                else if (VRPN_ISIT("vrpn_Tracker_NovintFalcon")) {
                    VRPN_CHECK(setup_Tracker_NovintFalcon);
                }
                else if (VRPN_ISIT("vrpn_Tracker_TrivisioColibri")) {
                    VRPN_CHECK(setup_Tracker_TrivisioColibri);
                }
                else if (VRPN_ISIT ("vrpn_Tracker_Colibri")) {
                    VRPN_CHECK (setup_Tracker_Colibri);
                }
                else if (VRPN_ISIT("vrpn_Tracker_SpacePoint")) {
                    VRPN_CHECK(setup_SpacePoint);
                }
                else if (VRPN_ISIT("vrpn_Tracker_Wintracker")) {
                    VRPN_CHECK(setup_Wintracker);
                }
                else if (VRPN_ISIT("vrpn_Tracker_GameTrak")) {
                    VRPN_CHECK(setup_Tracker_GameTrak);
                }
                else if (VRPN_ISIT("vrpn_Atmel")) {
                    VRPN_CHECK(setup_Atmel);
                }
                else if (VRPN_ISIT("vrpn_inertiamouse")) {
                    VRPN_CHECK(setup_inertiamouse);
                }
                else if (VRPN_ISIT("vrpn_Event_Mouse")) {
                    VRPN_CHECK(setup_Event_Mouse);
                }
                else if (VRPN_ISIT("vrpn_Dream_Cheeky_USB_roll_up_drums")) {
                    VRPN_CHECK(templated_setup_HID_device_name_only<
                        vrpn_DreamCheeky_Drum_Kit>);
                }
                else if (VRPN_ISIT("vrpn_LUDL_USBMAC6000")) {
                    VRPN_CHECK(setup_LUDL_USBMAC6000);
                }
                else if (VRPN_ISIT("vrpn_Analog_5dtUSB_Glove5Left")) {
                    VRPN_CHECK(setup_Analog_5dtUSB_Glove5Left);
                }
                else if (VRPN_ISIT("vrpn_Analog_5dtUSB_Glove5Right")) {
                    VRPN_CHECK(setup_Analog_5dtUSB_Glove5Right);
                }
                else if (VRPN_ISIT("vrpn_Analog_5dtUSB_Glove14Left")) {
                    VRPN_CHECK(setup_Analog_5dtUSB_Glove14Left);
                }
                else if (VRPN_ISIT("vrpn_Analog_5dtUSB_Glove14Right")) {
                    VRPN_CHECK(setup_Analog_5dtUSB_Glove14Right);
                }
                else if (VRPN_ISIT("vrpn_Tracker_FilterOneEuro")) {
                    VRPN_CHECK(setup_Tracker_FilterOneEuro);
                }
                else if (VRPN_ISIT("vrpn_Tracker_OSVRHackerDevKit")) {
                    VRPN_CHECK(setup_Tracker_OSVRHackerDevKit);
                }
                else if (VRPN_ISIT("vrpn_Tracker_RazerHydra")) {
                    VRPN_CHECK(setup_Tracker_RazerHydra);
                }
                else if (VRPN_ISIT ("vrpn_Tracker_ThalmicLabsMyo")) {
                    VRPN_CHECK(setup_Tracker_ThalmicLabsMyo);
                }
                else if (VRPN_ISIT("vrpn_Tracker_zSight")) {
                    VRPN_CHECK(setup_Tracker_zSight);
                }
                else if (VRPN_ISIT("vrpn_Tracker_ViewPoint")) {
                    VRPN_CHECK(setup_Tracker_ViewPoint);
                }
                else if (VRPN_ISIT("vrpn_Tracker_G4")) {
                    VRPN_CHECK(setup_Tracker_G4);
                }
                else if (VRPN_ISIT("vrpn_Tracker_LibertyPDI")) {
                    VRPN_CHECK(setup_Tracker_LibertyPDI);
                }
                else if (VRPN_ISIT("vrpn_Tracker_FastrakPDI")) {
                    VRPN_CHECK(setup_Tracker_FastrakPDI);
                }
                else if (VRPN_ISIT("vrpn_Tracker_JsonNet")) {
                    VRPN_CHECK(setup_Tracker_JsonNet);
                }
                else if (VRPN_ISIT("vrpn_YEI_3Space_Sensor")) {
                    VRPN_CHECK(setup_YEI_3Space_Sensor);
                }
                else if (VRPN_ISIT("vrpn_YEI_3Space_Sensor_Wireless")) {
                    VRPN_CHECK(setup_YEI_3Space_Sensor_Wireless);
                }
                else if (VRPN_ISIT("vrpn_Tracker_DeadReckoning_Rotation")) {
                    VRPN_CHECK(setup_Tracker_DeadReckoning_Rotation);
                }
                else if (VRPN_ISIT("vrpn_Oculus_DK1")) {
                  VRPN_CHECK(setup_Oculus_DK1);
                }
                else if (VRPN_ISIT("vrpn_Oculus_DK2_LEDs")) {
                  VRPN_CHECK(setup_Oculus_DK2_LEDs);
                }
                else if (VRPN_ISIT("vrpn_Oculus_DK2_inertial")) {
                  VRPN_CHECK(setup_Oculus_DK2_inertial);
                }
                else if (VRPN_ISIT("vrpn_IMU_Magnetometer")) {
                    VRPN_CHECK(setup_IMU_Magnetometer);
                }
                else if (VRPN_ISIT("vrpn_IMU_SimpleCombiner")) {
                  VRPN_CHECK(setup_IMU_SimpleCombiner);
                }
                else if (VRPN_ISIT("vrpn_nVidia_shield_USB")) {
                  VRPN_CHECK(setup_nVidia_shield_USB);
                }
                else if (VRPN_ISIT("vrpn_nVidia_shield_stealth_USB")) {
                  VRPN_CHECK(setup_nVidia_shield_stealth_USB);
                }
                else if (VRPN_ISIT("vrpn_Tracker_Spin")) {
                  VRPN_CHECK(setup_Tracker_Spin);
                }
                else if (VRPN_ISIT("vrpn_Adafruit_10DOF")) {
                  VRPN_CHECK(setup_Adafruit_10DOF);
                }
                else if (VRPN_ISIT("vrpn_OzzMaker_BerryIMU")) {
                  VRPN_CHECK(setup_OzzMaker_BerryIMU);
                }
                else if (VRPN_ISIT("vrpn_Laputa")){
                    VRPN_CHECK(setup_Laputa);
                }
                else if (VRPN_ISIT("vrpn_Vality_vGlass")) {
                  VRPN_CHECK(setup_Vality_vGlass);
                }
                else {                         // Never heard of it
                    sscanf(line, "%511s", s1); // Find out the class name
                    fprintf(stderr, "vrpn_server: Unknown Device: %s\n", s1);
                    if (d_bail_on_open_error) {
                        d_doing_okay = false;
                        fclose(config_file);                                                   \
                        return;
                    }
                    else {
                        continue; // Skip this line
                    }
                }
            }
        }
    }

#undef VRPN_ISIT
#undef VRPN_CHECK

    // Restore the original settings into the global locale by
    // putting them into the global locale.
    std::locale::global(orig_locale);

    // Close the configuration file
    fclose(config_file);

#ifdef SGI_BDBOX
    fprintf(stderr, "sgibox: %p\n", vrpn_special_sgibox);
#endif
}

vrpn_Generic_Server_Object::~vrpn_Generic_Server_Object()
{
    closeDevices();
    delete _devices;
    _devices = NULL;
}

void vrpn_Generic_Server_Object::mainloop(void) { _devices->mainloop(); }
