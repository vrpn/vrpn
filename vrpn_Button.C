#include <stdio.h> // for fprintf, stderr, NULL, etc
#ifndef _WIN32_WCE
#include <fcntl.h> // for open, O_RDWR
#endif
#include <string.h>

#if defined(linux) && !defined(VRPN_CLIENT_ONLY) && !defined(__ANDROID__)
#define VRPN_USE_LINUX_PARALLEL_SUPPORT
#include <linux/lp.h>  // for LPGETSTATUS
#include <sys/ioctl.h> // for ioctl
#endif

#include "vrpn_Connection.h" // for vrpn_Connection, etc
#ifndef VRPN_CLIENT_ONLY
#include "vrpn_Serial.h"
#endif
// Include vrpn_Shared.h _first_ to avoid conflicts with sys/time.h
// and unistd.h
#include "vrpn_Shared.h" // for timeval, vrpn_buffer, etc

#ifdef linux
#include <unistd.h>
#endif
#if defined(_WIN32) && !defined(__CYGWIN__) && !defined(_WIN32_WCE)
#include <conio.h> // for _inp()
#endif

#if (defined(sgi) && (!defined(VRPN_CLIENT_ONLY)))
#include <gl/device.h>
// need these for the gl based setdblight calls
#include <gl/gl.h>
#endif

#include "vrpn_Button.h"

#define BUTTON_READY (1)
#define BUTTON_FAIL (-1)

// Bits in the status register
#ifndef VRPN_CLIENT_ONLY
static const unsigned char PORT_ERROR = (1 << 3);
static const unsigned char PORT_SLCT = (1 << 4);
static const unsigned char PORT_PE = (1 << 5);
static const unsigned char PORT_ACK = (1 << 6);
static const unsigned char PORT_BUSY = (1 << 7);
static const unsigned char BIT_MASK =
    PORT_ERROR | PORT_SLCT | PORT_PE | PORT_ACK | PORT_BUSY;

// message constants for the PinchGlove code.
const unsigned char PG_START_BYTE_DATA(0x80);
const unsigned char PG_START_BYTE_DATA_TIME(0x81);
//const unsigned char PG_START_BYTE_TEXT(0x82);
const unsigned char PG_END_BYTE(0x8F);
#endif

static int VRPN_CALLBACK
client_msg_handler(void *userdata, vrpn_HANDLERPARAM p);

#define PACK_ADMIN_MESSAGE(i, event)                                           \
    {                                                                          \
        char msgbuf[1000];                                                     \
        vrpn_int32 len = encode_to(msgbuf, i, event);                          \
        if (d_connection->pack_message(len, timestamp, admin_message_id,       \
                                       d_sender_id, msgbuf,                    \
                                       vrpn_CONNECTION_RELIABLE)) {            \
            fprintf(stderr, "vrpn_Button: can't write message: tossing\n");    \
        }                                                                      \
    }
#define PACK_ALERT_MESSAGE(i, event)                                           \
    {                                                                          \
        char msgbuf[1000];                                                     \
        vrpn_int32 len = encode_to(msgbuf, i, event);                          \
        if (d_connection->pack_message(len, timestamp, alert_message_id,       \
                                       d_sender_id, msgbuf,                    \
                                       vrpn_CONNECTION_RELIABLE)) {            \
            fprintf(stderr, "vrpn_Button: can't write message: tossing\n");    \
        }                                                                      \
    }

#define PACK_MESSAGE(i, event)                                                 \
    {                                                                          \
        char msgbuf[1000];                                                     \
        vrpn_int32 len = encode_to(msgbuf, i, event);                          \
        if (d_connection->pack_message(len, timestamp, change_message_id,      \
                                       d_sender_id, msgbuf,                    \
                                       vrpn_CONNECTION_RELIABLE)) {            \
            fprintf(stderr, "vrpn_Button: can't write message: tossing\n");    \
        }                                                                      \
    }

vrpn_Button::vrpn_Button(const char *name, vrpn_Connection *c)
    : vrpn_BaseClass(name, c)
    , num_buttons(0)
{
    vrpn_BaseClass::init();

    // Set the time to 0 just to have something there.
    timestamp.tv_usec = timestamp.tv_sec = 0;
    for (vrpn_int32 i = 0; i < vrpn_BUTTON_MAX_BUTTONS; i++) {
        buttons[i] = lastbuttons[i] = 0;
    }
}

