#ifndef VRPN_RUMBLEPAD_H

#include "vrpn_Configure.h"
#if defined(_WIN32) && defined(VRPN_USE_DIRECTINPUT)

#include "vrpn_Connection.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"
#include "vrpn_Analog_Output.h"

#include <windows.h>
// Included in vrpn_Configure.h #include <dinput.h>

// This implements a RumblePad, which has analog outputs and button outputs
// but also enables the user to set a rumble magnitude using an Analog_Output
// (channel zero controls the rumble magnitude).

class VRPN_API vrpn_DirectXRumblePad: public vrpn_Analog, public vrpn_Button, public vrpn_Analog_Output {
public:
	vrpn_DirectXRumblePad(const char *name, vrpn_Connection *c = NULL,
		GUID device_guid = GUID_NULL);

	~vrpn_DirectXRumblePad();

	virtual void mainloop();

private:
	static BOOL CALLBACK joystick_enum_cb(LPCDIDEVICEINSTANCE lpddi, LPVOID ref);
	static DWORD CALLBACK thread_proc(LPVOID ref);
	static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
	static BOOL CALLBACK axis_enum_cb(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID ref);

        // send report iff changed
        void report_changes (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
        // send report whether or not changed
        void report (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
        // NOTE:  class_of_service is only applied to vrpn_Analog
        //  values, not vrpn_Button

	HRESULT init_force();

	GUID _target_device;
	HWND _wnd;
	LPDIRECTINPUT8 _directInput;
	LPDIRECTINPUTDEVICE8 _gamepad;
	LPDIRECTINPUTEFFECT _effect;
	HANDLE _thread;
	timeval _timestamp;
	DIPERIODIC diPeriodic;
	DIEFFECT diEffect;

protected:
        // Handle the rumble-magnitude setting (channel 0).
        static int VRPN_CALLBACK handle_request_message( void *userdata,
                                          vrpn_HANDLERPARAM p );
        static int VRPN_CALLBACK handle_request_channels_message( void* userdata,
                                                   vrpn_HANDLERPARAM p);
	static int VRPN_CALLBACK handle_last_connection_dropped(void *selfPtr, vrpn_HANDLERPARAM data);

        void FAIL(const char *msg) { struct timeval now; vrpn_gettimeofday(&now, NULL); send_text_message(msg, now, vrpn_TEXT_ERROR); }
        static void FAIL(vrpn_DirectXRumblePad *obj, const char *msg) { struct timeval now; vrpn_gettimeofday(&now, NULL); obj->send_text_message(msg, now, vrpn_TEXT_ERROR); }
};

#endif  // _WIN32 and VRPN_USE_DIRECTINPUT

#define VRPN_RUMBLEPAD_H
#endif

