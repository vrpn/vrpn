///////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:        vrpn_Tracker_zSight.C
//
// Authors:     David Borland
//              Josep Maria Tomas Sanahuja
//
//				EventLab at the University of Barcelona
//
// Description: VRPN tracker class for Sensics zSight HMD with built-in tracker.  The tracker
//              reports only orientation information, not position.  It is interfaced to as
//              a DirectX joystick, so VRPN_USE_DIRECTINPUT must be defined in 
//              vrpn_Configure.h to use it.
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include "vrpn_Tracker_zSight.h"

#if defined(_WIN32) && defined(VRPN_USE_DIRECTINPUT) && defined(VRPN_HAVE_ATLBASE)

#include <math.h>

// Convert from 2's complement, as per the Sensics zSight documentation
short FromTwos(unsigned short x) {
	if ( x < 0x8000 ) {
		return (short) x;
	}
	else {
		x = x ^ 0xFFFF;
		short y = (short) x;
		if (y != 32767) {
			y = y + 1;
		}
		y = -y;
		return (short) y;
	}
}

vrpn_Tracker_zSight::vrpn_Tracker_zSight(const char* name, vrpn_Connection* c) :
    vrpn_Tracker(name, c)
{
	// Create a window handle
	hWnd = CreateWindow("STATIC", "zSightWindow", WS_ICONIC, 0, 0, 10, 10, NULL, NULL, NULL, NULL);

    // Initialize DirectInput and acquire the device
    if (FAILED(InitDevice())) {
        fprintf(stderr,"vrpn_Tracker_zSight::vrpn_Tracker_zSight(): Failed to open device\n");
        hWnd = NULL;
        return;
    }
    
    // VRPN stuff
    register_server_handlers();
}

vrpn_Tracker_zSight::~vrpn_Tracker_zSight()
{
	// Release the Sensics device if necessary
	if (sensics) {
		sensics->Unacquire();
	}
}

void vrpn_Tracker_zSight::mainloop()
{
    // Call the generic server mainloop, since we are a server
    server_mainloop();

    // Get latest data
    get_report();
}

void vrpn_Tracker_zSight::get_report()
{
    // Get the current time
    vrpn_gettimeofday(&timestamp, NULL);

    if (hWnd) {
	    HRESULT hr;
	    DIJOYSTATE2 js;
	    if (FAILED(hr = sensics->GetDeviceState(sizeof(DIJOYSTATE2), &js))) {
		    fprintf(stderr, "vrpn_Tracker_zSight::get_report(): Can't read tracker\n");
		    return;
	    }

        // No need to fill in position, as we don´t get position information

	    // Get the orientation, as per the Sensics zSight documentation
	    float w = FromTwos((unsigned short) js.lRx) / 32768.0f;
	    float x = FromTwos((unsigned short) js.lRy) / 32768.0f;
	    float y = FromTwos((unsigned short) js.lX)  / 32768.0f;
	    float z = FromTwos((unsigned short) js.lY)  / 32768.0f;
	    float mag = sqrt(w*w + x*x + y*y + z*z);

	    // Set for internal quat, as per the Sensics zSight documentation
	    d_quat[3] = w / mag;
	    d_quat[0] = x / mag;
	    d_quat[1] = y / mag;
	    d_quat[2] = -z / mag;
    }

	// Send the data
	send_report();
}

void vrpn_Tracker_zSight::send_report()
{
    if (d_connection) {
        char msgbuf[1000];
        int len = encode_to(msgbuf);
        if (d_connection->pack_message(len, timestamp, position_m_id, d_sender_id, msgbuf, 
                                       vrpn_CONNECTION_LOW_LATENCY)) {
			fprintf(stderr,"vrpn_Tracker_zSight: cannot write message: tossing\n");
		}
    }
}

