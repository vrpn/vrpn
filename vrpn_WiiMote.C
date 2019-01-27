// vrpn_WiiMote.C:  Drive for the WiiMote, based on the WiiUse library.
//
// Russ Taylor, revised December 2008

#include "vrpn_WiiMote.h"

#if defined(VRPN_USE_WIIUSE)
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include VRPN_WIIUSE_H

#include <string>

/// If we don't have this macro, we don't have any newer feature.
#ifndef WIIUSE_HAS_VERSION
#define WIIUSE_HAS_VERSION(major, minor, patch) false
#endif

// Opaque class to hold WiiMote device information so we don't have
// to include wiimote.h in the vrpn_WiiMote.h file.
class vrpn_Wiimote_Device {
	public:
		vrpn_Wiimote_Device() :
			device(NULL),
			which(0),
			useMS(0),
			useIR(0),
			reorderButtons(false),
			found(false),
			connected(false) {}
		struct wiimote_t *device;
		unsigned  which;
		unsigned  useMS;
		unsigned  useIR;
		bool reorderButtons;
		bool      found;
		bool      connected;
		std::string bdaddr;
};

// Helper routines.
#ifndef	min
#define min(x,y) ((x)<(y)?(x):(y))
#endif

#if defined (vrpn_THREADS_AVAILABLE)
struct vrpn_WiiMote_SharedData {
	vrpn_WiiMote_SharedData(vrpn_WiiMote* wm)
		: connectLock(), wmHandle(wm), msgLock(), stopFlag(false), running(false)
	{}
	vrpn_Semaphore connectLock;
	vrpn_WiiMote *wmHandle;
	vrpn_Semaphore msgLock;
	bool stopFlag;
	bool running;
};
#endif


inline void vrpn_WiiMote::acquireMessageLock() {
#if defined (vrpn_THREADS_AVAILABLE)
	// altering server state needs to be synced with server main loop!
	sharedData->msgLock.p();
#endif
}

inline void vrpn_WiiMote::releaseMessageLock() {
#if defined (vrpn_THREADS_AVAILABLE)
	sharedData->msgLock.v();
#endif
}

unsigned vrpn_WiiMote::map_button(unsigned btn) {
	switch (btn) {
		case 0: //WIIMOTE_BUTTON_TWO:
			return 2;
		case 1: //WIIMOTE_BUTTON_ONE:
			return 1;
		case 2: //WIIMOTE_BUTTON_B:
			return 4;
		case 3: //WIIMOTE_BUTTON_A:
			return 3;
		case 4: //WIIMOTE_BUTTON_MINUS:
			return 5;
		case 5: //WIIMOTE_BUTTON_ZACCEL_BIT6
			return 13;
		case 6: //WIIMOTE_BUTTON_ZACCEL_BIT7
			return 14;
		case 7: //WIIMOTE_BUTTON_HOME:
			return 0;
		case 8: //WIIMOTE_BUTTON_LEFT:
			return 7;
		case 9: //WIIMOTE_BUTTON_RIGHT:
			return 8;
		case 10: //WIIMOTE_BUTTON_DOWN:
			return 9;
		case 11: //WIIMOTE_BUTTON_UP:
			return 10;
		case 12: //WIIMOTE_BUTTON_PLUS:
			return 6;
		case 13: //WIIMOTE_BUTTON_ZACCEL_BIT4
			return 11;
		case 14: //WIIMOTE_BUTTON_ZACCEL_BIT5
			return 12;
		case 15: //WIIMOTE_BUTTON_UNKNOWN
			return 15;
		default:
#ifdef DEBUG
			fprintf(stderr, "WiiMote: unhandled button %d\n", btn);
#endif
			return btn;
	}
}

