#include "vrpn_DirectXFFJoystick.h"
#if defined(_WIN32) && defined(VRPN_USE_DIRECTINPUT)
#include <math.h>
#include <algorithm> // for min
using std::min;
// vrpn_DirectXFFJoystick.C
//	This is a driver for joysticks being used through the
// DirectX interface, both for input and for force feedback.
//	The driver portions of this code are based on the Microsoft
// DirectInput example code from the DirectX SDK.

#undef	DEBUG
#undef VERBOSE

// Defines the modes in which the box can find itself.
const int STATUS_BROKEN	  = -1;	  // Broken joystick
const int STATUS_READING  =  1;	  // Looking for a report

#define MAX_TIME_INTERVAL       (2000000) // max time to try and reacquire

// This creates a vrpn_CerealBox and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
// The box seems to autodetect the baud rate when the "T" command is sent
// to it.
vrpn_DirectXFFJoystick::vrpn_DirectXFFJoystick (const char * name, vrpn_Connection * c,
						double readRate, double forceRate) :
		vrpn_Analog(name, c),
		vrpn_Button_Filter(name, c),
		vrpn_ForceDeviceServer(name, c),
		_read_rate(readRate),
		_force_rate(forceRate),
		_DirectInput(NULL),
		_Joystick(NULL),
		_ForceEffect(NULL),
		_numchannels(min(12,vrpn_CHANNEL_MAX)),		   // Maximum available
		_numbuttons(min(128,vrpn_BUTTON_MAX_BUTTONS)),     // Maximum available
		_numforceaxes(0)				   // Filles in later.
{
  // Never yet sent forces.
  _forcetime.tv_sec = 0;
  _forcetime.tv_usec = 0;

  // Never sent a report
  _last_report.tv_sec = 0;
  _last_report.tv_usec = 0;

  // No nonzero previous forces.
  _fx_1 = _fy_1 = 0.0;
  _fx_2 = _fy_2 = 0.0;

  // In case we exit early for some reason.
  _status = STATUS_BROKEN;
  
  // Set the parameters in the parent classes
  vrpn_Button::num_buttons = _numbuttons;
  vrpn_Analog::num_channel = _numchannels;
  if (_numchannels < 12) {
    fprintf(stderr,"vrpn_DirectXFFJoystick::vrpn_DirectXFFJoystick(): Not enough analog channels!\n");
    _hWnd = NULL;
    return;
  }

  // Set the status of the buttons and analogs to 0 to start
  clear_values();

  // We need a non-console window handle to give to the function if we are
  // asking for exclusive access (I don't know why, but we do).
  _hWnd = CreateWindow("STATIC", "JoystickWindow", WS_ICONIC, 0,0, 10,10, NULL, NULL, NULL, NULL);

  // Initialize DirectInput and set the axes to return numbers in the range
  // -1000 to 1000.  We make the same mapping for analogs and sliders, so we
  // don't care how many this particular joystick has.  This enables users of
  // joysticks to keep the same mappings (but does not pass on the list of
  // what is actually intalled, unfortunately).
  // If we're using a forceDevice, then set it up as well.
#ifdef DEBUG
  printf("vrpn_DirectXFFJoystick::vrpn_DirectXFFJoystick(): Window handle %ld\n", _hWnd);
#endif
  if( FAILED( InitDirectJoystick() ) ) {
    fprintf(stderr,"vrpn_DirectXFFJoystick::vrpn_DirectXFFJoystick(): Failed to open direct joystick\n");
    _hWnd = NULL;
    return;
  }

  // Zero the forces on the device, if we have one.
  if (_force_rate > 0) {
    _fX = _fY = 0;
    send_normalized_force(0,0);
  }

  // Register an autodeleted handler on the "last connection dropped" system message
  // and have it call a routine that zeroes the forces when it is called.
  register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), handle_last_connection_dropped, this);

  // Register handlers for the force-feedback messages coming from the remote object.
  // XXX Eventually, fill out this list.  For now, we have only planes.
  if (_force_rate > 0) {
    if (register_autodeleted_handler(plane_message_id, 
	  handle_plane_change_message, this, vrpn_ForceDevice::d_sender_id)) {
	fprintf(stderr,"vrpn_DirectXFFJoystick:can't register plane handler\n");
	return;
    }
    if (register_autodeleted_handler(forcefield_message_id, 
	  handle_forcefield_change_message, this, vrpn_ForceDevice::d_sender_id)) {
	fprintf(stderr,"vrpn_DirectXFFJoystick:can't register force handler\n");
	return;
    }
  }

  // Set the mode to reading.  Set time to zero, so we'll try to read
  _status = STATUS_READING;
  vrpn_gettimeofday(&_timestamp, NULL);
}

