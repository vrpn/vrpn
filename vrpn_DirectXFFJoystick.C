#if defined(_WIN32) && defined(VRPN_USE_DIRECTINPUT)
// vrpn_DirectXFFJoystick.C
//	This is a driver for the BG Systems CerealBox controller.
// This box is a serial-line device that allows the user to connect
// analog inputs, digital inputs, digital outputs, and digital
// encoders and read from them over RS-232. You can find out more
// at www.bgsystems.com. This code is written for version 3.07 of
// the EEPROM code.
//	This code is based on their driver code, which was posted
// on their web site as of the summer of 1999. This code reads the
// characters as they arrive, rather than waiting "long enough" for
// them to get here; this should allow the CerealBox to be used at
// the same time as a tracking device without slowing the tracker
// down.
//	The driver portions of this code are based on the Microsoft
// DirectInput example code from the DirectX SDK.

#include "vrpn_DirectXFFJoystick.h"
#include "vrpn_Shared.h"

#undef	DEBUG
#undef VERBOSE

// Defines the modes in which the box can find itself.
#define	STATUS_BROKEN		(-1)	// Broken joystick
#define	STATUS_READING		(1)	// Looking for a report

#define MAX_TIME_INTERVAL       (2000000) // max time to try and reacquire

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

// XXX This is a horrible hack to get a handle to the console window for
// the running process.  This is needed to pass into the DirectInput
// routines.
static	HWND GetConsoleHwnd(void) {
  // Get the current window title.
  char pszOldWindowTitle[1024];
  GetConsoleTitle(pszOldWindowTitle, 1024);

  // Format a unique New Window Title.
  char pszNewWindowTitle[1024];
  wsprintf(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

  // Change the current window title.
  SetConsoleTitle(pszNewWindowTitle);

  // Ensure window title has changed.
  Sleep(40);

  // Look for the new window.
  HWND hWnd = FindWindow(NULL, pszNewWindowTitle);

  // Restore orignal name
  SetConsoleTitle(pszOldWindowTitle);

  return hWnd;
}

// This creates a vrpn_CerealBox and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
// The box seems to autodetect the baud rate when the "T" command is sent
// to it.
vrpn_DirectXFFJoystick::vrpn_DirectXFFJoystick (const char * name, vrpn_Connection * c,
						double readRate, double forceRate) :
		vrpn_Analog(name, c),
		vrpn_Button(name, c),
		_read_rate(readRate),
		_force_rate(forceRate),
		_DirectInput(NULL),
		_Joystick(NULL),
		_numchannels(min(12,vrpn_CHANNEL_MAX)),		   // Maximum available
		_numbuttons(min(128,vrpn_BUTTON_MAX_BUTTONS))      // Maximum available
{
  // Set the parameters in the parent classes
  vrpn_Button::num_buttons = _numbuttons;
  vrpn_Analog::num_channel = _numchannels;
  if (_numchannels < 12) {
    fprintf(stderr,"vrpn_DirectXFFJoystick::vrpn_DirectXFFJoystick(): Not enough analog channels!\n");
    _status = STATUS_BROKEN;
    _hWnd = NULL;
    return;
  }

  // Set the status of the buttons and analogs to 0 to start
  clear_values();

  // Initialize DirectInput and set the axes to return numbers in the range
  // -1000 to 1000.  We make the same mapping for analogs and sliders, so we
  // don't care how many this particular joystick has.  This enables users of
  // joysticks to keep the same mappings (but does not pass on the list of
  // what is actually intalled, unfortunately).
  _hWnd = GetConsoleHwnd();
  if( FAILED( InitDirectJoystick() ) ) {
    fprintf(stderr,"vrpn_DirectXFFJoystick::vrpn_DirectXFFJoystick(): Failed to open direct input\n");
    _status = STATUS_BROKEN;
    _hWnd = NULL;
    return;
  }

  // Set the mode to reading.  Set time to zero, so we'll try to read
  _status = STATUS_READING;
  gettimeofday(&_timestamp, NULL);
}

vrpn_DirectXFFJoystick::~vrpn_DirectXFFJoystick()
{
    // Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
  if( _Joystick ) {
    _Joystick->Unacquire();
    _Joystick->Release();
    _Joystick = NULL;
  }
    
  // Release any DirectInput objects.
  if ( _DirectInput ) {
    _DirectInput->Release();
    _DirectInput = NULL;
  }
}

HRESULT vrpn_DirectXFFJoystick::InitDirectJoystick( void )
{
    HWND    hWnd = GetConsoleHwnd();
    HRESULT hr;

    // Register with the DirectInput subsystem and get a pointer
    // to a IDirectInput interface we can use.
    // Create a DInput object
    if( FAILED( hr = DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, 
			   IID_IDirectInput8, (VOID**)&_DirectInput, NULL ) ) ) {
        fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Cannot open DirectInput\n");
	_status = STATUS_BROKEN;
        return hr;
    }

    // Look for a simple joystick we can use for this sample program.
    if( FAILED( hr = _DirectInput->EnumDevices( DI8DEVCLASS_GAMECTRL, 
                           EnumJoysticksCallback,
                           this, DIEDFL_ATTACHEDONLY ) ) ) {
        fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Cannot Enumerate devices\n");
	_status = STATUS_BROKEN;
        return hr;
    }

    // Make sure we got a joystick
    if( NULL == _Joystick ) {
	fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): No joystick found\n");
	_status = STATUS_BROKEN;
        return S_OK;
    }

    // Set the data format to "simple joystick" - a predefined data format 
    //
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    if( FAILED( hr = _Joystick->SetDataFormat( &c_dfDIJoystick2 ) ) ) {
        fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Cannot set data format\n");
	_status = STATUS_BROKEN;
        return hr;
    }

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
    if( FAILED( hr = _Joystick->SetCooperativeLevel( hWnd, DISCL_NONEXCLUSIVE | 
		     DISCL_BACKGROUND ) ) ) {
        fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Cannot set cooperative level\n");
	_status = STATUS_BROKEN;
        return hr;
    }

    // Enumerate the joystick objects. The callback function enabled user
    // interface elements for objects that are found, and sets the min/max
    // values property for discovered axes.
    if( FAILED( hr = _Joystick->EnumObjects( EnumObjectsCallback, this, DIDFT_ALL ) ) ) {
        fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Cannot enumerate objects\n");
	_status = STATUS_BROKEN;
        return hr;
    }

    // Acquire the joystick
    if( FAILED( hr = _Joystick->Acquire() ) ) {
      char  *reason;
      switch (hr) {
      case DIERR_INVALIDPARAM:
	reason = "Invalid parameter";
	break;
      case DIERR_NOTINITIALIZED:
	reason = "Not Initialized";
	break;
      case DIERR_OTHERAPPHASPRIO:
	reason = "Another application has priority";
	break;
      default:
	reason = "Unknown";
      }
      fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Cannot acquire joystick because %s\n", reason);
      _status = STATUS_BROKEN;
      return hr;
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// Desc: Called once for each enumerated joystick. If we find one, create a
//       device interface on it so we can play with it, then tell that we
//	 don't want to hear about any more.

BOOL CALLBACK vrpn_DirectXFFJoystick::EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance,
                                     VOID* selfPtr )
{
    vrpn_DirectXFFJoystick  *me = (vrpn_DirectXFFJoystick*)(selfPtr);
    HRESULT hr;

#ifdef	DEBUG
    printf("vrpn_DirectXFFJoystick::EnumJoysticksCallback(): Found joystick\n");
#endif

    // Obtain an interface to the enumerated joystick.
    hr = me->_DirectInput->CreateDevice( pdidInstance->guidInstance, &me->_Joystick, NULL );

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if( FAILED(hr) ) return DIENUM_CONTINUE;

    // Stop enumeration. Note: we're just taking the first joystick we get. You
    // could store all the enumerated joysticks and let the user pick.
    return DIENUM_STOP;
}

