// vrpn_ImmersionBox.C
// This is a driver for an Immersion Corporation Immersion Box controller.
// This box is a serial-line device that allows the user to connect
// analog inputs, digital inputs, digital outputs, and digital
// encoders and read from them over RS-232. 
//
// This code is based on the ImmersionBox code previously written as part
// of the vrpn library

#include <math.h>                       // for floor
#include <stdio.h>                      // for fprintf, stderr, printf
#include <string.h>                     // for NULL, strcpy

#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_ImmersionBox.h"
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday

#undef VERBOSE

// low-level stuff
//static const unsigned char CMD_BASIC = static_cast<unsigned char>(0xC0);   // mask for command
//static const unsigned char CMD_HOMEREF = static_cast<unsigned char>(0xC1);
//static const unsigned char CMD_HOMEPOS = static_cast<unsigned char>(0xC2);
//static const unsigned char CMD_SETHOME = static_cast<unsigned char>(0xC3);
//static const unsigned char CMD_BAUDSET = static_cast<unsigned char>(0xC4);
//static const unsigned char CMD_ENDSESS = static_cast<unsigned char>(0xC5);
//static const unsigned char CMD_GETMAXS = static_cast<unsigned char>(0xC6);
//static const unsigned char CMD_SETPARM = static_cast<unsigned char>(0xC7);
static const unsigned char CMD_GETNAME = static_cast<unsigned char>(0xC8);
static const unsigned char CMD_GETPRID = static_cast<unsigned char>(0xC9);
static const unsigned char CMD_GETMODL = static_cast<unsigned char>(0xCA);
static const unsigned char CMD_GETSERN = static_cast<unsigned char>(0xCB);
static const unsigned char CMD_GETCOMM = static_cast<unsigned char>(0xCC);
static const unsigned char CMD_GETPERF = static_cast<unsigned char>(0xCD);
static const unsigned char CMD_GETVERS = static_cast<unsigned char>(0xCE);
//static const unsigned char CMD_GETMOTN = static_cast<unsigned char>(0xCF);
//static const unsigned char CMD_SETHREF = static_cast<unsigned char>(0xD0);
//static const unsigned char CMD_FACREST = static_cast<unsigned char>(0xD1);
//static const unsigned char CMD_INSMARK = static_cast<unsigned char>(0xD2);

//static const double PAUSE_RESET     = .015;
//static const double PAUSE_END       = .015;
//static const double PAUSE_RESTORE   = 2.0;
//static const double PAUSE_BYTE      = .015;

// Defines the modes in which the box can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the box
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

/* Strings for establishing connection */
char    S_INITIALIZE[5] = "IMMC";
char    S_START[6]      = "BEGIN";
char    S_END[4]        = "END";

#define MAX_TIME_INTERVAL  (2000000) // max time between reports (usec)

static void pause (double delay) {
    if (delay < 0)
	delay = 0;
    unsigned long interval = (long) floor(1000000.0 * delay);

    struct timeval start, now;
    vrpn_gettimeofday (&start, NULL);

    do {
	vrpn_gettimeofday (&now, NULL);
    } while (vrpn_TimevalDuration(now, start) < interval);
	
}

// This creates a vrpn_ImmersionBox and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
// The box will autodetect the baud rate when the "IMMC" command is sent
// to it.

vrpn_ImmersionBox::vrpn_ImmersionBox (const char * name, 
				      vrpn_Connection * c,
				      const char * port, 
				      int baud,
				      const int numbuttons, 
				      const int numchannels, 
				      const int numencoders):
    vrpn_Serial_Analog(name, c, port, baud),
    vrpn_Button_Filter(name, c),
    vrpn_Dial(name, c),
    _numbuttons(numbuttons),
    _numchannels(numchannels),
    _numencoders(numencoders)
{


    // Verify the validity of the parameters
    if (_numbuttons > MAX_IBUTTONS) {
      fprintf(stderr,"vrpn_ImmersionBox: Can only support %d buttons, not %d\n", MAX_IBUTTONS, _numbuttons);
      _numbuttons = MAX_IBUTTONS;
    }
    if (_numchannels > MAX_ICHANNELS) {
      fprintf(stderr,"vrpn_ImmersionBox: Can only support %d analog channels, not %d\n", MAX_ICHANNELS, _numchannels);
      _numchannels = MAX_ICHANNELS;
    }
    if (_numencoders > MAX_IENCODERS) {
      fprintf(stderr,"vrpn_ImmersionBox: Can only support %d encoders, not %d\n", MAX_IENCODERS, _numencoders);
      _numencoders = MAX_IENCODERS;
    }

    // explicitly clear the identification variables
    iname[0]   = 0;
    comment[0] = 0;
    serial[0]  = 0;
    id[0]      = 0;
    model[0]   = 0;
    vers[0]    = 0;
    parmf[0]   = 0;

    // Set the parameters in the parent classes
    vrpn_Button::num_buttons = _numbuttons;
    vrpn_Analog::num_channel = _numchannels;
    vrpn_Dial::num_dials = _numencoders;

    vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

    // Set the status of the buttons, analogs and encoders to 0 to start
    clear_values();
    dataRecordLength = 0;

    // Set the mode to reset
    _status = STATUS_RESETTING;
}