HRESULT vrpn_Tracker_zSight::InitDevice()
{
    HRESULT hr;

	// Register with the DirectInput subsystem and get a pointer to an IDirectInput interface we can use.
    if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL), 
									   DIRECTINPUT_VERSION, 
									   IID_IDirectInput8, 
									   (void**)&directInput, NULL))) {
        fprintf(stderr, "vrpn_Tracker_zSight::InitDevice(): Cannot open DirectInput\n");
        return hr;
    }

	// Initialize the zSight device, which is implemented as a DirectInput joystick
    if (FAILED(hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumSensicsCallback, this, DIEDFL_ATTACHEDONLY))) {
        fprintf(stderr, "vrpn_Tracker_zSight::InitDevice(): Cannot enumerate device\n");
        return hr;
    }

    // Make sure we got the device we wanted
	if (sensics == NULL) {
		fprintf(stderr, "vrpn_Tracker_zSight::InitDevice(): No Sensics zSight device found\n");
        return E_FAIL;
	}

    // Set the data format
	if (FAILED(hr = sensics->SetDataFormat(&c_dfDIJoystick2)))
	{
        fprintf(stderr, "vrpn_Tracker_zSight::InitDevice(): Cannot set data format\n");
		sensics->Unacquire();
        return hr;
	}
	
    // Set the cooperative level
	if (FAILED(hr = sensics->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND))) {
		fprintf(stderr, "vrpn_Tracker_zSight::InitDevice(): Cannot set cooperative level\n");
		sensics->Unacquire();
        return hr;
	}

    // Get the joystick axis object
	if (FAILED(hr = sensics->EnumObjects(EnumObjectsCallback, this, DIDFT_AXIS))) {
		fprintf(stderr, "vrpn_Tracker_zSight::InitDevice(): Cannot enumerate objects\n");
		sensics->Unacquire();
        return hr;
	}

    // Acquire the joystick
	if (FAILED(hr = sensics->Acquire())) {
		char  *reason;

		switch (hr) {

			case DIERR_INVALIDPARAM:
				reason = "Invalid parameter";
				break;

			case DIERR_NOTINITIALIZED:
				reason = "Not initialized";
				break;

			case DIERR_OTHERAPPHASPRIO:
				reason = "Another application has priority";
				break;

			default:
				reason = "Unknown";
      }

      fprintf(stderr, "vrpn_Tracker_zSight::InitDevice(): Cannot acquire device because %s\n", reason);
	  sensics->Unacquire();
      return hr;
    }

    return S_OK;
}

BOOL CALLBACK vrpn_Tracker_zSight::EnumSensicsCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* selfPtr)
{
	vrpn_Tracker_zSight* me = (vrpn_Tracker_zSight*)(selfPtr);
	HRESULT hr;

	// Obtain an interface to the tracker
	hr = me->directInput->CreateDevice(pdidInstance->guidInstance, &me->sensics, NULL);

	// Make sure it is a Sensics zSight device
	if (SUCCEEDED(hr) == TRUE) {
		if (strcmp(pdidInstance->tszProductName, "Sensics zSight HMD") != 0) {
			me->sensics->Unacquire();
		}
		else {
			return DIENUM_STOP;
		}
	}

	return DIENUM_CONTINUE;
}

BOOL CALLBACK vrpn_Tracker_zSight::EnumObjectsCallback(const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* selfPtr)
{
	vrpn_Tracker_zSight* me = (vrpn_Tracker_zSight*)(selfPtr);

	if (pdidoi->dwType & DIDFT_AXIS) {
		DIPROPRANGE diprg;
		::memset(&diprg, 0, sizeof(DIPROPRANGE));
		diprg.diph.dwSize = sizeof(DIPROPRANGE);
		diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		diprg.diph.dwHow = DIPH_BYID;
		diprg.diph.dwObj = pdidoi->dwType; // Specify the enumerated axis
		diprg.lMin = 0;
		diprg.lMax = 65535;

		// Set the range for the axis
		if (FAILED(me->sensics->SetProperty(DIPROP_RANGE, &diprg.diph))) {
			fprintf(stderr, "vrpn_Tracker_zSight::vrpn_Tracker_zSight(): Cannot set data range\n");
			return DIENUM_STOP;
		}
	}

	return DIENUM_CONTINUE;
}

#endif