int vrpn_Button::register_types(void)
{
    // used to handle button strikes
    change_message_id =
        d_connection->register_message_type("vrpn_Button Change");

    // used to handle button states reports
    states_message_id =
        d_connection->register_message_type("vrpn_Button States");

    // to handle button state changes -- see Button_Filter should register a
    // handler
    // for this ID -- ideally the message will be ignored otherwise
    admin_message_id = d_connection->register_message_type("vrpn_Button Admin");

    return 0;
}

// virtual
vrpn_Button::~vrpn_Button(void)
{

    // do nothing
}

vrpn_Button_Filter::vrpn_Button_Filter(const char *name, vrpn_Connection *c)
    : vrpn_Button(name, c)
{
    if ((d_sender_id == -1) || (admin_message_id == -1)) {
        fprintf(stderr, "vrpn_Button: Can't register IDs\n");
        d_connection = NULL;
    }
    register_autodeleted_handler(admin_message_id, client_msg_handler, this);

    // setup message id type for alert messages to alert a device about changes
    alert_message_id = d_connection->register_message_type("vrpn_Button Alert");
    send_alerts = 0; // used to turn on/off alerts -- send and admin message
                     // from
    // remote to turn it on -- or server side call set_alerts();

    // Set up callback handler for ping message from client so that it
    // sends the button states.  This will make sure that the other side hears
    // the initial button states.  Also set this up
    // to fire on the "new connection" system message.

    register_autodeleted_handler(d_ping_message_id, handle_ping_message, this,
                                 d_sender_id);
    register_autodeleted_handler(
        d_connection->register_message_type(vrpn_got_connection),
        handle_ping_message, this, vrpn_ANY_SENDER);

    // set button default buttonstates
    for (vrpn_int32 i = 0; i < vrpn_BUTTON_MAX_BUTTONS; i++) {
        buttonstate[i] = vrpn_BUTTON_MOMENTARY;
    }
    return;
}

int vrpn_Button_Filter::handle_ping_message(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_Button_Filter *me = (vrpn_Button_Filter *)userdata;
    me->report_states();
    return 0;
}

void vrpn_Button_Filter::set_alerts(vrpn_int32 i)
{
    if (i == 0 || i == 1)
        send_alerts = i;
    else
        fprintf(stderr, "Invalid send_alert state\n");
    return;
}

void vrpn_Button_Filter::set_momentary(vrpn_int32 which_button)
{
    if (which_button >= num_buttons) {
        char msg[200];
        sprintf(msg, "vrpn_Button::set_momentary() buttons id %d is greater "
                     "than the number of buttons(%d)\n",
                which_button, num_buttons);
        send_text_message(msg, timestamp, vrpn_TEXT_ERROR);
        return;
    }
    buttonstate[which_button] = vrpn_BUTTON_MOMENTARY;
    if (send_alerts) PACK_ALERT_MESSAGE(which_button, vrpn_BUTTON_TOGGLE_OFF);
}

void vrpn_Button::set_momentary(vrpn_int32 which_button)
{
    if (which_button >= num_buttons) {
        char msg[200];
        sprintf(msg, "vrpn_Button::set_momentary() buttons id %d is greater "
                     "than the number of buttons(%d)\n",
                which_button, num_buttons);
        send_text_message(msg, timestamp, vrpn_TEXT_ERROR);
        return;
    }
    PACK_ADMIN_MESSAGE(which_button, vrpn_BUTTON_MOMENTARY);
}

void vrpn_Button_Filter::set_toggle(vrpn_int32 which_button,
                                    vrpn_int32 current_state)
{
    if (which_button >= num_buttons) {
        char msg[200];
        sprintf(msg, "vrpn_Button::set_toggle() buttons id %d is greater then "
                     "the number of buttons(%d)\n",
                which_button, num_buttons);
        send_text_message(msg, timestamp, vrpn_TEXT_ERROR);
        return;
    }
    if (current_state == vrpn_BUTTON_TOGGLE_ON) {
        buttonstate[which_button] = vrpn_BUTTON_TOGGLE_ON;
        if (send_alerts)
            PACK_ALERT_MESSAGE(which_button, vrpn_BUTTON_TOGGLE_ON);
    }
    else {
        buttonstate[which_button] = vrpn_BUTTON_TOGGLE_OFF;
        if (send_alerts)
            PACK_ALERT_MESSAGE(which_button, vrpn_BUTTON_TOGGLE_OFF);
    }
}
void vrpn_Button::set_toggle(vrpn_int32 which_button, vrpn_int32 current_state)
{
    if (which_button >= num_buttons) {
        char msg[200];
        sprintf(msg, "vrpn_Button::set_toggle() buttons id %d is greater then "
                     "the number of buttons(%d)\n",
                which_button, num_buttons);
        send_text_message(msg, timestamp, vrpn_TEXT_ERROR);
        return;
    }
    if (current_state == vrpn_BUTTON_TOGGLE_ON) {
        PACK_ADMIN_MESSAGE(which_button, vrpn_BUTTON_TOGGLE_ON);
    }
    else {
        PACK_ADMIN_MESSAGE(which_button, vrpn_BUTTON_TOGGLE_OFF);
    }
}