void	vrpn_ImmersionBox::clear_values(void)
{
    int	i;

    for (i = 0; i < _numbuttons; i++) {
	vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
    }
    for (i = 0; i < _numchannels; i++) {
	vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
    }
    for (i = 0; i < _numencoders; i++) {
	vrpn_Dial::dials[i] = 0.0;
    }
}

// This routine will reset the ImmersionBox, asking it to send the requested number
// of analogs, buttons and encoders. 
// It verifies that it can communicate with the device and checks the version of 
// the device.

int    vrpn_ImmersionBox::reset(void)
{

    //-----------------------------------------------------------------------
    // Set the values back to zero for all buttons, analogs and encoders
    clear_values();

    //-----------------------------------------------------------------------
    // sending an end at this time will force the ibox into the reset mode, if it
    // was not already.  if the ibox is in the power up mode, nothing will happen because
    // it 'should' be waiting to sync up the baudrate

    // try to synchronize for 2 seconds

    if (syncBaudrate (10.0)) {
	printf("vrpn_ImmersionBox found\n");
    } else {
	return -1;
    }

    if (0 == sendIboxCommand((char) CMD_GETNAME, (char *) &iname, .1)) {
	fprintf(stderr,"problems with ibox command CMD_GETNAME\n");
	return -1;
    }
    if (0 == sendIboxCommand((char) CMD_GETPRID, (char *) &id, .1)) {
	fprintf(stderr,"problems with ibox command CMD_GETPRID\n");
	return -1;
    }
    if (0 == sendIboxCommand((char) CMD_GETMODL, (char *) &model, .1)){
	fprintf(stderr,"problems with ibox command CMD_GETMODL\n");
	return -1;
    }
    if (0 == sendIboxCommand((char) CMD_GETSERN, (char *) &serial, .1)){
	fprintf(stderr,"problems with ibox command CMD_GETSERN\n");
	return -1;
    }
    if (0 == sendIboxCommand((char) CMD_GETCOMM, (char *) &comment, .1)){
	fprintf(stderr,"problems with ibox command CMD_GETCOMM\n");
	return -1;
    }
    if (0 == sendIboxCommand((char) CMD_GETPERF, (char *) &parmf, .1)){
	fprintf(stderr,"problems with ibox command CMD_GETPERF\n");
	return -1;
    }
    if (0 == sendIboxCommand((char) CMD_GETVERS, (char *) &vers, .1)){
	fprintf(stderr,"problems with ibox command CMD_GETVERS\n");
	return -1;
    }

#ifdef VERBOSE
    printf("%s\n%s\n%s\n%s\n%s\n%s\n", iname, id, serial, comment, parmf, vers);
#endif

    //-----------------------------------------------------------------------
    // Compute the proper string to initialize the device based on how many
    // of each type of input/output that is selected. 

    setupCommand (0, _numchannels, _numencoders);  

    unsigned char command[26] = "";
    command[0] = 0xcf;
    command[1] = 0x0;
    command[2] = (unsigned char) 10;  // milliseconds between reports
    command[3] = commandByte;

    for (int i = 4; i < 25;i++)
	command[i] = 0x0;

    //-----------------------------------------------------------------------

    int result = vrpn_write_characters(serial_fd, (const unsigned char *) command, 25);
    
    // Ask the box to send a report, ensure echo received
    if (result < 25) {
      fprintf(stderr,"vrpnImmersionBox::reset: could not write command\n");
      return -1;
    }

    pause (.1);

    // look for command echo
    result = vrpn_read_available_characters(serial_fd, (unsigned char *) command, 1);    
    
    if (result <= 0 || command[0] != 0xcf) {
	fprintf(stderr,"vrpnImmersionBox::reset: no command echo\n");
	return -1;
    }

    // flush the input buffer
    vrpn_flush_input_buffer(serial_fd);

    printf("ImmersionBox reset complete.\n");

    status = STATUS_SYNCING;
    vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
    return 0;
}