//-----------------------------------------------------------------------------
// Desc: Callback function for enumerating objects (axes, buttons, POVs) on a 
//       joystick. This function records how many there are and scales axes
//	 min/max values.
//-----------------------------------------------------------------------------
BOOL CALLBACK vrpn_DirectXFFJoystick::EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
                                   VOID* selfPtr )
{
    vrpn_DirectXFFJoystick  *me = (vrpn_DirectXFFJoystick*)(selfPtr);

    static int nSliderCount = 0;  // Number of returned slider controls
    static int nPOVCount = 0;     // Number of returned POV controls

#ifdef	DEBUG
    printf("vrpn_DirectXFFJoystick::EnumObjectsCallback(): Found type %d object\n", pdidoi->dwType);
#endif
    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values to -1000 to 1000.
    if( pdidoi->dwType & DIDFT_AXIS )
    {
        DIPROPRANGE diprg; 
        diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
        diprg.diph.dwHow        = DIPH_BYID; 
        diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin              = -1000; 
        diprg.lMax              = +1000; 
    
        // Set the range for the axis
        if( FAILED( me->_Joystick->SetProperty( DIPROP_RANGE, &diprg.diph ) ) ) 
            return DIENUM_STOP;
         
    }

    return DIENUM_CONTINUE;
}