void vrpn_WiiMote::handle_event() {
	//-------------------------------------------------------------------------
	// Set the status of the buttons.  The first 16 buttons come from
	// the main contoller.  If there is an expansion controller plugged in,
	// then there is a second set of up to 16 buttons that can be read from it.
	unsigned i;
	for (i = 0; i < 16; i++) {
		if (wiimote->reorderButtons) {
			buttons[map_button(i)] = (wiimote->device->btns & (1 << i)) != 0;
		} else {
			buttons[i] = (wiimote->device->btns & (1 << i)) != 0;
		}
	}

	switch (wiimote->device->exp.type) {
		case EXP_NONE:
			// No expansion buttons to grab
			break;
		case EXP_NUNCHUK:
			for (i = 0; i < 16; i++) {
				buttons[16 + i] = (wiimote->device->exp.nunchuk.btns & (1 << i)) != 0;
			}
			break;

		case EXP_CLASSIC:
			for (i = 0; i < 16; i++) {
				buttons[32 + i] = (wiimote->device->exp.classic.btns & (1 << i)) != 0;
			}
			break;

		case EXP_GUITAR_HERO_3:
			for (i = 0; i < 16; i++) {
				buttons[48 + i] = (wiimote->device->exp.gh3.btns & (1 << i)) != 0;
			}
			break;
#ifdef EXP_WII_BOARD
		case EXP_WII_BOARD:
			// The balance board pretends to be its own wiimote, with only an "a" button
			// Use to calibrate, in a perfect world..
			if (IS_PRESSED(wiimote->device, WIIMOTE_BUTTON_A)) {
				wiiuse_set_wii_board_calib(wiimote->device);
			}
			break;
#endif
	}

	//-------------------------------------------------------------------------
	// Read the status of the analog values.  There are six of them for the
	// base unit and extras for expansion units.
	channel[0] = wiimote->device->battery_level;
	if (WIIUSE_USING_ACC(wiimote->device)) {
		channel[1] = wiimote->device->gforce.x;
		channel[2] = wiimote->device->gforce.y;
		channel[3] = wiimote->device->gforce.z;
	}
	if (WIIUSE_USING_IR(wiimote->device)) {
		unsigned dot;
		for (dot = 0; dot < 4; dot++) {
			if (wiimote->device->ir.dot[dot].visible) {
				channel[4 + 3 * dot + 0] = wiimote->device->ir.dot[dot].rx;
				channel[4 + 3 * dot + 1] = wiimote->device->ir.dot[dot].ry;
				channel[4 + 3 * dot + 2] = wiimote->device->ir.dot[dot].size;
			} else {
				channel[4 + 3 * dot + 0] = -1;
				channel[4 + 3 * dot + 1] = -1;
				channel[4 + 3 * dot + 2] = -1;
			}
		}
	}

	// See which secondary controller is installed and report
	switch (wiimote->device->exp.type) {
		case EXP_NONE:
			// No expansion analogs to grab
			break;

		case EXP_NUNCHUK:
			channel[16 + 0] = wiimote->device->exp.nunchuk.gforce.x;
			channel[16 + 1] = wiimote->device->exp.nunchuk.gforce.y;
			channel[16 + 2] = wiimote->device->exp.nunchuk.gforce.z;
			channel[16 + 3] = wiimote->device->exp.nunchuk.js.ang;
			channel[16 + 4] = wiimote->device->exp.nunchuk.js.mag;
#if WIIUSE_HAS_VERSION(0,14,2)
			channel[16 + 5] = wiimote->device->exp.nunchuk.js.x;
			channel[16 + 6] = wiimote->device->exp.nunchuk.js.y;
#endif
			break;

		case EXP_CLASSIC:
			channel[32 + 0] = wiimote->device->exp.classic.l_shoulder;
			channel[32 + 1] = wiimote->device->exp.classic.r_shoulder;
			channel[32 + 2] = wiimote->device->exp.classic.ljs.ang;
			channel[32 + 3] = wiimote->device->exp.classic.ljs.mag;
			channel[32 + 4] = wiimote->device->exp.classic.rjs.ang;
			channel[32 + 5] = wiimote->device->exp.classic.rjs.mag;
#if WIIUSE_HAS_VERSION(0,14,2)
			channel[32 + 6] = wiimote->device->exp.classic.ljs.x;
			channel[32 + 7] = wiimote->device->exp.classic.ljs.y;
			channel[32 + 8] = wiimote->device->exp.classic.rjs.x;
			channel[32 + 9] = wiimote->device->exp.classic.rjs.y;
#endif
			break;


		case EXP_GUITAR_HERO_3:
			channel[48 + 0] = wiimote->device->exp.gh3.whammy_bar;
			channel[48 + 1] = wiimote->device->exp.gh3.js.ang;
			channel[48 + 2] = wiimote->device->exp.gh3.js.mag;
#if WIIUSE_HAS_VERSION(0,14,2)
			channel[48 + 3] = wiimote->device->exp.gh3.js.x;
			channel[48 + 4] = wiimote->device->exp.gh3.js.y;
#endif
			break;

#ifdef EXP_WII_BOARD
		case EXP_WII_BOARD:
			{
				struct wii_board_t* wb = (wii_board_t*)&wiimote->device->exp.wb;
				float total = wb->tl + wb->tr + wb->bl + wb->br;
				float x = ((wb->tr + wb->br) / total) * 2 - 1;
				float y = ((wb->tl + wb->tr) / total) * 2 - 1;
				//printf("Got a wii board report: %f kg, @ (%f, %f)\n", total, x, y);
				//printf("Got a wii board report: %f, %f, %f, %f\n", wb->tl, wb->tr, wb->bl, wb->br);
				channel[64 + 0] = wb->tl;
				channel[64 + 1] = wb->tr;
				channel[64 + 2] = wb->bl;
				channel[64 + 3] = wb->br;
				channel[64 + 4] = total;
				channel[64 + 5] = x;
				channel[64 + 6] = y;
				break;
			}
#endif
		default:
			struct timeval now;
			vrpn_gettimeofday(&now, NULL);
			char msg[1024];
			sprintf(msg, "Unknown Wii Remote expansion type: device->exp.type = %d", wiimote->device->exp.type);
			send_text_message(msg, now, vrpn_TEXT_ERROR);
	}

	//-------------------------------------------------------------------------
	// Read the state of the Infrared sensors.
}