void vrpn_Button_Filter::set_all_momentary(void)
{
    for (vrpn_int32 i = 0; i < num_buttons; i++) {
        if (buttonstate[i] != vrpn_BUTTON_MOMENTARY) {
            buttonstate[i] = vrpn_BUTTON_MOMENTARY;
            if (send_alerts) PACK_ALERT_MESSAGE(i, vrpn_BUTTON_TOGGLE_OFF);
        }
    }
}

void vrpn_Button::set_all_momentary(void)
{
    PACK_ADMIN_MESSAGE(vrpn_ALL_ID, vrpn_BUTTON_MOMENTARY);
}

void vrpn_Button::set_all_toggle(vrpn_int32 default_state)
{
    PACK_ADMIN_MESSAGE(vrpn_ALL_ID, default_state);
}

void vrpn_Button_Filter::set_all_toggle(vrpn_int32 default_state)
{
    for (vrpn_int32 i = 0; i < num_buttons; i++) {
        if (buttonstate[i] == vrpn_BUTTON_MOMENTARY) {
            buttonstate[i] = default_state;
            if (send_alerts) {
                PACK_ALERT_MESSAGE(i, default_state);
            }
        }
    }
}

void vrpn_Button::print(void)
{
    vrpn_int32 i;
    printf("CurrButtons: ");
    for (i = num_buttons - 1; i >= 0; i--) {
        printf("%c", buttons[i] ? '1' : '0');
    }
    printf("\n");

    printf("LastButtons: ");
    for (i = num_buttons - 1; i >= 0; i--) {
        printf("%c", lastbuttons[i] ? '1' : '0');
    }
    printf("\n");
}

/** Encode a message describing the new state of a button.
    Assumes that there is enough room in the buffer to hold
    the bytes from the message. Returns the number of bytes
    sent.
*/

vrpn_int32 vrpn_Button::encode_to(char *buf, vrpn_int32 button,
                                  vrpn_int32 state)
{
    char *bufptr = buf;
    int buflen = 1000;

    // Message includes: vrpn_int32 buttonNum, vrpn_int32 state
    vrpn_buffer(&bufptr, &buflen, button);
    vrpn_buffer(&bufptr, &buflen, state);

    return 1000 - buflen;
}

/** Encode a message describing the state of all buttons.
    Assumes that there is enough room in the buffer to hold
    the bytes from the message. Returns the number of bytes
    sent.
*/

vrpn_int32 vrpn_Button::encode_states_to(char *buf)
{
    // Message includes: vrpn_int32 number_of_buttons, vrpn_int32 states
    // Byte order of each needs to be reversed to match network standard

    vrpn_int32 int_btn = num_buttons;
    int buflen = (vrpn_BUTTON_MAX_BUTTONS + 1) * sizeof(vrpn_int32);

    vrpn_buffer(&buf, &buflen, int_btn);
    for (int i = 0; i < num_buttons; i++) {
        vrpn_int32 state = buttons[i];
        vrpn_buffer(&buf, &buflen, state);
    }

    return (num_buttons + 1) * sizeof(vrpn_int32);
}

/** Encode a message describing the state of all buttons.
    Assumes that there is enough room in the buffer to hold
    the bytes from the message. Returns the number of bytes
    sent.
*/

vrpn_int32 vrpn_Button_Filter::encode_states_to(char *buf)
{
    // Message includes: vrpn_int32 number_of_buttons, vrpn_int32 state
    // Byte order of each needs to be reversed to match network standard

    vrpn_int32 int_btn = num_buttons;
    int buflen = (vrpn_BUTTON_MAX_BUTTONS + 1) * sizeof(vrpn_int32);

    vrpn_buffer(&buf, &buflen, int_btn);
    for (int i = 0; i < num_buttons; i++) {
        vrpn_buffer(&buf, &buflen, buttonstate[i]);
    }

    return (num_buttons + 1) * sizeof(vrpn_int32);
}