// This function will read characters until it has a full report, then
// put that report into the time, analog, button and encoder fields and call
// the report methods on these. The time stored is that of
// the first character received as part of the report. 
// 
// Reports start with the header packetHeader followed by dataRecordLength bytes
//
// If we get a report that is not valid, we assume that we have lost a
// character or something and re-synchronize by flushing the buffer
//
// The routine that calls this one
// makes sure we get a full reading often enough (ie, it is responsible
// for doing the watchdog timing to make sure the box hasn't simply
// stopped sending characters).
// Returns 1 if got a full report, 0 otherwise.
   
int vrpn_ImmersionBox::get_report(void)
{
    unsigned char responseString[MAX_IBOX_STRING];
    int i;
    unsigned int buttonBits = 0;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    status = STATUS_SYNCING;

    // process as long as we can get characters
    while (1 == vrpn_read_available_characters(serial_fd, buffer, 1)) {
	// if not a record start, skip it
	if (buffer[0] != dataPacketHeader) {
	    continue;
	}
	// we got a good one... we're reading now
	status = STATUS_READING;
	break;
    }

    // we broke out.. if we're not reading, then we have nothing to do
    if (STATUS_READING != status) {
	return 0;
    }    


    // we're reading now, get the report   

    // get the expected number of data record bytes
    int result = vrpn_read_available_characters(serial_fd, 
						(unsigned char *) responseString, 
						dataRecordLength, 
						&timeout );    
    
    if (result < dataRecordLength) {
	status = STATUS_SYNCING;
	return 0;
    }
    
    // parse the report here
    buttonBits = responseString[0];

    for (i = 0; i < _numbuttons; i++) {
	vrpn_Button::lastbuttons[i] = vrpn_Button::buttons[i];
	vrpn_Button::buttons[i]	= static_cast<unsigned char>(buttonBits & (1 << i));
    }

#if VERBOSE
    if (vrpn_Button::buttons[3])
	cerr << "left button pressed " << endl;
#endif

    // if we processed timer bits we would do so here

    // here is where we decode the analog stuff
    for (i = 0; i < _numchannels; i++) {
	vrpn_Analog::last[i] = vrpn_Analog::channel[i];
    }

    // here is where we convert the angle encoders
    for (i = 0; i < _numencoders; i++) {
	vrpn_Dial::dials[i] = 0.0;
    }

    report_changes();
    vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

    return 1;
}

void vrpn_ImmersionBox::report_changes(vrpn_uint32 class_of_service)
{
    vrpn_Analog::timestamp = timestamp;
    vrpn_Button::timestamp = timestamp;
    vrpn_Dial::timestamp = timestamp;

    vrpn_Analog::report_changes(class_of_service);
    vrpn_Button::report_changes();
    vrpn_Dial::report_changes();
}