void vrpn_WiiMote::connect_wiimote(int timeout) {
	struct timeval now;
	char msg[1024];

	wiimote->device = NULL;
	unsigned num_available;
	num_available = wiiuse_find(available_wiimotes, VRPN_WIIUSE_MAX_WIIMOTES, timeout);
#ifdef __linux
	if (!wiimote->bdaddr.empty()) {
		for (int i = 0; i < VRPN_WIIUSE_MAX_WIIMOTES; ++i) {
			std::string current(available_wiimotes[i]->bdaddr_str);
			if (current == wiimote->bdaddr) {
				wiimote->device = available_wiimotes[i];
				break;
			} else if (!current.empty()) {
				acquireMessageLock();
				vrpn_gettimeofday(&now, NULL);
				sprintf(msg, "Wiimote found, but it's not the one we want: '%s' isn't '%s'\n", available_wiimotes[i]->bdaddr_str, wiimote->bdaddr.c_str());
				send_text_message(msg, now);
				releaseMessageLock();
			}
		}
	}
#endif
	if (wiimote->bdaddr.empty()) {
		wiimote->device = wiiuse_get_by_id(available_wiimotes, VRPN_WIIUSE_MAX_WIIMOTES, wiimote->which);
	}
	if (! wiimote->device) {
		acquireMessageLock();
		vrpn_gettimeofday(&now, NULL);
		sprintf(msg, "Could not open remote %d (%d found)", wiimote->which, num_available);
		send_text_message(msg, now, vrpn_TEXT_ERROR);
		releaseMessageLock();
		wiimote->found = false;
		return;
	} else {
		wiimote->found = true;
	}

	wiimote->connected = (wiiuse_connect(&(wiimote->device), 1) != 0);
	if (wiimote->connected) {
		acquireMessageLock();
		vrpn_gettimeofday(&now, NULL);
		sprintf(msg, "Connected to remote %d", wiimote->which);
		send_text_message(msg, now);
		releaseMessageLock();

		// rumble shortly to acknowledge connection:
		wiiuse_rumble(wiimote->device, 1);
		vrpn_SleepMsecs(200);
		initialize_wiimote_state();
	}
	else {
		acquireMessageLock();
		vrpn_gettimeofday(&now, NULL);
		sprintf(msg, "No connection to remote %d", wiimote->which);
		send_text_message(msg, now, vrpn_TEXT_ERROR);
		releaseMessageLock();
	}
}