vrpn_DirectXFFJoystick::~vrpn_DirectXFFJoystick()
{
  // Remove the ForceEffect if there is one
  if ( _ForceEffect ) {
    send_normalized_force(0,0);
    _ForceEffect->Release();
    _ForceEffect = NULL;
  }

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

    // Look for a simple joystick we can use for this sample program; if we want force feedback,
    // then also look for one that has this feature.
    long device_type = DIEDFL_ATTACHEDONLY;
    if (_force_rate > 0) { device_type |= DIEDFL_FORCEFEEDBACK; }
    if( FAILED( hr = _DirectInput->EnumDevices( DI8DEVCLASS_GAMECTRL, 
                           EnumJoysticksCallback,
                           this, device_type ) ) ) {
        fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Cannot Enumerate devices\n");
	_status = STATUS_BROKEN;
        return hr;
    }

    // Make sure we got a joystick of the type we wanted
    if( NULL == _Joystick ) {
	fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): No joystick found\n");
	_status = STATUS_BROKEN;
        return E_FAIL;
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
    // Exclusive access is required in order to perform force feedback.
    // Exclusive access is also required to keep other applications (like VMD)
    // from opening the same joystick, so we'll use it all the time.
    long access_type = DISCL_EXCLUSIVE | DISCL_BACKGROUND;
    if( FAILED( hr = _Joystick->SetCooperativeLevel( _hWnd, access_type) ) ) {
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
    if (_force_rate > 0) {
      if (_numforceaxes != 2) {
	fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Not two force axes, disabling forces\n");
	_force_rate = 0;
      } else {
#ifdef DEBUG
	printf("vrpn_DirectXFFJoystick::InitDirectJoystick(): found %d force axes\n", _numforceaxes);
#endif
	// Since we will be playing force feedback effects, we should disable the
	// auto-centering spring.
        DIPROPDWORD dipdw;
	dipdw.diph.dwSize       = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj        = 0;
	dipdw.diph.dwHow        = DIPH_DEVICE;
	dipdw.dwData            = FALSE;

#ifdef DEBUG
	printf("vrpn_DirectXFFJoystick::InitDirectJoystick(): disabling autocenter\n");
#endif
	if( FAILED( hr = _Joystick->SetProperty( DIPROP_AUTOCENTER, &dipdw.diph ) ) ) {
	  fprintf(stderr, "vrpn_DirectXFFJoystick::InitDirectJoystick(): Can't disable autocenter, disabling forces\n");
	  _force_rate = 0;
	} else {
	  // This application needs only one effect: Applying raw forces.
	  DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
	  LONG            rglDirection[2] = { 0, 0 };
	  DICONSTANTFORCE cf              = { 0 };

/*
	  DIENVELOPE diEnvelope;      // envelope
	  diEnvelope.dwSize = sizeof(DIENVELOPE);
	  diEnvelope.dwAttackLevel = 0; 
	  diEnvelope.dwAttackTime = (DWORD)(0.005 * DI_SECONDS); 
	  diEnvelope.dwFadeLevel = 0; 
	  diEnvelope.dwFadeTime = (DWORD)(0.005 * DI_SECONDS); 
*/

	  DIEFFECT eff;
	  ZeroMemory( &eff, sizeof(eff) );
	  eff.dwSize                  = sizeof(DIEFFECT);
	  eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
//	    eff.dwDuration              = INFINITE;
	  eff.dwDuration              = (DWORD)(0.02 * DI_SECONDS); 
	  eff.dwSamplePeriod          = 0;
	  eff.dwGain                  = DI_FFNOMINALMAX;
	  eff.dwTriggerButton         = DIEB_NOTRIGGER;
	  eff.dwTriggerRepeatInterval = 0;
	  eff.cAxes                   = _numforceaxes;
	  eff.rgdwAxes                = rgdwAxes;
	  eff.rglDirection            = rglDirection;
//      eff.lpEnvelope              = &diEnvelope;
	  eff.lpEnvelope              = 0;
	  eff.cbTypeSpecificParams    = sizeof(DICONSTANTFORCE);
	  eff.lpvTypeSpecificParams   = &cf;
	  eff.dwStartDelay            = 0;

	  // Create the prepared effect
	  if( FAILED( hr = _Joystick->CreateEffect( GUID_ConstantForce, &eff, &_ForceEffect, NULL ) ) ||
	      (_ForceEffect == NULL) ) {
	    fprintf(stderr,"vrpn_DirectXFFJoystick::InitDirectJoystick(): Can't create force effect, disabling forces\n");
	    _force_rate = 0;
	  }
	}
      }
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

#ifdef	DEBUG
    printf("vrpn_DirectXFFJoystick::EnumObjectsCallback(): Found type %d object\n", pdidoi->dwType);
#endif
    // For axes that are returned, set the DIPROP_RANGE property for the
    // enumerated axis in order to scale min/max values to -1000 to 1000.
    if (pdidoi->dwType & DIDFT_AXIS) {
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

    // For each force-feedback actuator returned, add one to the count.
    if (pdidoi->dwFlags & DIDOI_FFACTUATOR) {
      me->_numforceaxes++;
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

  // If it has been long enough, update the force sent to the user.
  {
    struct timeval now;
    vrpn_gettimeofday(&now, NULL);
    if (vrpn_TimevalDuration(now, _forcetime) >= 1000000.0 / _force_rate) {
      send_normalized_force(_fX, _fY);
      _forcetime = now;
    }
  }

  // If it is not time for the next read, just return
  struct timeval reporttime;
  vrpn_gettimeofday(&reporttime, NULL);
  if (vrpn_TimevalDuration(reporttime, _timestamp) < 1000000.0 / _read_rate) {
    return 0;
  }
#ifdef	VERBOSE
  printf(" now: %ld:%ld,   last %ld:%ld\n", reporttime.tv_sec, reporttime.tv_usec,
    _timestamp.tv_sec, static_cast<long>(_timestamp.tv_usec));
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
	  vrpn_gettimeofday(&resettime, NULL);
	  while ( ( hr == DIERR_INPUTLOST) && (vrpn_TimevalDuration(resettime, reporttime) <= MAX_TIME_INTERVAL) ) {
	    vrpn_gettimeofday(&resettime, NULL);
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

  // Send the new values out over the connection.
  _timestamp = reporttime;
  report();
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

// A force of 1 goes the the right in X and up in Y
void  vrpn_DirectXFFJoystick::send_normalized_force(double fx, double fy)
{
  // Make sure we have force capability.  If not, then set our status to
  // broken.
  if ( (_force_rate <= 0) || (_ForceEffect == NULL) ) {
    send_text_message("Asked to send force when no force enabled", _timestamp, vrpn_TEXT_ERROR);
    return;
  }

  // Set the forces to match a right-handed coordinate system
  fx *= -1;

  // If the total force vector is more than unit length, scale down by that
  // length.
  double len = sqrt(fx*fx + fy*fy);
  if (len > 1) {
    fx /= len;
    fy /= len;
  }

  // This version of the driver averages the last three force commands
  // before setting the force.

  double fx_avg = (fx + _fx_1 + _fx_2 )/3.0;
  double fy_avg = (fy + _fy_1 + _fy_2 )/3.0;

  // Convert the force from (-1..1) into the maximum range for each axis and then send it to
  // the device.
/*    INT xForce = (INT)(fx * DI_FFNOMINALMAX); */
/*    INT yForce = (INT)(fy * DI_FFNOMINALMAX); */

  INT xForce = (INT)(fx_avg * DI_FFNOMINALMAX);
  INT yForce = (INT)(fy_avg * DI_FFNOMINALMAX);

  _fx_2 = _fx_1;  _fy_2 = _fy_1;
  _fx_1 = fx;    _fy_1 = fy;

  LONG rglDirection[2];	  // Direction for the force (does not carry magnitude)
  DICONSTANTFORCE cf;	  // Magnitude of the force

  rglDirection[0] = xForce;
  rglDirection[1] = yForce;
  cf.lMagnitude = (DWORD)(sqrt( (double)xForce * (double)xForce +
				(double)yForce * (double)yForce ));

/*
  DIENVELOPE diEnvelope;      // envelope
  diEnvelope.dwSize = sizeof(DIENVELOPE);
  diEnvelope.dwAttackLevel = 0; 
  diEnvelope.dwAttackTime = (DWORD)(0.005 * DI_SECONDS); 
  diEnvelope.dwFadeLevel = 0; 
  diEnvelope.dwFadeTime = (DWORD)(0.005 * DI_SECONDS); 
*/

  DIEFFECT eff;
  ZeroMemory( &eff, sizeof(eff) );
  eff.dwSize                = sizeof(DIEFFECT);
  eff.dwFlags               = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
  eff.cAxes                 = _numforceaxes;
  eff.rglDirection          = rglDirection;
  eff.lpEnvelope            = 0;
//  eff.lpEnvelope            = &diEnvelope;
  eff.cbTypeSpecificParams  = sizeof(DICONSTANTFORCE);
  eff.lpvTypeSpecificParams = &cf;
  eff.dwStartDelay            = 0;

  // Now set the new parameters and start the effect immediately.
   if ( FAILED ( _ForceEffect->SetParameters( &eff, DIEP_DIRECTION |
					      DIEP_TYPESPECIFICPARAMS |
					      DIEP_START) ) ) {
    send_text_message("Can't send force", _timestamp, vrpn_TEXT_ERROR);
    return;
  }
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
	  struct  timeval now;
	  vrpn_gettimeofday(&now, NULL);
	  if (vrpn_TimevalDuration(now, _last_report) > MAX_TIME_INTERVAL) {
	    send_text_message("Cannot talk to joystick", now, vrpn_TEXT_ERROR);
	    _last_report = now;
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

/*XXX
// Set the force to match the X and Y components of the gradient of the plane.
int vrpn_DirectXFFJoystick::handle_plane_change_message(void *selfPtr, 
					      vrpn_HANDLERPARAM p)
{
  vrpn_DirectXFFJoystick *me = (vrpn_DirectXFFJoystick *)selfPtr;
  vrpn_float32	abcd[4];
  vrpn_float32	kspring, kdamp, fricdynamic, fricstatic;
  vrpn_int32	plane_index, plane_recovery_cycles;

  // XXX We are ignoring the plane index and treating it as if there is
  // only one plane.
  decode_plane(p.buffer, p.payload_len, abcd, 
	  &kspring, &kdamp, &fricdynamic, &fricstatic,
	  &plane_index, &plane_recovery_cycles);

  // If the plane normal is (0,0,0) this is a command to stop the surface
  if ( (abcd[0] == 0) && (abcd[1] == 0) && (abcd[2] == 0) ) {
    me->_fX = me->_fY = 0;
    return 0;
  }

  // Since the plane equation is (AX + BY + CZ + D = 0), the normalized A and B
  // coefficients determine the amount of force in X and Y.  We normalize by the
  // plane's direction vector not counting D (we send the force no matter where
  // we are with respect to the plane, since we are a 2D device in a 3D space).
  double  norm = sqrt(abcd[0]*abcd[0] + abcd[1]*abcd[1] + abcd[2]*abcd[2]);
  me->_fX = abcd[0] / norm;
  me->_fY = abcd[1] / norm;

  return 0;
}
XXX*/

// Margaret Minsky's dissertation suggests using the slope of the plane,
// which is the equivalent of the step size in Z for a unit step in X and
// a unit step in Y.  This turns out to be equivalent to A/C and B/C.
// Note that this can increase without bound, so we have to find some
// scaling factor and then clip if the step size in Z gets too large
// (clipping happens in the code that writes the value to the device
// when the length of the force vector exceeds 1).
int vrpn_DirectXFFJoystick::handle_plane_change_message(void *selfPtr, 
					      vrpn_HANDLERPARAM p)
{
  vrpn_DirectXFFJoystick *me = (vrpn_DirectXFFJoystick *)selfPtr;
  vrpn_float32	abcd[4];
  vrpn_float32	kspring, kdamp, fricdynamic, fricstatic;
  vrpn_int32	plane_index, plane_recovery_cycles;
  double	fscale = 0.25;	// Maximum force at four times slope for 45 degrees

  // XXX We are ignoring the plane index and treating it as if there is
  // only one plane.
  decode_plane(p.buffer, p.payload_len, abcd, 
	  &kspring, &kdamp, &fricdynamic, &fricstatic,
	  &plane_index, &plane_recovery_cycles);

  // If the plane normal is (0,0,0) this is a command to stop the surface
  if ( (abcd[0] == 0) && (abcd[1] == 0) && (abcd[2] == 0) ) {
    me->_fX = me->_fY = 0;
    return 0;
  }

  // If C is zero, then we set the forces to unit length in the direction of
  // the vector (A, B).  This preserves the direction of the force and makes it
  // be the maximum force (would be infinite if we divided by C).
  if (abcd[2] == 0) {
    double len = sqrt(abcd[0]*abcd[0] + abcd[1]*abcd[1]);
    me->_fX = abcd[0] / len;
    me->_fY = abcd[1] / len;

  // Since the plane equation is (AX + BY + CZ + D = 0), A/C and B/C
  // determine the amount of force in X and Y.  We do not count D
  // (we send the force no matter where we are with respect to the plane,
  // since we are a 2D device in a 3D space).
  } else {

    me->_fX = fscale * abcd[0] / abcd[2];
    me->_fY = fscale * abcd[1] / abcd[2];
  }

  return 0;
}

int vrpn_DirectXFFJoystick::handle_forcefield_change_message(void *selfPtr, 
					      vrpn_HANDLERPARAM p)
{
  vrpn_DirectXFFJoystick *me = (vrpn_DirectXFFJoystick *)selfPtr;

  vrpn_float32 center[3];
  vrpn_float32 force[3];
  vrpn_float32 jacobian[3][3];
  vrpn_float32 radius;

  decode_forcefield(p.buffer, p.payload_len, center, force, jacobian, &radius);

  // XXX We are ignoring the center, jacobian, and radius for now.  Just use the force.
  me->_fX = force[0];
  me->_fY = force[1];

  return 0;
}

// Zero the force sent to the device when the last connection is dropped.
int	vrpn_DirectXFFJoystick::handle_last_connection_dropped(void *selfPtr, vrpn_HANDLERPARAM)
{
  vrpn_DirectXFFJoystick  *me = (vrpn_DirectXFFJoystick*)selfPtr;
  if (me->_force_rate > 0) {
    me->_fX = me->_fY = 0;
    me->send_normalized_force(0,0);
  }
  return 0;
}


#endif
