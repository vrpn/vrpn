#if defined(_WIN32) && defined(VRPN_USE_DIRECTINPUT)
#ifndef VRPN_DIRECTXFFJOYSTICK_H
#define VRPN_DIRECTXFFJOYSTICK_H

#include <windows.h>
#include <basetsd.h>
#include <dinput.h>
#include "vrpn_Connection.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"

class vrpn_DirectXFFJoystick: public vrpn_Analog
			     ,public vrpn_Button
{
public:
    vrpn_DirectXFFJoystick (const char * name, vrpn_Connection * c,
      double readRate = 60, double forceRate = 200);

    ~vrpn_DirectXFFJoystick ();

    // Called once through each main loop iteration to handle
    // updates.
    virtual void mainloop ();

protected:
    HWND  _hWnd;	// Handle to the console window
    int	  _status;
    int	  _numbuttons;	// How many buttons
    int	  _numchannels;	// How many analog channels

    struct timeval _timestamp;	// Time of the last report from the device
    double _read_rate;		// How many times per second to read the device
    double _force_rate;		// How many times per second to update forces

    virtual int get_report(void);	// Try to read a report from the device
    void	clear_values(void);	// Clear the Analog and Button values

    // send report iff changed
    virtual void report_changes (vrpn_uint32 class_of_service
	      = vrpn_CONNECTION_LOW_LATENCY);
    // send report whether or not changed
    virtual void report (vrpn_uint32 class_of_service
	      = vrpn_CONNECTION_LOW_LATENCY);
    // NOTE:  class_of_service is only applied to vrpn_Analog
    //  values, not vrpn_Button

    HRESULT vrpn_DirectXFFJoystick::InitDirectJoystick( void );
    LPDIRECTINPUT8	  _DirectInput;   // Handle to Direct Input
    LPDIRECTINPUTDEVICE8  _Joystick;	  // Handle to the joystick we are using
    static  BOOL CALLBACK    EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* selfPtr );
    static  BOOL CALLBACK    EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* selfPtr );
};

#endif
#endif