#include "vrpn_DirectXRumblePad.h"

#if defined(_WIN32) && defined(VRPN_USE_DIRECTINPUT)
#include <stdio.h>

#undef DEBUG
#ifdef DEBUG
#define LOG(msg) send_text_message(msg);
#else
#define LOG (void)
#endif

#pragma warning(disable: 4244)

const char *CLASS_NAME = "vrpn_DirectXRumblePad";

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

const int MSG_POLL = WM_APP + 101;
const int MSG_SETMAGNITUDE = WM_APP + 102;

bool guid_equals(const GUID &a, const GUID &b) {
	return !memcmp(&a, &b, sizeof(GUID));
}

vrpn_DirectXRumblePad::vrpn_DirectXRumblePad(const char *name, vrpn_Connection *c,
							   GUID device_guid):
	vrpn_Analog(name, c),
	vrpn_Button(name, c),
        vrpn_Analog_Output(name, c),
	_target_device(device_guid),
	_gamepad(NULL),
	_directInput(NULL),
	_effect(NULL)
{
	vrpn_Analog::num_channel = 12;
	vrpn_Button::num_buttons = 128;
        vrpn_Analog_Output::o_num_channel = 1;

	// Register a handler for the request channel change message
	if (register_autodeleted_handler(request_m_id,
		handle_request_message, this, d_sender_id)) {
		fprintf(stderr,"vrpn_DirectXRumblePad: can't register change channel request handler\n");
		d_connection = NULL;
	}
	
	// Register a handler for the request channels change message
	if (register_autodeleted_handler(request_channels_m_id,
		handle_request_channels_message, this, d_sender_id)) {
		fprintf(stderr,"vrpn_DirectXRumblePad: can't register change channels request handler\n");
		d_connection = NULL;
	}

	if (register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_last_connection), handle_last_connection_dropped, this)) {
		FAIL("Can't register self-destruct handler");
		return;
	}

	_thread = CreateThread(NULL, 0, thread_proc, this, 0, NULL);
	if (_thread == INVALID_HANDLE_VALUE) {
		FAIL("Unable to create thread.");
		return;
	}
	LOG("Completed constructor.\n");
}

vrpn_DirectXRumblePad::~vrpn_DirectXRumblePad() {
	if (_wnd) {
		SendMessage(_wnd, WM_CLOSE, 0, 0);
	}
	if (_thread != INVALID_HANDLE_VALUE) {
		DWORD code;
		do {
			GetExitCodeThread(_thread, &code);
		} while (code == STILL_ACTIVE);
	}
}

BOOL CALLBACK vrpn_DirectXRumblePad::joystick_enum_cb(LPCDIDEVICEINSTANCE lpddi, LPVOID ref) {
	LOG("Checking a joystick.\n");

	vrpn_DirectXRumblePad *me = reinterpret_cast<vrpn_DirectXRumblePad *>(ref);

	if (guid_equals(me->_target_device, GUID_NULL)
	||  guid_equals(me->_target_device, lpddi->guidInstance)) {
		if SUCCEEDED(me->_directInput->CreateDevice(lpddi->guidInstance, &me->_gamepad, 0)) {
			return DIENUM_STOP;
		}
		else {
			FAIL(me,"Unable to create device instance! Attempting to find another one.");
			return DIENUM_CONTINUE;
		}
	}
	return DIENUM_CONTINUE;
}

