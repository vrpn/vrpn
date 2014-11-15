
#include "vrpn_Joywin32.h"

#if defined(_WIN32)

#pragma comment(lib,"winmm.lib")
#include "vrpn_Shared.h"
#include <windows.h>
#include <stdio.h>
#include <cmath>
#include <algorithm>
// vrpn_Joywin32.C
//	This is a driver for joysticks being used through the
// Win32 basic interface.
//	The driver portions of this code are based on the Microsoft MSDN
// and tutorial from DigiBen at gametutorials.com

#undef VERBOSE

// Defines the modes in which the box can find itself.
const int STATUS_BROKEN	  = -1;	  // Broken joystick
const int STATUS_READING  =  1;	  // Looking for a report
const int CENTERED_VALUE  = -1;	  // returned value for centered POV

#define MAX_TIME_INTERVAL       (2000000) // max time to try and reacquire

vrpn_Joywin32::vrpn_Joywin32 (const char * name, vrpn_Connection * c, vrpn_uint8 joyNumber, vrpn_float64 readRate, vrpn_uint8 mode, vrpn_int32 deadzone) :
		vrpn_Analog(name, c),
		vrpn_Button_Filter(name, c),
		_read_rate(readRate),
		_joyNumber(joyNumber),
		_numchannels((std::min)(12,vrpn_CHANNEL_MAX)),		   // Maximum available
		_numbuttons((std::min)(128,vrpn_BUTTON_MAX_BUTTONS)),     // Maximum available
		_mode(mode)
{
	last_error_report.tv_sec = 0;
	last_error_report.tv_usec = 0;

	if (deadzone >100 || deadzone<0) {
		fprintf(stderr,"invalid deadzone, (should be a percentage between 0 and 100).\n");
		_status = STATUS_BROKEN;
        return;
	}

	if (_mode >2) {
		fprintf(stderr,"invalid mode, should be 0 (raw), 1 (normalized to 0,1) or 2 (normalized to -1,1).\n");
		_status = STATUS_BROKEN;
        return;
	}	

	if (_read_rate <0 ) {
		fprintf(stderr,"invalid read rate, should be positive.\n");
		_status = STATUS_BROKEN;
        return;
	}	
	
	if (_numchannels < 12) {
		fprintf(stderr,"vrpn_JoyWin32::vrpn_JoyWin32(): Not enough analog channels!\n");
		_status = STATUS_BROKEN;
		return;
	}

	_deadzone = deadzone / 100.0;

	// initialize the joystick
    init_joystick();
}

void vrpn_Joywin32::init_joystick(void)
{
    DWORD dwResult = 0;

	// This function below return the number of joysticks the driver supports.
	// If it returns 0 then there is no joystick driver installed.

	if (!joyGetNumDevs())
    {
		fprintf(stderr,"There are no joystick devices installed.\n");
		_status = STATUS_BROKEN;
        return;
    }

	if(_joyNumber > joyGetNumDevs()) {
		fprintf(stderr,"There are not %d joysticks devices installed, unable to get joystick id %d.\n", _joyNumber, _joyNumber);
		_status = STATUS_BROKEN;
        return;
	}

	// initialize requested joystick
	// polling every 10 ms.
	JOYINFO tempJoyInfo;
	dwResult = joyGetPos (_joyNumber-1, &tempJoyInfo);

	// Let's check what the return value was from joySetCapture()
	// - If the joystick is unplugged, say so.
	// - If there is no driver installed, say so
	// - If we can access the joystick for some strange reason, say so
	// Otherwise, let's return a SUCCESS!
	// You don't need to do this, but it helps the user solve the problem if there is one.
	// Error checking is VERY important, and employers want to see you use it.

	switch (dwResult) 
	{
		case JOYERR_UNPLUGGED:								// The joystick is unplugged
			fprintf(stderr, "Please plug in the joystick first.\n");
			_status = STATUS_BROKEN;
			return;
		case MMSYSERR_NODRIVER:								// There is no driver for a joystick
			fprintf(stderr, "No valid joystick driver.\n");
			_status = STATUS_BROKEN;
			return;									// Return failure
                case JOYERR_PARMS:
                        fprintf(stderr, "Bad joystick parameters.\n");
                        _status = STATUS_BROKEN;
                        return;	                                                                // return failure
		case JOYERR_NOCANDO:								// Unknown error, try restarting
			fprintf(stderr, "Couldn't capture joystick input, try restarting.\n");
			_status = STATUS_BROKEN;
			return;									// return failure
	}

	// get joystick caps
	if (joyGetDevCaps(_joyNumber-1, &_jc, sizeof(_jc)) != JOYERR_NOERROR){
		fprintf(stderr, "Unable to get joystick capabilities.\n");
		_status = STATUS_BROKEN;
		return;
	}

	/* set size of data, deprecated
	// _numchannels = _jc.wNumAxes;
	// _numbuttons = _jc.wNumButtons;
	
	if (_jc.wCaps & JOYCAPS_HASPOV) {
		_numchannels++;
	}*/

	vrpn_Analog::num_channel = _numchannels;
	vrpn_Button::num_buttons = _numbuttons;
  
	fprintf(stderr, "Joystick (%s) has %d axes and %d buttons.\n",
		_jc.szPname, _jc.wNumAxes+(JOYCAPS_HASPOV?1:0),  _jc.wNumButtons);

	// Set the mode to reading.  Set time to zero, so we'll try to read
	_status = STATUS_READING;
	vrpn_gettimeofday(&_timestamp, NULL);
}

