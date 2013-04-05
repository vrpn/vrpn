#ifndef VRPN_GENERIC_SERVER_OBJECT_H
#define VRPN_GENERIC_SERVER_OBJECT_H

#include <stdio.h>                      // for FILE

#include "vrpn_Configure.h"             // for VRPN_USE_DEV_INPUT, etc
#include "vrpn_MainloopContainer.h"     // for vrpn_MainloopContainer
#include "vrpn_Types.h"                 // for vrpn_float64

class VRPN_API vrpn_Analog;
class VRPN_API vrpn_Analog_Output;
class VRPN_API vrpn_Auxiliary_Logger_Server_Generic;
class VRPN_API vrpn_Button;
class VRPN_API vrpn_CerealBox;
class VRPN_API vrpn_Connection;
class VRPN_API vrpn_DevInput;
class VRPN_API vrpn_Dial;
class VRPN_API vrpn_GlobalHapticsOrb;
class VRPN_API vrpn_Imager_Stream_Buffer;
class VRPN_API vrpn_ImmersionBox;
class VRPN_API vrpn_Keyboard;
class VRPN_API vrpn_Magellan;
class VRPN_API vrpn_Mouse;
class VRPN_API vrpn_PA_axis;
class VRPN_API vrpn_Poser;
class VRPN_API vrpn_Sound;
class VRPN_API vrpn_Spaceball;
class VRPN_API vrpn_TAF_axis;
class VRPN_API vrpn_Tng3;
class VRPN_API vrpn_Tracker;
class VRPN_API vrpn_Tracker_DTrack;
class VRPN_API vrpn_inertiamouse;
class VRPN_API vrpn_raw_SGIBox;
class VRPN_API vrpn_Timecode_Generator;
class VRPN_API vrpn_DirectXFFJoystick;
class VRPN_API vrpn_DirectXRumblePad;
class VRPN_API vrpn_XInputGamepad;
class VRPN_API vrpn_Joywin32;
class VRPN_API vrpn_Phantom;
class VRPN_API vrpn_Tracker_DTrack;
class VRPN_API vrpn_DevInput;
class VRPN_API vrpn_WiiMote;
class VRPN_API vrpn_Freespace;


const int VRPN_GSO_MAX_TRACKERS =             100;
const int VRPN_GSO_MAX_BUTTONS =              100;
const int VRPN_GSO_MAX_SOUNDS =               2;
const int VRPN_GSO_MAX_ANALOG =               8;
const int VRPN_GSO_MAX_ANALOGOUT =            8;
const int VRPN_GSO_MAX_SGIBOX =               2;
const int VRPN_GSO_MAX_CEREALS =              8;
const int VRPN_GSO_MAX_MAGELLANS =            8;
const int VRPN_GSO_MAX_MAGELLANSUSB =         8;
const int VRPN_GSO_MAX_SPACEBALLS =           8;
const int VRPN_GSO_MAX_IBOXES =               8;
const int VRPN_GSO_MAX_DIALS =                8;
const int VRPN_GSO_MAX_NDI_POLARIS_RIGIDBODIES = 20; //FIXME find out from the NDI specs if there is a maximum;
const int VRPN_GSO_MAX_TIMECODE_GENERATORS =  8;
const int VRPN_GSO_MAX_TNG3S =	              8;
const int VRPN_GSO_MAX_DIRECTXJOYS =          8;
const int VRPN_GSO_MAX_RUMBLEPADS =           8;
const int VRPN_GSO_MAX_XINPUTPADS =           8;
const int VRPN_GSO_MAX_WIN32JOYS =            100;
const int VRPN_GSO_MAX_GLOBALHAPTICSORBS =    8;
const int VRPN_GSO_MAX_PHANTOMS =             10;
const int VRPN_GSO_MAX_DTRACKS =              5;
const int VRPN_GSO_MAX_POSER =	              8;
const int VRPN_GSO_MAX_MOUSES =	              8;
const int VRPN_GSO_MAX_DEV_INPUTS =           16;
const int VRPN_GSO_MAX_KEYBOARD =             1;
const int VRPN_GSO_MAX_LOGGER =               10;
const int VRPN_GSO_MAX_IMAGE_STREAM =         10;
const int VRPN_GSO_MAX_WIIMOTES =             4;
const int VRPN_GSO_MAX_FREESPACES =           10;
const int VRPN_GSO_MAX_INERTIAMOUSES =        8;
const int VRPN_GSO_MAX_JSONNETS =             4;