DWORD CALLBACK vrpn_DirectXRumblePad::thread_proc(LPVOID ref) {
	vrpn_DirectXRumblePad *me = reinterpret_cast<vrpn_DirectXRumblePad *>(ref);
	
	WNDCLASS wc = {0};
	wc.lpfnWndProc = window_proc;
	wc.cbWndExtra = sizeof(vrpn_DirectXRumblePad *);
	wc.hInstance = HINST_THISCOMPONENT;
	wc.lpszClassName = CLASS_NAME;
	
	if (!RegisterClass(&wc)) {
		FAIL(me,"Unable to register class.");
		return 1;
	}

	me->_wnd = CreateWindow("STATIC", CLASS_NAME, WS_ICONIC, 0,0, 10,10, NULL, NULL, NULL, ref);

	if (!me->_wnd) {
		FAIL(me,"Unable to create window.");
		return 1;
	}

	if FAILED(DirectInput8Create(HINST_THISCOMPONENT, DIRECTINPUT_VERSION,
	IID_IDirectInput8, (void**)&me->_directInput, NULL)) {
		FAIL(me,"Unable to connect DirectInput.");
		return 1;
	}

	if FAILED(me->_directInput->EnumDevices(DI8DEVCLASS_GAMECTRL,
	joystick_enum_cb, me, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK)) {
		FAIL(me,"Unable to enumerate joysticks.");
		return 1;
	}

	if (!me->_gamepad) {
		FAIL(me,"No compatible joystick found!");
		return 1;
	}

	if FAILED(me->_gamepad->SetDataFormat(&c_dfDIJoystick2)) {
		FAIL(me,"Unable to set data format.");
		return 1;
	}

	if FAILED(me->_gamepad->SetCooperativeLevel(me->_wnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND)) {
		FAIL(me,"Unable to set cooperative level.");
		return 1;
	}

	if FAILED(me->_gamepad->EnumObjects(axis_enum_cb, me->_gamepad, DIDFT_AXIS)) {
		FAIL(me,"Unable to enumerate axes.");
		return 1;
	}

	if FAILED(me->init_force()) {
		FAIL(me,"Unable to initialize forces.");
		return 1;
	}

	if FAILED(me->_gamepad->Acquire()) {
		FAIL(me,"Unable to acquire gamepad.");
		return 1;
	}

	BOOL ret;
	MSG msg;
	while ((ret = GetMessage(&msg, me->_wnd, 0, 0)) != 0) {
		if (ret == -1) {
			FAIL(me,"GetMessage() threw an error.");
			return 1;
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (me->_effect)
		me->_effect->Release();
	if (me->_gamepad)
		me->_gamepad->Release();
	if (me->_directInput)
		me->_directInput->Release();

	LOG("Shutting down thread.\n");

	return 0;
}

LRESULT CALLBACK vrpn_DirectXRumblePad::window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
// Hack to make this compile under Visual Studio 6
#ifdef GetWindowLongPtr
	vrpn_DirectXRumblePad *me = reinterpret_cast<vrpn_DirectXRumblePad *>
		((LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA));
#else
	vrpn_DirectXRumblePad *me = static_cast<vrpn_DirectXRumblePad *>
		( (void*)(GetWindowLong(hwnd, GWL_USERDATA)));
#endif
	
	switch (msg) {
		case WM_CREATE: {
			CREATESTRUCT *s = reinterpret_cast<CREATESTRUCT *>(lp);
			me = reinterpret_cast<vrpn_DirectXRumblePad *>(s->lpCreateParams);
#ifdef SetWindowLongPtr
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me));
#else
			SetWindowLong(hwnd, GWL_USERDATA, (long)(me));
#endif
			return 0;
		}

		case WM_CLOSE:
			DestroyWindow(hwnd);
			PostQuitMessage(0);
			break;

		case MSG_POLL:
			if FAILED(me->_gamepad->Poll()) {
				do {
					me->_gamepad->Acquire();
				} while (me->_gamepad->Poll() == DIERR_INPUTLOST);
			}
			if SUCCEEDED(me->_gamepad->GetDeviceState(sizeof(DIJOYSTATE2), reinterpret_cast<DIJOYSTATE2 *>(lp))) {
				return 1701;
			}
			else {
				FAIL(me,"GetDeviceState() returned error result.");
				break;
			}

		case MSG_SETMAGNITUDE: {
			float mag = *reinterpret_cast<float*>(&wp);

			if (me->_effect)
				me->_effect->Stop();

			if (mag > 0) {
				HRESULT hr;

				me->diPeriodic.dwMagnitude = (DWORD) (DI_FFNOMINALMAX * mag);
				hr = me->_effect->SetParameters(&me->diEffect, DIEP_TYPESPECIFICPARAMS);
				hr = me->_effect->Download();
				hr = me->_effect->Start(1, 0);
				hr = hr;
			}
			break;
		}

		default:
			return DefWindowProc(hwnd, msg, wp, lp);
	}

	return 0;
}

BOOL CALLBACK vrpn_DirectXRumblePad::axis_enum_cb(LPCDIDEVICEOBJECTINSTANCE lpddoi, LPVOID ref) {

	LPDIRECTINPUTDEVICE8 gamepad = reinterpret_cast<LPDIRECTINPUTDEVICE8>(ref);

	DIPROPRANGE prop;
	prop.diph.dwSize = sizeof(DIPROPRANGE);
	prop.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	prop.diph.dwHow = DIPH_BYID;
	prop.diph.dwObj = lpddoi->dwType;
	prop.lMin = -1000;
	prop.lMax = 1000;

	if FAILED(gamepad->SetProperty(DIPROP_RANGE, &prop.diph)) {
		return DIENUM_STOP;
	}

	return DIENUM_CONTINUE;
}

void vrpn_DirectXRumblePad::mainloop() {
	static time_t last_error = time(NULL);
	DIJOYSTATE2 js;

	server_mainloop();

	if (SendMessage(_wnd, MSG_POLL, 0, reinterpret_cast<LPARAM>(&js)) != 1701) {
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

	for (i = 0; i < min(128,vrpn_BUTTON_MAX_BUTTONS); i++) {
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

    float mag = value;
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
    vrpn_float64  value;
    vrpn_unbuffer(&bufptr, &value);
    float mag = value;
    mag = (mag < 0) ? 0 : (mag > 1) ? 1 : mag;
    SendMessage(me->_wnd, MSG_SETMAGNITUDE, *reinterpret_cast<WPARAM*>(&mag), 0);
    
    return 0;
}

int VRPN_CALLBACK vrpn_DirectXRumblePad::handle_last_connection_dropped(void *selfPtr, vrpn_HANDLERPARAM data) {
	SendMessage(((vrpn_DirectXRumblePad *)selfPtr)->_wnd, MSG_SETMAGNITUDE, 0, 0);
	return 0;
}

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

	diPeriodic.dwMagnitude = DI_FFNOMINALMAX;
	diPeriodic.lOffset = 0;
	diPeriodic.dwPhase = 0;
	diPeriodic.dwPeriod = static_cast<DWORD>(0.05 * DI_SECONDS);

	diEffect.dwSize = sizeof(DIEFFECT);
	diEffect.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
	diEffect.dwDuration = INFINITE;
	diEffect.dwSamplePeriod = 0;
	diEffect.dwGain = DI_FFNOMINALMAX;
	diEffect.dwTriggerButton = DIEB_NOTRIGGER;
	diEffect.dwTriggerRepeatInterval = 0;
	diEffect.cAxes = 2;
	diEffect.rgdwAxes = dwAxes;
	diEffect.rglDirection = &lDirection[0];
	diEffect.lpEnvelope = NULL;
	diEffect.cbTypeSpecificParams = sizeof(diPeriodic);
	diEffect.lpvTypeSpecificParams = &diPeriodic;
	diEffect.dwStartDelay = 0;

	hr = _gamepad->CreateEffect(GUID_Square, &diEffect, &_effect, NULL);

	return hr;
}


#endif  // _WIN32 and VRPN_USE_DIRECTINPUT