void	vrpn_DirectXFFJoystick::clear_values(void)
{
	int	i;

	for (i = 0; i < _numbuttons; i++) {
		vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
	}
	for (i = 0; i < _numchannels; i++) {
		vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
	}
}

// This function will send a report if any of the analog or button values
// have changed.  This reads from the joystick _read_rate times per second, returns
// right away at other times.
// Returns 0 on success, -1 (and sets _status) on failure.

int vrpn_DirectXFFJoystick::get_report(void)
{
  HRESULT     hr;

  // If it has not been long enough, just return
  struct timeval reporttime;
  gettimeofday(&reporttime, NULL);
  if (duration(reporttime, _timestamp) < 1000000.0 / _read_rate) {
    return 0;
  }

#ifdef	DEBUG
  printf(" now: %ld:%ld,   last %ld:%ld\n", reporttime.tv_sec, reporttime.tv_usec,
    _timestamp.tv_sec, _timestamp.tv_usec);
  printf(" DirectX joystick: Getting report\n");
#endif
  // Poll the joystick.  If we can't poll it then try to reacquire
  // until that times out.
  hr = _Joystick->Poll(); 
  if( FAILED(hr) ) {
      // DInput is telling us that the input stream has been
      // interrupted. We aren't tracking any state between polls, so
      // we don't have any special reset that needs to be done. We
      // just re-acquire and try again.
      hr = _Joystick->Acquire();
      if ( hr == DIERR_INPUTLOST ) {
	  struct timeval resettime;
	  while ( ( hr == DIERR_INPUTLOST) && (duration(resettime, reporttime) <= MAX_TIME_INTERVAL) ) {
	    gettimeofday(&resettime, NULL);
	    hr = _Joystick->Acquire();
	  }
	  if (hr == DIERR_INPUTLOST) {
	    fprintf(stderr, "vrpn_DirectXFFJoystick::get_report::vrpn_DirectXFFJoystick::get_report(): Can't Acquire joystick\n");
	    _status = STATUS_BROKEN;
	    return -1;
	  }
	  reporttime = resettime;
      }

      // hr may be DIERR_OTHERAPPHASPRIO or other errors.  This
      // may occur when the app is minimized or in the process of 
      // switching, so just try again later
      fprintf(stderr, "Error other than INPUTLOST\n");
      return 0;
  }

  // Read the values from the joystick and put them into the internal structures.
  // Map the analogs representing sliders and axes to the range (-1...1).
  // Map the POVs to degrees by dividing by 100.
  DIJOYSTATE2 js;           // DInput joystick state 
  if( FAILED( hr = _Joystick->GetDeviceState( sizeof(DIJOYSTATE2), &js ) ) ) {
    fprintf(stderr, "vrpn_DirectXFFJoystick::get_report(): Can't read joystick\n");
    _status = STATUS_BROKEN;
    return -1;
  }
  channel[0] = js.lX / 1000.0;
  channel[1] = js.lY / 1000.0;
  channel[2] = js.lZ / 1000.0;

  channel[3] = js.lRx / 1000.0;
  channel[4] = js.lRy / 1000.0;
  channel[5] = js.lRz / 1000.0;

  channel[6] = js.rglSlider[0] / 1000.0;
  channel[7] = js.rglSlider[1] / 1000.0;

  channel[8] = (long)js.rgdwPOV[0] / 100.0;
  channel[9] = (long)js.rgdwPOV[1] / 100.0;
  channel[10] = (long)js.rgdwPOV[2] / 100.0;
  channel[11] = (long)js.rgdwPOV[3] / 100.0;

  int i;
  for (i = 0; i < min(128,vrpn_BUTTON_MAX_BUTTONS); i++) {
    buttons[i] = ( (js.rgbButtons[i] & 0x80) != 0);
  }

  // Send any changes out over the connection.
  _timestamp = reporttime;
  report_changes();
  return 0;
}

void	vrpn_DirectXFFJoystick::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

void	vrpn_DirectXFFJoystick::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the joystick,
// either trying to reset it or trying to get a reading from it.
void	vrpn_DirectXFFJoystick::mainloop()
{
  // Call the generic server mainloop, since we are a server
  server_mainloop();

  switch(_status) {
    case STATUS_BROKEN:
	{
	  static  struct  timeval last_report = {0,0};
	  struct  timeval now;
	  gettimeofday(&now, NULL);
	  if (duration(now, last_report) > MAX_TIME_INTERVAL) {
	    send_text_message("Cannot talk to joystick", now, vrpn_TEXT_ERROR);
	    last_report = now;
	  }
	}
	break;

    case STATUS_READING:
	get_report();
	break;

    default:
	fprintf(stderr,"vrpn_DirectXFFJoystick: Unknown mode (internal error)\n");
	break;
  }
}

#endif