class vrpn_Generic_Server_Object
{
  public:
    vrpn_Generic_Server_Object (vrpn_Connection *connection_to_use, const char *config_file_name = "vrpn.cfg", int port = vrpn_DEFAULT_LISTEN_PORT_NO, bool be_verbose = false, bool bail_on_open_error = false);
    ~vrpn_Generic_Server_Object();

    void mainloop (void);
    inline bool doing_okay (void) const {
      return d_doing_okay;
    }

  protected:
    vrpn_Connection *connection;          //< Connection to communicate on
    bool            d_doing_okay;         //< Is the object working okay?
    bool            verbose;              //< Should we print lots of info?
    bool            d_bail_on_open_error; //< Should we bail if we have an error opening a device?

    // Lists of devices
    vrpn_MainloopContainer _devices;
    vrpn_Tracker	* trackers [VRPN_GSO_MAX_TRACKERS];
    int		num_trackers;
    vrpn_Button	* buttons [VRPN_GSO_MAX_BUTTONS];
    int		num_buttons;
    vrpn_Sound	* sounds [VRPN_GSO_MAX_SOUNDS];
    int		num_sounds;
    vrpn_Analog	* analogs [VRPN_GSO_MAX_ANALOG];
    int		num_analogs;
    vrpn_raw_SGIBox	* sgiboxes [VRPN_GSO_MAX_SGIBOX];
    int		num_sgiboxes;
    vrpn_CerealBox	* cereals [VRPN_GSO_MAX_CEREALS];
    int		num_cereals;
    vrpn_Magellan	* magellans [VRPN_GSO_MAX_MAGELLANS];
    int		num_magellans;
    vrpn_Spaceball  * spaceballs [VRPN_GSO_MAX_SPACEBALLS];
    int             num_spaceballs;
    vrpn_ImmersionBox  *iboxes[VRPN_GSO_MAX_IBOXES];
    int             num_iboxes;
    vrpn_Dial	* dials [VRPN_GSO_MAX_DIALS];
    int		num_dials;
    vrpn_Timecode_Generator * timecode_generators[VRPN_GSO_MAX_TIMECODE_GENERATORS];
    int		num_generators;
    vrpn_Tng3       *tng3s[VRPN_GSO_MAX_TNG3S];
    int             num_tng3s;
    vrpn_DirectXFFJoystick   * DirectXJoys [VRPN_GSO_MAX_DIRECTXJOYS];
    int  num_DirectXJoys;
    vrpn_DirectXRumblePad    * RumblePads  [VRPN_GSO_MAX_RUMBLEPADS ];
    int  num_RumblePads;
    vrpn_XInputGamepad       * XInputPads  [VRPN_GSO_MAX_XINPUTPADS ];
    int  num_XInputPads;
    vrpn_Joywin32 *win32joys[VRPN_GSO_MAX_WIN32JOYS];
    int		num_Win32Joys;
    vrpn_GlobalHapticsOrb *ghos[VRPN_GSO_MAX_GLOBALHAPTICSORBS];
    int		num_GlobalHapticsOrbs;
    vrpn_Phantom	*phantoms[VRPN_GSO_MAX_PHANTOMS];
    int		num_phantoms;
    vrpn_Tracker_DTrack *DTracks[VRPN_GSO_MAX_DTRACKS];
    int num_DTracks;
    vrpn_Analog_Output	* analogouts [VRPN_GSO_MAX_ANALOG];
    int		num_analogouts;
    vrpn_Poser	* posers [VRPN_GSO_MAX_POSER];
    int		num_posers;
    vrpn_Mouse	* mouses [VRPN_GSO_MAX_MOUSES];
    int		num_mouses;
    vrpn_DevInput       * dev_inputs [VRPN_GSO_MAX_DEV_INPUTS];
    int		num_dev_inputs;
    vrpn_Keyboard * Keyboards [VRPN_GSO_MAX_KEYBOARD];
    int		num_Keyboards;
    vrpn_Auxiliary_Logger_Server_Generic * loggers [VRPN_GSO_MAX_LOGGER];
    int           num_loggers;
    vrpn_Imager_Stream_Buffer * imagestreams [VRPN_GSO_MAX_IMAGE_STREAM];
    int           num_imagestreams;
    vrpn_WiiMote  * wiimotes [VRPN_GSO_MAX_WIIMOTES];
    int           num_wiimotes;
    vrpn_Freespace  * freespaces [VRPN_GSO_MAX_FREESPACES];
    int           num_freespaces;
    vrpn_inertiamouse * inertiamouses [VRPN_GSO_MAX_INERTIAMOUSES];
    int             num_inertiamouses;