static int VRPN_CALLBACK client_msg_handler(void *userdata, vrpn_HANDLERPARAM p)
{
    vrpn_Button_Filter *instance = (vrpn_Button_Filter *)userdata;
    const char *bufptr = p.buffer;
    vrpn_int32 event;
    vrpn_int32 buttonid;

    vrpn_unbuffer(&bufptr, &buttonid);
    vrpn_unbuffer(&bufptr, &event);

    if (event == vrpn_BUTTON_MOMENTARY) {
        if (buttonid == vrpn_ALL_ID)
            instance->set_all_momentary();
        else
            instance->set_momentary(buttonid);
    }
    else if (event == vrpn_BUTTON_TOGGLE_OFF ||
             event == vrpn_BUTTON_TOGGLE_ON) {
        if (buttonid == vrpn_ALL_ID)
            instance->set_all_toggle(event);
        else
            instance->set_toggle(buttonid, event);
    }
    return 0;
}

void vrpn_Button_Filter::report_changes(void)
{
    vrpn_int32 i;

    //   vrpn_Button::report_changes();
    if (d_connection) {
        for (i = 0; i < num_buttons; i++) {
            switch (buttonstate[i]) {
            case vrpn_BUTTON_MOMENTARY:
                if (buttons[i] != lastbuttons[i]) PACK_MESSAGE(i, buttons[i]);
                break;
            case vrpn_BUTTON_TOGGLE_ON:
                if (buttons[i] && !lastbuttons[i]) {
                    buttonstate[i] = vrpn_BUTTON_TOGGLE_OFF;
                    if (send_alerts)
                        PACK_ALERT_MESSAGE(i, vrpn_BUTTON_TOGGLE_OFF);
                    PACK_MESSAGE(i, 0);
                }
                break;
            case vrpn_BUTTON_TOGGLE_OFF:
                if (buttons[i] && !lastbuttons[i]) {
                    buttonstate[i] = vrpn_BUTTON_TOGGLE_ON;
                    if (send_alerts)
                        PACK_ALERT_MESSAGE(i, vrpn_BUTTON_TOGGLE_ON);
                    PACK_MESSAGE(i, 1);
                }
                break;
            default:
                fprintf(stderr, "vrpn_Button::report_changes(): Button %d in \
			invalid state (%d)\n",
                        i, buttonstate[i]);
            }
            lastbuttons[i] = buttons[i];
        }
    }
    else {
        fprintf(stderr, "vrpn_Button: No valid connection\n");
    }
}

void vrpn_Button::report_changes(void)
{
    vrpn_int32 i;

    if (d_connection) {
        for (i = 0; i < num_buttons; i++) {
            if (buttons[i] != lastbuttons[i]) PACK_MESSAGE(i, buttons[i]);
            lastbuttons[i] = buttons[i];
        }
    }
    else {
        fprintf(stderr, "vrpn_Button: No valid connection\n");
    }
}

