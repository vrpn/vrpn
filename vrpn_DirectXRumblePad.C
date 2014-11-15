// vrpn_DirectXRumblePad.C: This is another driver for force-feedback
// joysticks under Windows such as the Logitech Cordless RumblePad.
//
// Note that if you ARE using a Logitech gamepad, it is almost a
// certainty that this code will not work without Logitech's drivers
// installed. Without them you will not be able to find any available
// force feedback effects, though polling joystick axes and the POV hat
// should work fine.
//
// Chris VanderKnyff, revised July 2007

#include "vrpn_DirectXRumblePad.h"

#if defined(_WIN32) && defined(VRPN_USE_DIRECTINPUT)
#include <stdio.h>
#include <time.h> // for time
#include <algorithm> // for min
using std::min;

// Hacks to maintain VC++6 compatibility while preserving 64-bitness
// Also cleans up the annoying (and in this case meaningless) warnings under Win32
#ifdef _WIN64
# if !defined(GetWindowLongPtr) || !defined(SetWindowLongPtr)
#  error 64-bit compilation requires an SDK that supports LONG_PTRs.
# endif
#else
# ifndef LONG_PTR
#  define LONG_PTR LONG
# endif
# ifdef GetWindowLongPtr
#  undef GetWindowLongPtr
#  undef GWLP_USERDATA
# endif
# ifdef SetWindowLongPtr
#  undef SetWindowLongPtr
# endif
# define GetWindowLongPtr GetWindowLong
# define SetWindowLongPtr SetWindowLong
# define GWLP_USERDATA GWL_USERDATA
#endif
#ifndef HWND_MESSAGE
# define HWND_MESSAGE ((HWND) -3)
#endif

// __ImageBase allows us to get the HINSTANCE for the specific component
// we've been compiled into. This is actually the correct way for static
// libraries, because GetModuleHandle(NULL) returns the HINSTANCE for the
// hosting EXE. In the case where VRPN is a DLL itself or has been linked
// into a DLL, GetModuleHandle(NULL) is the wrong process's HINSTANCE.
//
// By using the pseudo-variable __ImageBase provided by the Visual C++
// compiler, we know exactly what our proper HINSTANCE is.
//
// More info: http://blogs.msdn.com/oldnewthing/archive/2004/10/25/247180.aspx
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

namespace {
	const char *CLASS_NAME = "vrpn_DirectXRumblePad"; // Win32 window class name
	const int MSG_POLL = WM_APP + 101;	// app-specific "polling" message
	const int MSG_SETMAGNITUDE = WM_APP + 102; // app-specific "set magnitude" message
	const int POLL_SUCCESS = 1701;	// private value to indicate polling success

	// Quick-and-dirty GUID comparison function
	// Arguments: Two Win32 GUID structures
	// Returns: true iff the two GUIDs are identical
	inline bool guid_equals(const GUID &a, const GUID &b) {
		return !memcmp(&a, &b, sizeof(GUID));
	}
}

// Device constructor.
// Parameters:
// - name: VRPN name to assign to this server
// - c: VRPN connection this device should be attached to
// - device_guid: If provided, the specific DirectInput device GUID we want to use
//   If not provided (or the null GUID), we'll bind to the first available joystick
vrpn_DirectXRumblePad::vrpn_DirectXRumblePad(const char *name, vrpn_Connection *c,
							   GUID device_guid)
  : vrpn_Analog(name, c)
  , vrpn_Button_Filter(name, c)
  , vrpn_Analog_Output(name, c)
  , _target_device(device_guid)
  , _wnd(NULL)
  , _thread(INVALID_HANDLE_VALUE)
  , _directInput(NULL)
  , _effect(NULL)
  , _gamepad(NULL)
{
	last_error = time(NULL);

	vrpn_Analog::num_channel = 12;
	vrpn_Button::num_buttons = min(128, vrpn_BUTTON_MAX_BUTTONS);
	vrpn_Analog_Output::o_num_channel = 1;

	// Register a handler for the request channel change message
	if (register_autodeleted_handler(request_m_id,
	  handle_request_message, this, d_sender_id)) {
		FAIL("vrpn_DirectXRumblePad: can't register change channel request handler");
		return;
	}

	// Register a handler for the request channels change message
	if (register_autodeleted_handler(request_channels_m_id,
	  handle_request_channels_message, this, d_sender_id)) {
		FAIL("vrpn_DirectXRumblePad: can't register change channels request handler");
		return;
	}

	// Register a handler for the no-one's-connected-now message
	if (register_autodeleted_handler(
	  d_connection->register_message_type(vrpn_dropped_last_connection), 
	  handle_last_connection_dropped, this)) {
		FAIL("Can't register self-destruct handler");
		return;
	}

	_thread = CreateThread(NULL, 0, thread_proc, this, 0, NULL);
	if (_thread == INVALID_HANDLE_VALUE) {
		FAIL("Unable to create thread.");
		return;
	}
}