void vrpn_WiiMote::initialize_wiimote_state(void) {
	if (!wiimote->device || !wiimote->found || !wiimote->connected) {
		return;
	}

	// Turn on a light so we know which device we are.
	switch (wiimote->which) {
		case 1:
			wiiuse_set_leds(wiimote->device, WIIMOTE_LED_1);
			break;
		case 2:
			wiiuse_set_leds(wiimote->device, WIIMOTE_LED_2);
			break;
		case 3:
			wiiuse_set_leds(wiimote->device, WIIMOTE_LED_3);
			break;
		case 4:
			wiiuse_set_leds(wiimote->device, WIIMOTE_LED_4);
			break;
		default:
			struct timeval now;
			vrpn_gettimeofday(&now, NULL);
			char msg[1024];
			sprintf(msg, "Too-large remote %d (1-4 available)", wiimote->which);
			send_text_message(msg, now, vrpn_TEXT_ERROR);
			break;
	}

	// Set motion sensing on or off
	wiiuse_motion_sensing(wiimote->device, wiimote->useMS);

	// Turn off rumbling
	wiiuse_rumble(wiimote->device, 0);

	// Set IR sensing on or off
	wiiuse_set_ir(wiimote->device, wiimote->useIR);
}

// Device constructor.
// Parameters:
// - name: VRPN name to assign to this server
// - c: VRPN connection this device should be attached to
vrpn_WiiMote::vrpn_WiiMote(const char *name, vrpn_Connection *c, unsigned which, unsigned useMS, unsigned useIR, unsigned reorderButtons, const char *bdaddr):
	vrpn_Analog(name, c),
	vrpn_Button_Filter(name, c),
	vrpn_Analog_Output(name, c),
#if defined (vrpn_THREADS_AVAILABLE)
	waitingForConnection(true),
	sharedData(0),
	connectThread(0),