void vrpn_Button::report_states(void)
{
    // msgbuf must be int32-aligned!
    vrpn_int32 ibuf[vrpn_BUTTON_MAX_BUTTONS + 1];
    char *msgbuf = (char *)ibuf;

    vrpn_int32 len;

    len = vrpn_Button::encode_states_to(msgbuf);
    if (d_connection &&
        d_connection->pack_message(len, timestamp, states_message_id,
                                   d_sender_id, msgbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_Button: cannot write states message: tossing\n");
    }
}

#ifndef VRPN_CLIENT_ONLY

vrpn_Button_Server::vrpn_Button_Server(const char *name, vrpn_Connection *c,
                                       int numbuttons)
    : vrpn_Button_Filter(name, c)
{
    if (numbuttons > vrpn_BUTTON_MAX_BUTTONS) {
        num_buttons = vrpn_BUTTON_MAX_BUTTONS;
    }
    else {
        num_buttons = numbuttons;
    }
}

int vrpn_Button_Server::number_of_buttons(void) { return num_buttons; }

void vrpn_Button_Server::mainloop(void)
{
    server_mainloop();
    report_changes();
}

int vrpn_Button_Server::set_button(int button, int new_value)
{
    if ((button < 0) || (button >= num_buttons)) {
        return -1;
    }

    buttons[button] = (unsigned char)(new_value != 0);

    return 0;
}

vrpn_Button_Example_Server::vrpn_Button_Example_Server(const char *name,
                                                       vrpn_Connection *c,
                                                       int numbuttons,
                                                       vrpn_float64 rate)
    : vrpn_Button_Filter(name, c)
{
    if (numbuttons > vrpn_BUTTON_MAX_BUTTONS) {
        num_buttons = vrpn_BUTTON_MAX_BUTTONS;
    }
    else {
        num_buttons = numbuttons;
    }

    _update_rate = rate;

    // IN A REAL SERVER, open the device that will service the buttons here
}

void vrpn_Button_Example_Server::mainloop()
{
    struct timeval current_time;
    int i;

    // Call the generic server mainloop, since we are a server
    server_mainloop();

    // See if its time to generate a new report (every 1 second).
    // IN A REAL SERVER, this check would not be done; although the
    // time of the report would be updated to the current time so
    // that the correct timestamp would be issued on the report.
    vrpn_gettimeofday(&current_time, NULL);
    if (vrpn_TimevalDuration(current_time, timestamp) >=
        1000000.0 / _update_rate) {

        // Update the time
        timestamp.tv_sec = current_time.tv_sec;
        timestamp.tv_usec = current_time.tv_usec;

        // Update the values for the buttons, to say that each one has
        // switched its state.
        // THIS CODE WILL BE REPLACED by the user code that
        // actually reads from the button devices and fills in buttons[]
        // appropriately.
        for (i = 0; i < num_buttons; i++) {
            buttons[i] = !lastbuttons[i];
        }

        // Send reports. Stays the same in a real server.
        report_changes();
    }
}

// changed to take raw port hex address
vrpn_Button_Parallel::vrpn_Button_Parallel(const char *name, vrpn_Connection *c,
                                           int portno, unsigned porthex)
    : vrpn_Button_Filter(name, c)
{
#if defined(VRPN_USE_LINUX_PARALLEL_SUPPORT)
    const char *portname;
    switch (portno) {
    case 1:
        portname = "/dev/lp0";
        break;
    case 2:
        portname = "/dev/lp1";
        break;
    case 3:
        portname = "/dev/lp2";
        break;
    default:
        fprintf(stderr, "vrpn_Button_Parallel: "
                        "Bad port number (%x) for Linux lp#\n",
                portno);
        status = BUTTON_FAIL;
        portname = "UNKNOWN";
        break;
    }

    // Open the port
    if ((port = open(portname, O_RDWR)) < 0) {
        perror("vrpn_Button_Parallel::vrpn_Button_Parallel(): "
               "Can't open port");
        fprintf(stderr, "vrpn_Button_Parallel::vrpn_Button_Parallel(): "
                        "Can't open port %s\n",
                portname);
        status = BUTTON_FAIL;
        return;
    }
#elif defined(_WIN32)
    // if on NT we need a device driver to do direct reads
    // from the parallel port
    if (!openGiveIO()) {
        fprintf(stderr, "vrpn_Button: need giveio driver for port access!\n");
        fprintf(stderr, "vrpn_Button: can't use vrpn_Button()\n");
        status = BUTTON_FAIL;
        return;
    }
    if (porthex != 0) // using direct hex port address
        port = porthex;
    else { // using old style port 1, 2, or 3
        switch (portno) {
        // we use this mapping, although LPT? can actually
        // have different addresses.
        case 1:
            port = 0x378;
            break; // usually LPT1
        case 2:
            port = 0x3bc;
            break; // usually LPT2
        case 3:
            port = 0x278;
            break; // usually LPT3
        default:
            fprintf(stderr, "vrpn_Button_Parallel: Bad port number (%d)\n",
                    portno);
            status = BUTTON_FAIL;
            break;
        }
    }
    fprintf(stderr, "vrpn_Button_Parallel: Using port %x\n", port);
#else // not linux or _WIN32
    fprintf(stderr, "vrpn_Button_Parallel: not supported on this platform\n?");
    status = BUTTON_FAIL;
    portno = portno + 1; // unused argument if we don't do something with it
    porthex = porthex + 1; // unused argument if we don't do something with it
    return;
#endif

#if defined(VRPN_USE_LINUX_PARALLEL_SUPPORT) || defined(_WIN32)
// Set the INIT line on the device to provide power to the python
//  update: don't need this for builtin LPT1 on DELLs, but do need it
//    for warp9 PCI parallel port. Added next lines.  BCE Nov07.00
// Setting Bit 1 high also for a specific HiBall system
//  requirement (IR blocking device on same port)
//  - BCE 08 July 03

#ifdef _outp
    const unsigned short DATA_REGISTER_OFFSET = 0;
    _outp((unsigned short)(port + DATA_REGISTER_OFFSET), 3);
#else
    fprintf(stderr, "vrpn_Button_Parallel: Not setting bit 0 on Linux, may not "
                    "work with all ports\n");
#endif

    // Zero the button states
    num_buttons = 5; // XXX This is specific to the python
    for (vrpn_int32 i = 0; i < num_buttons; i++) {
        buttons[i] = lastbuttons[i] = 0;
    }

    status = BUTTON_READY;
    vrpn_gettimeofday(&timestamp, NULL);
#endif
}

vrpn_Button_Parallel::~vrpn_Button_Parallel()
{
#ifdef VRPN_USE_LINUX_PARALLEL_SUPPORT
    if (port >= 0) {
        close(port);
    }
#endif
}

vrpn_Button_Python::vrpn_Button_Python(const char *name, vrpn_Connection *c,
                                       int p)
    : vrpn_Button_Parallel(name, c, p)
    , d_first_fail(true)
{
}

vrpn_Button_Python::vrpn_Button_Python(const char *name, vrpn_Connection *c,
                                       int p, unsigned ph)
    : vrpn_Button_Parallel(name, c, p, ph)
    , d_first_fail(true)
{
}

void vrpn_Button_Python::mainloop()
{
    // Call the generic server mainloop, since we are a server
    server_mainloop();

    switch (status) {
    case BUTTON_READY:
        read();
        report_changes();
        break;
    case BUTTON_FAIL:
        if (d_first_fail) {
            d_first_fail = false;
            fprintf(stderr, "vrpn_Button_Python failure!\n");
            send_text_message("Failure", timestamp, vrpn_TEXT_ERROR);
        }
        break;
    }
}

// Fill in the buttons[] array with the current value of each of the
// buttons.  Debounce the buttons (make sure that they have had the
// same value for the past several times to debounce noise).
void vrpn_Button_Python::read(void)
{
    int i;
    const int debounce_count = 30;
    int status_register[debounce_count];

    // Make sure we're ready to read
    if (status != BUTTON_READY) {
        return;
    }

// Read from the status register, read 3 times to debounce noise.
#if defined(VRPN_USE_LINUX_PARALLEL_SUPPORT)
    for (i = 0; i < debounce_count; i++)
        if (ioctl(port, LPGETSTATUS, &status_register[i]) == -1) {
            perror("vrpn_Button_Python::read(): ioctl() failed");
            return;
        }

#elif defined(_WIN32)
#ifndef __CYGWIN__
    const unsigned short STATUS_REGISTER_OFFSET = 1;
    for (i = 0; i < debounce_count; i++) {
#ifdef _inp
        status_register[i] =
            _inp((unsigned short)(port + STATUS_REGISTER_OFFSET));
#else // !_inp
        status_register[i] = 0;
#endif // _inp
    }
#else // __CYGWIN__
    for (i = 0; i < debounce_count; i++) {
        status_register[i] = 0;
    }
#endif // __CYGWIN
#else // not _WIN32 or VRPN_USE_LINUX_PARALLEL_SUPPORT
    for (i = 0; i < debounce_count; i++) {
        status_register[i] = 0;
    }
#endif // _WIN32, VRPN_USE_LINUX_PARALLEL_SUPPORT
    for (i = 0; i < debounce_count; i++) {
        status_register[i] = status_register[i] & BIT_MASK;
    }

    // Make sure they are all the same; if not, return and assume that
    // we are still debouncing.
    for (i = 1; i < debounce_count; i++) {
        if (status_register[0] != status_register[i]) {
            return;
        }
    }

    // Assign values to the bits based on the control signals
    buttons[0] = ((status_register[0] & PORT_SLCT) == 0);
    buttons[1] = ((status_register[0] & PORT_BUSY) != 0);
    buttons[2] = ((status_register[0] & PORT_PE) == 0);
    buttons[3] = ((status_register[0] & PORT_ERROR) == 0);
    buttons[4] = ((status_register[0] & PORT_ACK) == 0);

    vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Button_Serial::vrpn_Button_Serial(const char *name, vrpn_Connection *c,
                                       const char *port, long baud)
    : vrpn_Button_Filter(name, c)
{
    // port name and baud rate
    if (port == NULL) {
        fprintf(stderr, "vrpn_Button_Serial: NULL port name\n");
        status = BUTTON_FAIL;
        return;
    }
    else {
        vrpn_strcpy(portname, port);
    }
    baudrate = baud;

    // Open the serial port
    if ((serial_fd = vrpn_open_commport(portname, baudrate)) == -1) {
        fprintf(stderr, "vrpn_Button_Serial: Cannot Open serial port\n");
        status = BUTTON_FAIL;
    }

    // Reset the tracker and find out what time it is
    status = BUTTON_READY;
    vrpn_gettimeofday(&timestamp, NULL);
}

vrpn_Button_Serial::~vrpn_Button_Serial() { vrpn_close_commport(serial_fd); }

// init pinch glove to send hand data only
vrpn_Button_PinchGlove::vrpn_Button_PinchGlove(const char *name,
                                               vrpn_Connection *c,
                                               const char *port, long baud)
    : vrpn_Button_Serial(name, c, port, baud)
    , reported_failure(false)
{
    num_buttons = 10; // 0-4: right, 5-9: left starting from thumb
    status = BUTTON_READY;

    // check if the number of buttons is not more than max allowed.
    if (num_buttons > vrpn_BUTTON_MAX_BUTTONS) {
        fprintf(stderr,
                "vrpn_Button_PinchGlove: Too many buttons. The limit is ");
        fprintf(stderr, "%i but this device has %i.\n", vrpn_BUTTON_MAX_BUTTONS,
                num_buttons);
        status = BUTTON_FAIL;
    }

    // set glove to report with no timestamp
    report_no_timestamp();

    // initialize the buttons
    for (vrpn_int32 i = 0; i < num_buttons; i++)
        buttons[i] = lastbuttons[i] = VRPN_BUTTON_OFF;

    // Reset the tracker and find out what time it is
    vrpn_gettimeofday(&timestamp, NULL);
}

void vrpn_Button_PinchGlove::mainloop()
{
    // Call the generic server mainloop, since we are a server
    server_mainloop();

    switch (status) {
    case BUTTON_READY:
        read();
        report_changes();
        break;
    case BUTTON_FAIL: {
        if (!reported_failure) {
            reported_failure = true;
            fprintf(stderr, "vrpn_Button_PinchGlove failure!\n");
        }
    } break;
    }
    return;
}

// Pinch glove should only report hand data(see contructor). If the message has
// time stamp data change the the message type by sending command to just send
// hand data. Any other kind of message is ignored and the buffer is read until
// end of message byte(PG_END_BYTE) is reached.
void vrpn_Button_PinchGlove::read()
{
    // check if button is ready
    if (status != BUTTON_READY) return;

    // check if there is something to read
    if (vrpn_read_available_characters(serial_fd, buffer, 1) != 1) return;

    // This while loop is to keep reading messages until its empty.
    while (buffer[0] != PG_END_BYTE) {
        // switch according to its message start byte
        if (buffer[0] == PG_START_BYTE_DATA) {
            // reset button state
            for (vrpn_int32 i = 0; i < num_buttons; i++)
                buttons[i] = VRPN_BUTTON_OFF;

            // read next message
            bufcount = vrpn_read_available_characters(serial_fd, buffer, 2);

            // read until end of message
            while (buffer[0] != PG_END_BYTE) {

                // if only one byte read, read the left hand byte
                while (bufcount != 2)
                    bufcount += vrpn_read_available_characters(serial_fd,
                                                               buffer + 1, 1);

                unsigned char mask = 0x10;
                // set button states
                for (int j = 0; j < 5; j++, mask = (unsigned char)(mask >> 1)) {
                    if (mask & buffer[1]) { // right hand
                        buttons[j] = VRPN_BUTTON_ON;
                    }
                    if (mask & buffer[0]) { // left hand
                        buttons[j + 5] = VRPN_BUTTON_ON;
                    }
                }

                // read next bytes
                bufcount = vrpn_read_available_characters(serial_fd, buffer, 2);
            } // while (buffer[0] != PG_END_BYTE)
            // if there is another message read it
            if (bufcount != 1) buffer[0] = buffer[1];
        } // if (buffer[0] == PG_START_BYTE_DATA) {
        else if (buffer[0] == PG_START_BYTE_DATA_TIME) {
            send_text_message(
                "vrpn_Button_PinchGlove message start byte: time stamped byte!",
                timestamp, vrpn_TEXT_ERROR);
            report_no_timestamp();
        }
        else { // ignore any other type of messages: empty butter till
               // PG_END_BYTE
            // clear messages
            while (buffer[0] != PG_END_BYTE)
                vrpn_read_available_characters(serial_fd, buffer, 1);
            send_text_message("vrpn_Button_PinchGlove wrong message start byte",
                              timestamp, vrpn_TEXT_ERROR);
        } // else
    }     // while (buffer[0] != PG_END_BYTE )

    vrpn_gettimeofday(&timestamp, NULL);
    return;
}

// set the glove to report data without timestamp
void vrpn_Button_PinchGlove::report_no_timestamp()
{
    struct timeval timeout = {
        0, 30000}; // time_out for response from glove: 30 msec
    struct timeval timeout_to_pass;

    // read until correct reply is received
    do {
        vrpn_flush_input_buffer(serial_fd);
        vrpn_write_characters(serial_fd, (const unsigned char *)"T0", 2);
        vrpn_drain_output_buffer(serial_fd);
        timeout_to_pass.tv_sec = timeout.tv_sec;
        timeout_to_pass.tv_usec = timeout.tv_usec;
        bufcount = vrpn_read_available_characters(serial_fd, buffer, 3,
                                                  &timeout_to_pass);
    } while ((bufcount != 3) || (buffer[1] != '0') ||
             (buffer[2] != PG_END_BYTE));

    return;
}

#endif // VRPN_CLIENT_ONLY

vrpn_Button_Remote::vrpn_Button_Remote(const char *name, vrpn_Connection *cn)
    : vrpn_Button(name, cn)
{
    vrpn_int32 i;

    // Register a handler for the change callback from this device,
    // if we got a connection.
    if (d_connection != NULL) {
        if (register_autodeleted_handler(
                change_message_id, handle_change_message, this, d_sender_id)) {
            fprintf(stderr,
                    "vrpn_Button_Remote: can't register change handler\n");
            d_connection = NULL;
        }
        if (register_autodeleted_handler(
                states_message_id, handle_states_message, this, d_sender_id)) {
            fprintf(stderr,
                    "vrpn_Button_Remote: can't register states handler\n");
            d_connection = NULL;
        }
    }
    else {
        fprintf(stderr, "vrpn_Button_Remote: Can't get connection!\n");
    }

    // Fill in all possible buttons with off.  The number of buttons
    // will be overwritten when the states message comes in from the
    // server.
    num_buttons = vrpn_BUTTON_MAX_BUTTONS;
    for (i = 0; i < num_buttons; i++) {
        buttons[i] = lastbuttons[i] = 0;
    }
    vrpn_gettimeofday(&timestamp, NULL);
}

// virtual
vrpn_Button_Remote::~vrpn_Button_Remote(void) {}

void vrpn_Button_Remote::mainloop()
{
    if (d_connection) {
        d_connection->mainloop();
    }
    client_mainloop();
}

int vrpn_Button_Remote::handle_change_message(void *userdata,
                                              vrpn_HANDLERPARAM p)
{
    vrpn_Button_Remote *me = (vrpn_Button_Remote *)userdata;
    const char *bufptr = p.buffer;
    vrpn_BUTTONCB bp;

    // Fill in the parameters to the button from the message
    if (p.payload_len != 2 * sizeof(vrpn_int32)) {
        fprintf(stderr, "vrpn_Button: change message payload error\n");
        fprintf(stderr, "             (got %d, expected %lud)\n", p.payload_len,
                static_cast<unsigned long>(2 * sizeof(vrpn_int32)));
        return -1;
    }
    bp.msg_time = p.msg_time;
    vrpn_unbuffer(&bufptr, &bp.button);
    vrpn_unbuffer(&bufptr, &bp.state);

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->d_callback_list.call_handlers(bp);

    return 0;
}

int vrpn_Button_Remote::handle_states_message(void *userdata,
                                              vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_int32 numbuttons; //< Number of buttons
    vrpn_Button_Remote *me = (vrpn_Button_Remote *)userdata;
    vrpn_BUTTONSTATESCB cp;

    cp.msg_time = p.msg_time;
    vrpn_unbuffer(&bufptr, &numbuttons);
    cp.num_buttons = numbuttons;
    me->num_buttons = cp.num_buttons;
    for (vrpn_int32 i = 0; i < cp.num_buttons; i++) {
        vrpn_unbuffer(&bufptr, &cp.states[i]);
    }

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->d_states_callback_list.call_handlers(cp);

    return 0;
}

// We use _inp (inport) to read the parallel port status register
// since we haven't found a way to do it through the Win32 API.
// On NT we need a special device driver (giveio) to be installed
// so that we can get permission to execute _inp.  On Win95 we
// don't need to do anything.
//
// The giveio driver is from the May 1996 issue of Dr. Dobb's,
// an article by Dale Roberts called "Direct Port I/O and Windows NT"
//
// This function returns 1 if either running Win95 or running NT and
// the giveio device was opened.  0 if device not opened.
#ifndef VRPN_CLIENT_ONLY
#ifdef _WIN32
int vrpn_Button_Parallel::openGiveIO(void)
{
    OSVERSIONINFO osvi;

    memset(&osvi, 0, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);

    // if Win95: no initialization required
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
        return 1;
    }

    // else if NT: use giveio driver
    if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        HANDLE h = CreateFile("\\\\.\\giveio", GENERIC_READ, 0, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            // maybe driver not installed?
            return 0;
        }
        CloseHandle(h);
        // giveio opened successfully
        return 1;
    }

    // else GetVersionEx gave unexpected result
    fprintf(stderr,
            "vrpn_Button_Parallel::openGiveIO: unknown windows version\n");
    return 0;
}
#endif // _WIN32
#endif // VRPN_CLIENT_ONLY