// Device destructor
vrpn_DirectXRumblePad::~vrpn_DirectXRumblePad() {
	// Tell the thread window to close itself
	if (_wnd) {
		SendMessage(_wnd, WM_CLOSE, 0, 0);
	}

	// Wait for the worker thread to terminate
	// Spinlocks are suboptimal, but I know no easy way to "join" to a thread's termination.
	if (_thread != INVALID_HANDLE_VALUE) {
		DWORD code;
		do {
			GetExitCodeThread(_thread, &code);
		} while (code == STILL_ACTIVE);
	}
}

// joystick_enum_cb: Callback function for joystick enumeration
// Tries to find the desired device GUID if provided.
// Otherwise it will match on the first joystick enumerated.
BOOL CALLBACK vrpn_DirectXRumblePad::joystick_enum_cb(LPCDIDEVICEINSTANCE lpddi, LPVOID ref) {
	vrpn_DirectXRumblePad *me = reinterpret_cast<vrpn_DirectXRumblePad *>(ref);

	if (guid_equals(me->_target_device, GUID_NULL)
	||  guid_equals(me->_target_device, lpddi->guidInstance)) {
		if (SUCCEEDED(me->_directInput->CreateDevice(lpddi->guidInstance, &me->_gamepad, 0))) {
			return DIENUM_STOP;
		}
		else {
			me->FAIL("Unable to create device instance! Attempting to find another one.");
			return DIENUM_CONTINUE;
		}
	}
	return DIENUM_CONTINUE;
}