#endif
	wiimote(NULL)
{
#ifndef vrpn_THREADS_AVAILABLE
	last_reconnect_attempt.tv_sec = 0;
	last_reconnect_attempt.tv_usec = 0;
#endif

        try { wiimote = new vrpn_Wiimote_Device; }
        catch (...) {
          FAIL("vrpn_WiiMote: out of memory");
          return;
        }

        int i;

	vrpn_Analog::num_channel = min(96, vrpn_CHANNEL_MAX);
	for (i = 0; i < vrpn_Analog::num_channel; i++) {
		channel[i] = 0;
	}

	// There are bits for up to 16 buttons on the main remote, and for
	// up to 16 more on an expansion pack.
	vrpn_Button::num_buttons = min(64, vrpn_BUTTON_MAX_BUTTONS);
	for (i = 0; i < vrpn_Button::num_buttons; i++) {
		buttons[i] = 0;
	}

	// Two channels: channel 0 is rumble, channel 1 is IR sensitivity.
	vrpn_Analog_Output::o_num_channel = 2;

	// Register a handler for the request channel change message
	if (register_autodeleted_handler(request_m_id,
	                                 handle_request_message, this, d_sender_id)) {
		FAIL("vrpn_WiiMote: can't register change channel request handler");
		return;
	}

	// Register a handler for the request channels change message
	if (register_autodeleted_handler(request_channels_m_id,
	                                 handle_request_channels_message, this, d_sender_id)) {
		FAIL("vrpn_WiiMote: can't register change channels request handler");
		return;
	}

	// Register a handler for the no-one's-connected-now message
	if (register_autodeleted_handler(
	            d_connection->register_message_type(vrpn_dropped_last_connection),
	            handle_last_connection_dropped, this)) {
		FAIL("Can't register self-destruct handler");
		return;
	}

	// Get a list of available devices and select the one we want.
	// Look for up to VRPN_WIIUSE_MAX_WIIMOTES motes.  Timeout in 5 seconds if one not found.
	wiimote->which = which > 0 ? which : 1;
	wiimote->useMS = useMS;
	wiimote->useIR = useIR;
	wiimote->reorderButtons = (reorderButtons != 0);
	if (bdaddr) {
		wiimote->bdaddr = std::string(bdaddr);
	}
#ifndef __linux
	if (!wiimote->bdaddr.empty()) {
		fprintf(stderr, "vrpn_WiiMote: Specifying the bluetooth address of the desired wiimote is only supported on Linux right now\n");
		wiimote->bdaddr.clear();
	}
#endif

	available_wiimotes = wiiuse_init(VRPN_WIIUSE_MAX_WIIMOTES);
#if !defined(DEBUG) && defined(WIIUSE_HAS_OUTPUT_REDIRECTION)
	/* disable debug and info messages from wiiuse itself */
	wiiuse_set_output(LOGLEVEL_DEBUG, 0);
	wiiuse_set_output(LOGLEVEL_INFO, 0);
#endif
#if defined  (vrpn_THREADS_AVAILABLE)
	// Pack the sharedData into another ThreadData
	// (this is an API flaw in vrpn_Thread)
	vrpn_ThreadData connectThreadData;
        try { sharedData = new vrpn_WiiMote_SharedData(this); }
        catch (...) {
          FAIL("vrpn_WiiMote: out of memory");
          return;
        }
	connectThreadData.pvUD = sharedData;
	// take ownership of msgLock:
	acquireMessageLock();
	// initialize connectThread:
        try { connectThread = new vrpn_Thread(&vrpn_WiiMote::connectThreadFunc, connectThreadData); }
        catch (...) {
          FAIL("vrpn_WiiMote: out of memory");
          return;
        }
        connectThread->go();
#else
	connect_wiimote(3);
#endif
}

// Device destructor
vrpn_WiiMote::~vrpn_WiiMote() {
#if defined (vrpn_THREADS_AVAILABLE)
	// stop connectThread
	sharedData->stopFlag = true;
	// Release the lock blocking the connection thread.
	if (!waitingForConnection) {
		sharedData->connectLock.v();
	}
	while (connectThread->running()) {
		// Let the connect thread send messages

		releaseMessageLock();
		vrpn_SleepMsecs(10);
		acquireMessageLock();
	}

        try {
          delete connectThread;
          delete sharedData;
        } catch (...) {
          fprintf(stderr, "vrpn_WiiMote::~vrpn_WiiMote(): delete failed\n");
          return;
        }
#endif
	// Close the device and delete

	if (wiimote->connected) {
		wiiuse_disconnect(wiimote->device);
	}
        try {
          delete wiimote;
        } catch (...) {
          fprintf(stderr, "vrpn_WiiMote::~vrpn_WiiMote(): delete failed\n");
          return;
        }
}