void	vrpn_Joywin32::clear_values(void)
{
	vrpn_uint32	i;

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

int vrpn_Joywin32::get_report(void)
{
	// If it is not time for the next read, just return
	struct timeval reporttime;
	vrpn_gettimeofday(&reporttime, NULL);
	if (vrpn_TimevalDuration(reporttime, _timestamp) < 1000000.0 / _read_rate) {
		return 0;
	}
#ifdef	VERBOSE
	printf(" now: %ld:%ld,   last %ld:%ld\n", reporttime.tv_sec, reporttime.tv_usec,
		_timestamp.tv_sec, static_cast<long>(_timestamp.tv_usec));
	printf(" win32 joystick: Getting report\n");
#endif

	// update channels and buttons state :
	JOYINFOEX jie;
	jie.dwFlags = JOY_RETURNALL;//JOY_RETURNBUTTONS|JOY_RETURNCENTERED|JOY_RETURNPOVCTS|JOY_RETURNR|JOY_RETURNU|JOY_RETURNV|JOY_RETURNX|JOY_RETURNY|JOY_RETURNZ|JOY_USEDEADZONE;
	jie.dwSize = sizeof(jie);

	if (joyGetPosEx(_joyNumber-1, &jie) != JOYERR_NOERROR ){
		fprintf(stderr, "Unable to get joystick information.\n");
		// clear channels and buttons :
		clear_values();
		_status = STATUS_BROKEN;
		return -1;
	}

	// update vrpn_Channels state
	// all joysticks have x and y axes.
	
	vrpn_float64 normalizer;
	
	//*************** Retrieve information from JOYINFOEX about joystick state *****************
	channel[0] = jie.dwXpos;
	channel[1] = jie.dwYpos;

	if (_jc.wCaps & JOYCAPS_HASZ) {
		channel[2] = jie.dwZpos;
	}
	else channel[2] = 0;

	if (_jc.wCaps & JOYCAPS_HASR) {
		channel[3] = jie.dwRpos;
	}
	else channel[3] = 0;
	if (_jc.wCaps & JOYCAPS_HASU) {
		channel[4] = jie.dwUpos;
	}
	else channel[4] = 0;
	if (_jc.wCaps & JOYCAPS_HASV) {
		channel[5] = jie.dwVpos;
	}
	else channel[5] = 0;
	
	channel[6] = 0;
	channel[7] = 0;
	
	if (_jc.wCaps & JOYCAPS_HASPOV) {
		channel[8] = jie.dwPOV;
		if (channel[8] == 65535 || channel[8] == -1) {
			channel[8] = CENTERED_VALUE;
		}
		if (channel[8] != CENTERED_VALUE) {
			channel[8] /= 100.0;
		}
	}
	else {
		channel[8] = -1;
	}

	channel[9] = -1;
	channel[10] = -1;
	channel[11] = -1;

	//*********************** normalize from -1 to 1 *********************************

	if (_mode == 2) {
		normalizer = (_jc.wXmax - _jc.wXmin);
		if (normalizer != 0) channel[0] = (2*channel[0] - _jc.wXmax - _jc.wXmin) / (_jc.wXmax - _jc.wXmin);
		normalizer = (_jc.wYmax - _jc.wYmin);
		if (normalizer != 0) channel[1] = (2*channel[1] - _jc.wYmax - _jc.wYmin) / (_jc.wYmax - _jc.wYmin);
		if (_jc.wCaps & JOYCAPS_HASZ) {
			normalizer = (_jc.wZmax - _jc.wZmin);
			if (normalizer != 0) channel[2] = (2*channel[2] - _jc.wZmax - _jc.wZmin) / (_jc.wZmax - _jc.wZmin);
		}
		if (_jc.wCaps & JOYCAPS_HASR) {
			normalizer = (_jc.wRmax - _jc.wRmin);
			if (normalizer != 0) channel[3] = (2*channel[3] - _jc.wRmax - _jc.wRmin) / (_jc.wRmax - _jc.wRmin);
		}
		if (_jc.wCaps & JOYCAPS_HASU) {
			normalizer = (_jc.wUmax - _jc.wUmin);
			if (normalizer != 0) channel[4] = (2*channel[4] - _jc.wUmax - _jc.wUmin) / (_jc.wUmax - _jc.wUmin);
		}
		if (_jc.wCaps & JOYCAPS_HASV) {
			normalizer = (_jc.wVmax - _jc.wVmin);
			if (normalizer != 0) channel[5] = (2*channel[5] - _jc.wVmax - _jc.wVmin) / (_jc.wVmax - _jc.wVmin);
		}
	}
	//*********************** normalize from 0 to 1 *********************************
	else if (_mode == 1){
		normalizer = _jc.wXmax - _jc.wXmin;
		if (normalizer != 0) channel[0] = (channel[0] - _jc.wXmin) / normalizer;
		normalizer = _jc.wYmax - _jc.wYmin;
		if (normalizer != 0) channel[1] = (channel[1] - _jc.wYmin) / normalizer;
		if (_jc.wCaps & JOYCAPS_HASZ) {
			normalizer = _jc.wZmax - _jc.wZmin;
			if (normalizer != 0) channel[2] = (channel[2] - _jc.wZmin) / normalizer;
		}
		if (_jc.wCaps & JOYCAPS_HASR) {
			normalizer = _jc.wRmax - _jc.wRmin;
			if (normalizer != 0) channel[3] = (channel[3] - _jc.wRmin) / normalizer;
		}
		if (_jc.wCaps & JOYCAPS_HASU) {
			normalizer = _jc.wUmax - _jc.wUmin;
			if (normalizer != 0) channel[4] = (channel[4] - _jc.wUmin) / normalizer;
		}
		if (_jc.wCaps & JOYCAPS_HASV) {
			normalizer = _jc.wVmax - _jc.wVmin;
			if (normalizer != 0) channel[5] = (channel[5] - _jc.wVmin) / normalizer;
		}
	}

	// tidy up clipping
	if (_mode == 1){
		// clip to -1;1 + deadzone each channel which is not a POV
		for (vrpn_uint32 j=0;j<8;j++) {
			// clip to -1 ; 1
			if (channel[j] > 1.0) channel[j] = 1.0;
			else if (channel[j] < 0.0) channel[j] = 0.0;
		
			// clip with dead zone
			if (channel[j] <= _deadzone) {
				channel[j] = 0;
			}
			else {
				channel[j] = (channel[j] - _deadzone)/(1-_deadzone);
			}
		}
	}
	else if(_mode == 2) {
		// clip to -1;1 + deadzone each channel which is not a POV
		for (vrpn_uint32 j=0;j<8;j++) {
			// clip to -1 ; 1
			if (channel[j] > 1.0) channel[j] = 1.0;
			else if (channel[j] < -1.0) channel[j] = -1.0;
		
			// clip with dead zone
			if (channel[j] > -_deadzone && channel[j] < _deadzone) {
				channel[j] = 0;
			}
			else {
				if (channel[j]>0) {
					channel[j] = (channel[j] - _deadzone)/(1-_deadzone);
				}
				else {
					channel[j] = (channel[j] + _deadzone)/(1-_deadzone);
				}
			}
		}
	}

	// update vrpn_Buttons state
	for (vrpn_uint32 i=0;i<(std::min)(_jc.wMaxButtons, _numbuttons);i++) {
		// get flag for current button with a single bit mask and move it left to get 0x1 or 0x0 value
		buttons[i] = (char) ((jie.dwButtons&(1<<(i)))>>(i));
	}

	// Send any changes out over the connection.
	_timestamp = reporttime;
	report_changes();
	return 0;
}

void	vrpn_Joywin32::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

void	vrpn_Joywin32::report(vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}


// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the joystick,
// either trying to reset it or trying to get a reading from it.
void	vrpn_Joywin32::mainloop()
{
  // Call the generic server mainloop, since we are a server
  server_mainloop();
  
  switch(_status) {
    case STATUS_BROKEN:
	{
	  struct  timeval now;
	  vrpn_gettimeofday(&now, NULL);
	  if (vrpn_TimevalDuration(now, last_error_report) > MAX_TIME_INTERVAL) {
	    send_text_message("Cannot talk to joystick, trying resetting it", now, vrpn_TEXT_ERROR);
	    last_error_report = now;

        init_joystick();
	  }
	}
	break;

    case STATUS_READING:
	get_report();
	break;

    default:
	fprintf(stderr,"vrpn_Joywin32: Unknown mode (internal error)\n");
	break;
  }
}

#endif