void	vrpn_ImmersionBox::report(vrpn_uint32 class_of_service)
{
    vrpn_Analog::timestamp = timestamp;
    vrpn_Button::timestamp = timestamp;
    vrpn_Dial::timestamp = timestamp;

    vrpn_Analog::report(class_of_service);
    vrpn_Button::report_changes();
    vrpn_Dial::report();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the cerealbox,
// either trying to reset it or trying to get a reading from it.
void	vrpn_ImmersionBox::mainloop(void)
{
    server_mainloop();

    switch(status) {
	case STATUS_RESETTING:
	    reset();
	    break;

	case STATUS_SYNCING:
	case STATUS_READING:
	{
	    // It turns out to be important to get the report before checking
	    // to see if it has been too long since the last report.  This is
	    // because there is the possibility that some other device running
	    // in the same server may have taken a long time on its last pass
	    // through mainloop().  Trackers that are resetting do this.  When
	    // this happens, you can get an infinite loop -- where one tracker
	    // resets and causes the other to timeout, and then it returns the
	    // favor.  By checking for the report here, we reset the timestamp
	    // if there is a report ready (ie, if THIS device is still operating).
	    while (get_report()) {};	// Get multiple reports if available
	    struct timeval current_time;
	    vrpn_gettimeofday(&current_time, NULL);
	    if ( vrpn_TimevalDuration(current_time,timestamp) > MAX_TIME_INTERVAL) {
		    fprintf(stderr,"Tracker failed to read... current_time=%ld:%ld, timestamp=%ld:%ld\n",
					current_time.tv_sec, static_cast<long>(current_time.tv_usec),
					timestamp.tv_sec, static_cast<long>(timestamp.tv_usec));
		    send_text_message("Too long since last report, resetting", current_time, vrpn_TEXT_ERROR);
		    status = STATUS_RESETTING;
	    }
	}
	break;

	default:
	    fprintf(stderr,"vrpn_ImmersionBox: Unknown mode (internal error)\n");
	    break;
    }
}

// utility routine (optionally) to send commands to the ibox,
//                 (optionally) to return results,
//                 (optionally) delaying between sending the command and receiving the results

int vrpn_ImmersionBox::sendIboxCommand (char cmd, char * returnString, double delay) 
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 15000;
    char command[2];
    char responseString [MAX_IBOX_STRING+1];
    int result;
    
    if (cmd) {
	command[0] = cmd;	
	result = vrpn_write_characters(serial_fd, (const unsigned char *) command, 1);
	if (delay > 0)
	    pause (delay);
   
	if (NULL == returnString)
	    // if we are not anticipating a return, go back now	 
	    return result;
    }
	    
    // read the response to the command 
    result = vrpn_read_available_characters(serial_fd, (unsigned char *) responseString, MAX_IBOX_STRING, &timeout );    
    
    if (result <= 0) 
	return 0;
    
    // since we're looking for a response, we need to ensure the command was properly 
    // echoed back... To begin with, bit 7 must be set
    if (!(responseString[0] & 0x80))
	return 0;

    // the bits, excepting bit 7 must match
    if ((responseString[0] & 0x7f) != (cmd & 0x7f))
	return 0;

    // copy the remainder of the response into the response
    strcpy (returnString, &responseString[1]);

    return 1;
}


// sync the baud rate on the ibox.
// seconds determines how long the process is permitted to continue
int vrpn_ImmersionBox::syncBaudrate (double seconds) {

    struct timeval miniDelay;
    miniDelay.tv_sec = 0;
    miniDelay.tv_usec = 50000;

    unsigned long maxDelay = 1000000L * (long) seconds;
    struct timeval start_time;
    vrpn_gettimeofday(&start_time, NULL);

    int loggedOn = 0;
    unsigned char responseString[8];
    const unsigned char * matchString = (unsigned char *) S_INITIALIZE ;  // IMMC
    int index, numRead;

    if (serial_fd < 0)
	return 0;
    vrpn_flush_input_buffer(serial_fd);
    vrpn_write_characters(serial_fd, (const unsigned char *)"E", 1);
    pause (0.01);

    while (!loggedOn) {
	struct timeval current_time;
	vrpn_gettimeofday(&current_time, NULL);
	if (vrpn_TimevalDuration(current_time, start_time) > maxDelay ) {
	    // if we've timed out, go back unhappy
	    fprintf(stderr,"vrpn_ImmersionBox::syncBaudrate timeout expired: %lf secs \n", seconds);
	    break;  // out of while loop
	}
		
	// send "IMMC"
	if (4 != vrpn_write_characters(serial_fd, matchString, 4)) {
	    fprintf(stderr,"vrpn_ImmersionBox::syncBaudrate could not write to serial port\n");
	    break;  // out of while loop
	}

	pause (0.015);

	// wait for 4 characters
	numRead = vrpn_read_available_characters(serial_fd, responseString, 4, &miniDelay);

	if (numRead <= 0) 
	    continue;

	// get 4 characters, hopefully "IMMC"
	for (index = 0; index < 4; index++) {
	    // get a character, check for failure
	    if (responseString[index] != matchString[index]) 
		break;
	}

	// if we got all four, we're done
	if (4 == index)
	    loggedOn = 1;
    } 

    if (!loggedOn)
	return 0;

    // now begin the session && ensure that its an ibox we're talking to
    matchString = (const unsigned char *) "IBOX";
    vrpn_write_characters(serial_fd, (const unsigned char *)"BEGIN", 5);
    numRead = vrpn_read_available_characters(serial_fd, responseString, 4, &miniDelay);

    if (numRead <= 0) 
	return 0;

    // check 4 characters, hopefully "IBOX"
    for (index = 0; index < 4; index++) {
	// get a character, check for failure
	if (responseString[index] != matchString[index]) 
	    return 0;
    }
    vrpn_flush_input_buffer(serial_fd);
    return 1;
}