    void closeDevices (void);

    // Helper functions for the functions below
    int   get_AFline (char *line, vrpn_TAF_axis *axis);
    int	get_poser_axis_line (FILE *config_file, const char *axis_name, vrpn_PA_axis *axis, vrpn_float64 *min, vrpn_float64 *max);

    // Functions to parse each kind of device from the configuration file
    // and create a device of that type linked to the appropriate lists.
    int setup_raw_SGIBox (char * & pch, char * line, FILE * config_file);
    int setup_SGIBox (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_AnalogFly (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_ButtonFly (char * & pch, char * line, FILE * config_file);
    int setup_Joystick (char * & pch, char * line, FILE * config_file);
    int setup_Example_Button (char * & pch, char * line, FILE * config_file);
    int setup_Example_Dial (char * & pch, char * line, FILE * config_file);
    int setup_CerealBox (char * & pch, char * line, FILE * config_file);
    int setup_Magellan (char * & pch, char * line, FILE * config_file);
    int setup_Spaceball (char * & pch, char * line, FILE * config_file);
    int setup_Radamec_SPI (char * & pch, char * line, FILE * config_file);
    int setup_Zaber (char * & pch, char * line, FILE * config_file);
    int setup_BiosciencesTools (char * & pch, char * line, FILE * config_file);
    int setup_IDEA (char * & pch, char * line, FILE * config_file);
    int setup_NationalInstruments (char * & pch, char * line, FILE * config_file);
    int setup_NationalInstrumentsOutput (char * & pch, char * line, FILE * config_file);
    int setup_ImmersionBox (char * & pch, char * line, FILE * config_file);
    int setup_5dt (char * & pch, char * line, FILE * config_file);
    int setup_Wanda (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_Dyna (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_Fastrak (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_Isotrak (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_Liberty (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_LibertyHS (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_3Space (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_Flock (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_Flock_Parallel (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_NULL (char * & pch, char * line, FILE * config_file);
    int setup_Button_Python (char * & pch, char * line, FILE * config_file);
    int setup_Button_SerialMouse (char * & pch, char * line, FILE * config_file);
    int setup_Button_PinchGlove (char* &pch, char *line, FILE *config_file);
    int setup_Joylin (char * & pch, char * line, FILE * config_file);
    int setup_Joywin32 (char * & pch, char * line, FILE * config_file);
    int setup_Tng3 (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_InterSense (char * &pch, char *line, FILE * config_file);
    int setup_DirectXFFJoystick (char * & pch, char * line, FILE * config_file);
    int setup_RumblePad (char * & pch, char * line, FILE * config_file);
    int setup_XInputPad (char * & pch, char * line, FILE * config_file);
    int setup_GlobalHapticsOrb (char * & pch, char * line, FILE * config_file);
    int setup_ADBox (char* &pch, char *line, FILE *config_file);
    int setup_VPJoystick (char* &pch, char *line, FILE *config_file);
    int setup_DTrack (char* &pch, char* line, FILE* config_file);
    int setup_Poser_Analog (char * & pch, char * line, FILE * config_file);
    int setup_nikon_controls (char * & pch, char * line, FILE * config_file);
    int setup_Poser_Tek4662 (char * & pch, char * line, FILE * config_file);
    int setup_Timecode_Generator (char * & pch, char * line, FILE * config_file);
    int setup_Phantom (char * & pch, char * line, FILE * config_file);
    int setup_JoyFly (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_3DMouse (char * & pch, char * line, FILE * config_file);
    int setup_Mouse (char * & pch, char * line, FILE * config_file);
    int setup_DevInput (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_Crossbow (char * & pch, char * line, FILE * config_file);
    int setup_3DMicroscribe (char * & pch, char * line, FILE * config_file);
    int setup_5dt16 (char * & pch, char * line, FILE * config_file);
    int setup_Button_5DT_Server (char * & pch, char * line, FILE * config_file);
    int setup_Keyboard (char * & pch, char * line, FILE * config_file);
    int setup_Button_USB (char * & pch, char * line, FILE * config_file);
    int setup_Analog_USDigital_A2 (char * & pch, char * line, FILE * config_file) ;
    int setup_Button_NI_DIO24 (char * & pch, char * line, FILE * config_file) ;
    int setup_Tracker_PhaseSpace (char * & pch, char * line, FILE * config_file) ;
    int setup_Tracker_NDI_Polaris (char * & pch, char * line, FILE * config_file) ;
    int setup_Logger (char * & pch, char * line, FILE * config_file) ;
    int setup_ImageStream (char * & pch, char * line, FILE * config_file) ;
    int setup_Xkeys_Desktop (char * & pch, char * line, FILE * config_file) ;
    int setup_Xkeys_Pro (char * & pch, char * line, FILE * config_file) ;
    int setup_Xkeys_Joystick (char * & pch, char * line, FILE * config_file) ;
    int setup_Xkeys_Jog_And_Shuttle (char * & pch, char * line, FILE * config_file) ;
    int setup_Xkeys_XK3 (char * & pch, char * line, FILE * config_file) ;
    int setup_3DConnexion_Navigator (char * & pch, char * line, FILE * config_file) ;
    int setup_3DConnexion_Navigator_for_Notebooks (char * & pch, char * line, FILE * config_file) ;
    int setup_3DConnexion_Traveler (char * & pch, char * line, FILE * config_file) ;
    int setup_3DConnexion_SpaceMouse (char * & pch, char * line, FILE * config_file) ;
    int setup_3DConnexion_SpaceExplorer (char * & pch, char * line, FILE * config_file) ;
    int setup_3DConnexion_SpaceBall5000 (char * & pch, char * line, FILE * config_file) ;
    int setup_Tracker_MotionNode (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_GPS (char * & pch, char * line, FILE * config_file);
    int setup_WiiMote (char * & pch, char * line, FILE * config_file);
    int setup_SpacePoint (char * & pch, char * line, FILE * config_file);
    int setup_Wintracker (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_WiimoteHead (char * & pch, char * line, FILE * config_file);
    int setup_Freespace (char * & pch, char * line, FILE * config_file);
    int setup_DreamCheeky (char * & pch, char * line, FILE * config_file) ;
    int setup_Tracker_NovintFalcon (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_TrivisioColibri (char * &pch, char * line, FILE * config_file);
    int setup_Tracker_GameTrak (char *pch, char *line, FILE * config_file);
    int setup_LUDL_USBMAC6000 (char * &pch, char * line, FILE * config_file);
    int setup_Analog_5dtUSB_Glove5Left (char * &pch, char * line, FILE * config_file);
    int setup_Analog_5dtUSB_Glove5Right (char * &pch, char * line, FILE * config_file);
    int setup_Analog_5dtUSB_Glove14Left (char * &pch, char * line, FILE * config_file);
    int setup_Analog_5dtUSB_Glove14Right (char * &pch, char * line, FILE * config_file);
    int setup_Tracker_RazerHydra (char * &pch, char * line, FILE * config_file);
    int setup_Tracker_zSight (char * &pch, char * line, FILE * config_file);
    int setup_Tracker_ViewPoint (char * &pch, char * line, FILE * config_file);
    int setup_Atmel (char* &pch, char *line, FILE *config_file);
    int setup_Event_Mouse (char* &pch, char *line, FILE *config_file);
    int setup_inertiamouse (char * & pch, char * line, FILE * config_file);
    int setup_Tracker_G4 (char* &pch, char* line, FILE* config_file);
    int setup_Tracker_LibertyPDI (char* &pch, char* line, FILE* config_file);
    int setup_Tracker_FastrakPDI (char* &pch, char* line, FILE* config_file);
    int setup_Tracker_JsonNet (char* &pch, char* line, FILE* config_file);

};

#endif