// VRPN main loop
// Poll the device and let the VRPN change notifications fire
void vrpn_WiiMote::mainloop() {

	vrpn_gettimeofday(&_timestamp, NULL);

#if defined (vrpn_THREADS_AVAILABLE)
	if (waitingForConnection) {
		// did connectThread establish a connection?
		if (sharedData->connectLock.condP()) {
			// yay, we can use wiimote again!
			waitingForConnection = false;
		} else {
			// allow connectThread to send its messages:
			releaseMessageLock();
			vrpn_SleepMsecs(10);
			acquireMessageLock();
			// still waiting
			// do housekeeping and return:
			server_mainloop();
			// no need to update timestamp?
			// no need to call report_changes?
			return;
		}
	}
#endif
	if (! isValid()) {
#if defined (vrpn_THREADS_AVAILABLE)
		waitingForConnection = true;
		// release semaphore, so connectThread gets active again:
		sharedData->connectLock.v();
#else // no thread support: we have to block the server:
		// try reconnect every second:
		timeval diff = vrpn_TimevalDiff(_timestamp, last_reconnect_attempt);
		if (diff.tv_sec >= 1) {
			last_reconnect_attempt = _timestamp;
			//reconnect
			// timeout=1 means that we block the vrpn server for a whole second!
			connect_wiimote(1);
		}
#endif
	}

	server_mainloop();

	// Poll to get the status of the device.  When an event happens, call
	// the appropriate handler to fill in our data structures.
	if (wiimote->connected && wiiuse_poll(&(wiimote->device), 1)) {
		switch (wiimote->device->event) {
			case WIIUSE_EVENT:  // A generic event
				handle_event();
				break;

			case WIIUSE_STATUS: // A status event
				// Nothing to do here, we're polling what we need to know in mainloop.
				break;

			case WIIUSE_DISCONNECT:
			case WIIUSE_UNEXPECTED_DISCONNECT:
				wiimote->connected = false;
				wiiuse_disconnect(wiimote->device);
				send_text_message("Disconnected", _timestamp, vrpn_TEXT_ERROR);
#ifndef vrpn_THREADS_AVAILABLE
				last_reconnect_attempt = _timestamp;
#endif
				break;

			case WIIUSE_READ_DATA:
				// Data we requested was returned.  Take a look at wiimote->device->read_req
				// for the info.
				break;

			case WIIUSE_NUNCHUK_INSERTED:
				send_text_message("Nunchuck inserted", _timestamp);
				break;

			case WIIUSE_CLASSIC_CTRL_INSERTED:
				send_text_message("Classic controller inserted", _timestamp);
				break;

			case WIIUSE_GUITAR_HERO_3_CTRL_INSERTED:
				send_text_message("Guitar Hero 3 controller inserted", _timestamp);
				break;

#ifdef EXP_WII_BOARD
			case WIIUSE_WII_BOARD_CTRL_INSERTED:
				send_text_message("Wii Balance Board controller inserted/detected", _timestamp);
				break;
#endif

			case WIIUSE_NUNCHUK_REMOVED:
			case WIIUSE_CLASSIC_CTRL_REMOVED:
			case WIIUSE_GUITAR_HERO_3_CTRL_REMOVED:
				send_text_message("An expansion controller was removed", _timestamp,
				                  vrpn_TEXT_WARNING);
				break;
#ifdef EXP_WII_BOARD
			case WIIUSE_WII_BOARD_CTRL_REMOVED:
				send_text_message("Wii Balance Board controller removed/disconnected", _timestamp,
				                  vrpn_TEXT_WARNING);
				break;
#endif

			default:
				send_text_message("unknown event", _timestamp);
				break;
		}
	}

	// Send any changes out over the connection.
	vrpn_gettimeofday(&_timestamp, NULL);
	report_changes();
}

bool vrpn_WiiMote::isValid() const {
	return (wiimote->found) && (wiimote->connected);
}

void vrpn_WiiMote::report(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report(class_of_service);
	vrpn_Button::report_changes();
}

void vrpn_WiiMote::report_changes(vrpn_uint32 class_of_service) {
	vrpn_Analog::timestamp = _timestamp;
	vrpn_Button::timestamp = _timestamp;

	vrpn_Analog::report_changes(class_of_service);
	vrpn_Button::report_changes();
}