// Main thread procedure. Registers a window class, creates a worker window
// (so we have an HWND for DirectInput), and runs a message pump.
// We move all this onto another thread to prevent the host application
// from accidentally being flagged as a GUI thread. Enough DirectX things
// have thread affinity that it's safer to isolate things this way.
// All userland APIs are just wrappers around SendMessage to make sure all
// DirectX calls are executed in the context of the proper thread.
//
// LPVOID parameter is the generic "user data" passed to CreateThread API.
// In this case, it's our C++ this pointer--everything is static in the other thread.
// Multiple vrpn_DirectXRumblePads simply create multiple threads with their own local storage.
DWORD CALLBACK vrpn_DirectXRumblePad::thread_proc(LPVOID ref) {
	// Retrieve C++ this pointer
	vrpn_DirectXRumblePad *me = reinterpret_cast<vrpn_DirectXRumblePad *>(ref);
	
	// Register a simple window class
	WNDCLASS wc = {0};
	wc.lpfnWndProc = window_proc;
	wc.cbWndExtra = sizeof(vrpn_DirectXRumblePad *);
	wc.hInstance = HINST_THISCOMPONENT;
	wc.lpszClassName = CLASS_NAME;
	
	if (!RegisterClass(&wc)) {
		me->FAIL("Unable to register class.");
		return 1;
	}

	// Create our window
	me->_wnd = CreateWindow(CLASS_NAME, "VRPN Worker Thread", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, ref);

	if (!me->_wnd) {
		me->FAIL("Unable to create window.");
		return 1;
	}

	// Fire up DirectInput
	if (FAILED(DirectInput8Create(HINST_THISCOMPONENT, DIRECTINPUT_VERSION,
	IID_IDirectInput8, (void**)&me->_directInput, NULL))) {
		me->FAIL("Unable to connect DirectInput.");
		return 1;
	}

	// Enumerate force-feedback joysticks
	if (FAILED(me->_directInput->EnumDevices(DI8DEVCLASS_GAMECTRL,
	joystick_enum_cb, me, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK))) {
		me->FAIL("Unable to enumerate joysticks.");
		return 1;
	}

	// Make sure we found something
	if (!me->_gamepad) {
		me->FAIL("No compatible joystick found!");
		return 1;
	}

	// Set data format for retrieving axis values
	if (FAILED(me->_gamepad->SetDataFormat(&c_dfDIJoystick2))) {
		me->FAIL("Unable to set data format.");
		return 1;
	}

	// Grab exclusive access to the joystick
	if (FAILED(me->_gamepad->SetCooperativeLevel(me->_wnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND))) {
		me->FAIL("Unable to set cooperative level.");
		return 1;
	}

	// Find all the axes and set their ranges to [-1000, 1000]
	if (FAILED(me->_gamepad->EnumObjects(axis_enum_cb, me->_gamepad, DIDFT_AXIS))) {
		me->FAIL("Unable to enumerate axes.");
		return 1;
	}

	// Load the force effect onto the joystick
	if (FAILED(me->init_force())) {
		me->FAIL("Unable to initialize forces.");
		return 1;
	}

	// Acquire the joystick for polling
	if (FAILED(me->_gamepad->Acquire())) {
		me->FAIL("Unable to acquire joystick.");
		return 1;
	}

	// Start the main message loop
	BOOL ret;
	MSG msg;
	while ((ret = GetMessage(&msg, me->_wnd, 0, 0)) != 0) {
		if (ret == -1) {
			me->FAIL("GetMessage() threw an error.");
			return 1;
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	// Clean up after ourselves
	if (me->_effect)
		me->_effect->Release();
	if (me->_gamepad)
		me->_gamepad->Release();
	if (me->_directInput)
		me->_directInput->Release();

	return 0;
}

// Window procedure for the worker thread's helper window
// Standard WindowProc semantics for a message-only window
LRESULT CALLBACK vrpn_DirectXRumblePad::window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	// Retrieve "this" pointer
	// Guaranteed to be garbage until WM_CREATE finishes, but
	// we don't actually use this value until WM_CREATE writes a valid one
	vrpn_DirectXRumblePad *me = reinterpret_cast<vrpn_DirectXRumblePad *>
		(GetWindowLongPtr(hwnd, GWLP_USERDATA));
	
	switch (msg) {
		// Window is being created; store "this" pointer for future retrieval
		case WM_CREATE: {
			CREATESTRUCT *s = reinterpret_cast<CREATESTRUCT *>(lp);
			me = reinterpret_cast<vrpn_DirectXRumblePad *>(s->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me));
			return 0;
		}

		// Something (most likely ~vrpn_DirectXRumblePad) wants to close the window
		// Go ahead and signal shutdown
		case WM_CLOSE:
			DestroyWindow(hwnd);
			PostQuitMessage(0);
			break;

		// Main thread wants to poll the joystick. Do so.
		case MSG_POLL:
			if (FAILED(me->_gamepad->Poll())) {
				// Keep trying to acquire the joystick if necessary
				do {
					me->_gamepad->Acquire();
				} while (me->_gamepad->Poll() == DIERR_INPUTLOST);
			}
			// Read data from the joystick
			if (SUCCEEDED(me->_gamepad->GetDeviceState(sizeof(DIJOYSTATE2), reinterpret_cast<DIJOYSTATE2 *>(lp)))) {
				return POLL_SUCCESS; // Make sure main thread knows we're running here
			}
			else {
				me->FAIL("GetDeviceState() returned error result.");
				break;
			}

		// Main thread wants to change rumble magnitude. Do so.
		case MSG_SETMAGNITUDE: {
			float mag = *reinterpret_cast<float*>(&wp);

			if (me->_effect)
				me->_effect->Stop();

			if (mag > 0) {
				HRESULT hr;

				me->_diPeriodic.dwMagnitude = (DWORD) (DI_FFNOMINALMAX * mag);
				hr = me->_effect->SetParameters(&me->_diEffect, DIEP_TYPESPECIFICPARAMS);
				hr = me->_effect->Download();
				hr = me->_effect->Start(1, 0);
				hr = hr;
			}
			break;
		}

		// Everything not explicitly handled goes to DefWindowProc as per usual
		default:
			return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}

// Axis enumeration callback
// For each joystick axis we can find, tell it to report values in [-1000, 1000]
BOOL CALLBACK vrpn_DirectXRumblePad::axis_enum_cb(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID ref) {
	LPDIRECTINPUTDEVICE8 gamepad = reinterpret_cast<LPDIRECTINPUTDEVICE8>(ref);

	DIPROPRANGE prop;
	prop.diph.dwSize = sizeof(DIPROPRANGE);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwHow = DIPH_BYID;
	prop.diph.dwObj = lpddoi->dwType;
	prop.lMin = -1000;
	prop.lMax = 1000;

	if (FAILED(gamepad->SetProperty(DIPROP_RANGE, &prop.diph))) {
		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

// VRPN main loop
// Poll the joystick, update the axes, and let the VRPN change notifications fire
void vrpn_DirectXRumblePad::mainloop() {
	DIJOYSTATE2 js;

	server_mainloop();

	if (SendMessage(_wnd, MSG_POLL, 0, reinterpret_cast<LPARAM>(&js)) != POLL_SUCCESS) {
		if ((time(NULL) - last_error) > 2) {
			struct timeval now;

			time(&last_error);
			vrpn_gettimeofday(&now, NULL);
			send_text_message("Cannot talk to joystick", now, vrpn_TEXT_ERROR);
		}
		return;
	}

	channel[0] = js.lX / 1000.0;
	channel[1] = js.lY / 1000.0;
	channel[2] = js.lZ / 1000.0;

	channel[3] = js.lRx / 1000.0;
	channel[4] = js.lRy / 1000.0;
	channel[5] = js.lRz / 1000.0;

	channel[6] = js.rglSlider[0] / 1000.0;
	channel[7] = js.rglSlider[1] / 1000.0;

    int i;
	for (i = 0; i < 4; i++) {
		long v = (long) js.rgdwPOV[i];
		channel[8+i] = (v == -1) ? -1 : (v / 100.0);
	}

	for (i = 0; i < vrpn_Analog::num_channel/*min(128,vrpn_BUTTON_MAX_BUTTONS)*/; i++) {
		buttons[i] = ( (js.rgbButtons[i] & 0x80) != 0);
	}

	// Send any changes out over the connection.
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();
}

void vrpn_DirectXRumblePad::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

void vrpn_DirectXRumblePad::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

/* static */
int vrpn_DirectXRumblePad::handle_request_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 chan_num;
    vrpn_int32 pad;
    vrpn_float64 value;
    vrpn_DirectXRumblePad* me = (vrpn_DirectXRumblePad*)userdata;

    // Read the parameters from the buffer
    vrpn_unbuffer(&bufptr, &chan_num);
    vrpn_unbuffer(&bufptr, &pad);
    vrpn_unbuffer(&bufptr, &value);

    // Set the appropriate value, if the channel number is in the
    // range of the ones we have.
    if ( (chan_num < 0) || (chan_num >= me->o_num_channel) ) {
      fprintf(stderr,"vrpn_Analog_Output_Server::handle_request_message(): Index out of bounds\n");
      char msg[1024];
      sprintf( msg, "Error:  (handle_request_message):  channel %d is not active.  Squelching.", chan_num );
      me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
      return 0;
    }
    me->o_channel[chan_num] = value;

    float mag = static_cast<float>(value);
    mag = (mag < 0) ? 0 : (mag > 1) ? 1 : mag;
    SendMessage(me->_wnd, MSG_SETMAGNITUDE, *reinterpret_cast<WPARAM*>(&mag), 0);

    return 0;
}

/* static */
int vrpn_DirectXRumblePad::handle_request_channels_message(void* userdata,
    vrpn_HANDLERPARAM p)
{
    const char* bufptr = p.buffer;
    vrpn_int32 num;
    vrpn_int32 pad;
    vrpn_DirectXRumblePad* me = (vrpn_DirectXRumblePad*)userdata;

    // Read the values from the buffer
    vrpn_unbuffer(&bufptr, &num);
    vrpn_unbuffer(&bufptr, &pad);
    if (num > me->o_num_channel) 
    {
         char msg[1024];
         sprintf( msg, "Error:  (handle_request_channels_message):  channels above %d not active; "
              "bad request up to channel %d.  Squelching.", me->o_num_channel, num );
         me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
         num = me->o_num_channel;
    }
    if (num < 0) 
    {
         char msg[1024];
         sprintf( msg, "Error:  (handle_request_channels_message):  invalid channel %d.  Squelching.", num );
         me->send_text_message( msg, p.msg_time, vrpn_TEXT_ERROR );
         return 0;
    }

    // Pull only channel 0 from the buffer, no matter how many values we received.
    vrpn_float64 value;
    vrpn_unbuffer(&bufptr, &value);
    float mag = static_cast<float>(value);
    mag = (mag < 0) ? 0 : (mag > 1) ? 1 : mag;
    SendMessage(me->_wnd, MSG_SETMAGNITUDE, *reinterpret_cast<WPARAM*>(&mag), 0);
    
    return 0;
}

int VRPN_CALLBACK vrpn_DirectXRumblePad::handle_last_connection_dropped(void *selfPtr, vrpn_HANDLERPARAM data) {
	// Kill force feedback if no one is connected
	SendMessage(((vrpn_DirectXRumblePad *)selfPtr)->_wnd, MSG_SETMAGNITUDE, 0, 0);
	return 0;
}

// Initializes the joystick with a force feedback effect
HRESULT vrpn_DirectXRumblePad::init_force() {
	HRESULT hr;

	// Turn off autocentering
	DIPROPDWORD prop;
	prop.diph.dwSize = sizeof(prop);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwObj = 0;
	prop.diph.dwHow = DIPH_DEVICE;
	prop.dwData = DIPROPAUTOCENTER_OFF;

	hr = _gamepad->SetProperty(DIPROP_AUTOCENTER, &prop.diph);
	if (FAILED(hr))
		return hr;

	// Create the actual effect
	DWORD dwAxes[2] = {DIJOFS_X, DIJOFS_Y};
	LONG lDirection[2] = {0, 0};

	_diPeriodic.dwMagnitude = DI_FFNOMINALMAX;
	_diPeriodic.lOffset = 0;
	_diPeriodic.dwPhase = 0;
	_diPeriodic.dwPeriod = static_cast<DWORD>(0.05 * DI_SECONDS);

	_diEffect.dwSize = sizeof(DIEFFECT);
	_diEffect.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
	_diEffect.dwDuration = INFINITE;
	_diEffect.dwSamplePeriod = 0;
	_diEffect.dwGain = DI_FFNOMINALMAX;
	_diEffect.dwTriggerButton = DIEB_NOTRIGGER;
	_diEffect.dwTriggerRepeatInterval = 0;
	_diEffect.cAxes = 2;
	_diEffect.rgdwAxes = dwAxes;
	_diEffect.rglDirection = &lDirection[0];
	_diEffect.lpEnvelope = NULL;
	_diEffect.cbTypeSpecificParams = sizeof(_diPeriodic);
	_diEffect.lpvTypeSpecificParams = &_diPeriodic;
	_diEffect.dwStartDelay = 0;

	hr = _gamepad->CreateEffect(GUID_Square, &_diEffect, &_effect, NULL);

	return hr;
}


#endif  // _WIN32 and VRPN_USE_DIRECTINPUT