/* static */
int vrpn_WiiMote::handle_request_message(void *userdata,
        vrpn_HANDLERPARAM p) {
	const char* bufptr = p.buffer;
	vrpn_int32 chan_num;
	vrpn_int32 pad;
	vrpn_float64 value;
	vrpn_WiiMote* me = (vrpn_WiiMote*)userdata;

	// Read the parameters from the buffer
	vrpn_unbuffer(&bufptr, &chan_num);
	vrpn_unbuffer(&bufptr, &pad);
	vrpn_unbuffer(&bufptr, &value);

	// Set the appropriate value, if the channel number is in the
	// range of the ones we have.
	if ((chan_num < 0) || (chan_num >= me->o_num_channel)) {
		fprintf(stderr, "vrpn_WiiMote::handle_request_message(): Index out of bounds\n");
		char msg[1024];
		sprintf(msg, "Error:  (handle_request_message):  channel %d is not active.  Squelching.", chan_num);
		me->send_text_message(msg, p.msg_time, vrpn_TEXT_ERROR);
		return 0;
	}
	me->o_channel[chan_num] = value;
	switch (chan_num) {
		case 0:
			{
				if (value >= 0.5) {
					wiiuse_rumble(me->wiimote->device, 1);
				} else {
					wiiuse_rumble(me->wiimote->device, 0);
				}
				break;
			}
		case 1:
			{
				int level = static_cast<int>(value);
				wiiuse_set_ir_sensitivity(me->wiimote->device, level);
				break;
			}
	}
	return 0;
}

/* static */
int vrpn_WiiMote::handle_request_channels_message(void* userdata,
        vrpn_HANDLERPARAM p) {
	const char* bufptr = p.buffer;
	vrpn_int32 num;
	vrpn_int32 pad;
	vrpn_WiiMote* me = (vrpn_WiiMote*)userdata;

	// Read the values from the buffer
	vrpn_unbuffer(&bufptr, &num);
	vrpn_unbuffer(&bufptr, &pad);
	if (num > me->o_num_channel) {
		char msg[1024];
		sprintf(msg, "Error:  (handle_request_channels_message):  channels above %d not active; "
		        "bad request up to channel %d.  Squelching.", me->o_num_channel, num);
		me->send_text_message(msg, p.msg_time, vrpn_TEXT_ERROR);
		num = me->o_num_channel;
	}
	if (num < 0) {
		char msg[1024];
		sprintf(msg, "Error:  (handle_request_channels_message):  invalid channel %d.  Squelching.", num);
		me->send_text_message(msg, p.msg_time, vrpn_TEXT_ERROR);
		return 0;
	}

	// Pull only channel 0 from the buffer, no matter how many values we received.
	vrpn_float64 value;
	vrpn_unbuffer(&bufptr, &value);
	if (value >= 0.5) {
		wiiuse_rumble(me->wiimote->device, 1);
	} else {
		wiiuse_rumble(me->wiimote->device, 0);
	}

	return 0;
}

int VRPN_CALLBACK vrpn_WiiMote::handle_last_connection_dropped(void *selfPtr, vrpn_HANDLERPARAM data) {
	//XXX Turn off any rumble.
	return 0;
}

#if defined (vrpn_THREADS_AVAILABLE)
/* static */
void vrpn_WiiMote::connectThreadFunc(vrpn_ThreadData &threadData) {
	vrpn_WiiMote_SharedData *sharedData = static_cast<vrpn_WiiMote_SharedData *>(threadData.pvUD);
	if (! sharedData || ! sharedData->wmHandle) {
		return;
	}
	while (true) {
		// wait for semaphore
		sharedData->connectLock.p();
		if (sharedData->stopFlag) {
			break;
		}
		sharedData->wmHandle->connect_wiimote(3);
		// release semaphore
		sharedData->connectLock.v();
		// make sure that main thread gets semaphore:
		vrpn_SleepMsecs(100);
	}
}
#endif

#endif  // VRPN_USE_WIIUSE
