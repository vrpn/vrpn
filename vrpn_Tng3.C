// vrpn_Tng3.C
// This is a driver for an TNG 3 controller.
// This box is a serial-line device that allows the user to connect
// 8 analog and 8 digital inputs and, read from them over RS-232. 
//
// This code is based on the Tng3 code from pulsar.org

#include <string.h>
#include "vrpn_Tng3.h"
#include "vrpn_Shared.h"
#include "vrpn_Serial.h"
#include <math.h>
#include <iostream.h>

#undef VERBOSE

// Defines the modes in which the box can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the box
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report


#define MAX_TIME_INTERVAL  (2000000) // max time between reports (usec)

static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
    return (t1.tv_usec - t2.tv_usec) +
	1000000L * (t1.tv_sec - t2.tv_sec);
}

static void pause (double delay) {
    if (delay < 0)
	delay = 0;
    unsigned long interval = (long) floor(1000000.0 * delay);

    struct timeval start, now;
    gettimeofday (&start, NULL);

    do {
	gettimeofday (&now, NULL);
    } while (duration(now, start) < interval);
	
}

// This creates a vrpn_Tng3 and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
// The box will autodetect the baud rate when the "IMMC" command is sent
// to it.

vrpn_Tng3::vrpn_Tng3 (const char * name, 
		      vrpn_Connection * c,
		      const char * port, 
		      int baud,
		      const int numbuttons, 
		      const int numchannels):
    vrpn_Serial_Analog(name, c, port, baud),
    vrpn_Button(name, c),
    _numbuttons(numbuttons),
    _numchannels(numchannels)
{


    // Verify the validity of the parameters
    if (_numbuttons > MAX_TBUTTONS) {
	cout << "vrpn_Tng3: Can only support " << MAX_TBUTTONS << " buttons, not " <<
	    _numbuttons << endl;
	_numbuttons = MAX_TBUTTONS;
    }
    if (_numchannels > MAX_TCHANNELS) {
	cout << "vrpn_Tng3: Can only support " << MAX_TCHANNELS << " analog channels, not " <<
	    _numchannels << endl;
	_numchannels = MAX_TCHANNELS;
    }

    // Set the parameters in the parent classes
    vrpn_Button::num_buttons = _numbuttons;
    vrpn_Analog::num_channel = _numchannels;

    // Set the status of the buttons, analogs and encoders to 0 to start
    clear_values();

    // Set the mode to reset
    _status = STATUS_RESETTING;
}

void	vrpn_Tng3::clear_values(void)
{
    int	i;

    for (i = 0; i < _numbuttons; i++) {
	vrpn_Button::buttons[i] = vrpn_Button::lastbuttons[i] = 0;
    }
    for (i = 0; i < _numchannels; i++) {
	vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
    }
}

// This routine will reset the Tng3, asking it to send the requested number
// of analogs, buttons and encoders. 
// It verifies that it can communicate with the device and checks the version of 
// the device.

int    vrpn_Tng3::reset(void)
{

    //-----------------------------------------------------------------------
    // Set the values back to zero for all buttons, analogs and encoders
    clear_values();

    //-----------------------------------------------------------------------
    // sending an end at this time will force the ibox into the reset mode, if it
    // was not already.  if the ibox is in the power up mode, nothing will happen because
    // it 'should' be waiting to sync up the baudrate


    // try to synchronize for 2 seconds

    bDataPacketStart = 0x55;

    if (syncDatastream (2.0))
	cout << "vrpn_Tng3 found " << endl;   
    else
	return -1;

    cout << "TNG3B found" << endl;

    // flush the input buffer
    vrpn_flush_input_buffer(serial_fd);

    cout << "Tng3 reset complete." << endl;

    status = STATUS_SYNCING;
    gettimeofday(&timestamp, NULL);	// Set watchdog now
    return 0;
}

// This function will read characters until it has a full report, then
// put that report into the time, analog amd button fields and call
// the report methods on these. The time stored is that of
// the first character received as part of the report. 
// 
// Reports start with the byte bDataPacketStart followed by 
// DATA_RECORD_LENGTH bytes
//
// If we get a report that is not valid, we assume that we have lost a
// character or something and re-synchronize by flushing the buffer
//
// The routine that calls this one
// makes sure we get a full reading often enough (ie, it is responsible
// for doing the watchdog timing to make sure the box hasn't simply
// stopped sending characters).
   
int vrpn_Tng3::get_report(void)
{
    int i;
    unsigned int buttonBits = 0;

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;

    status = STATUS_SYNCING;

    // process as long as we can get characters
    while (1 == vrpn_read_available_characters(serial_fd, buffer, 1, &timeout)) 
    {
	// if not a record start, we need to resync
	if (buffer[0] != bDataPacketStart)
	    break;

	// we got a good start byte... we're reading now
	status = STATUS_READING;

	// invert the bits for the next packet start
	bDataPacketStart ^= 0xFF;
	break;
    }

    // we broke out.. if we're not reading, then we have nothing to do
    if (STATUS_READING != status) {
	return 0;
    }

    // we're reading now, get the report   

    // get the expected number of data record bytes
    int result = vrpn_read_available_characters(serial_fd, 
						buffer, 
						DATA_RECORD_LENGTH, 
						&timeout);    
    
    if (result < DATA_RECORD_LENGTH) {
	status = STATUS_SYNCING;
	return 0;
    }

    // parse the report here
 
    // here is where we decode the analog stuff
    for (i = 0; i < _numchannels; i++) {
	vrpn_Analog::last[i] = vrpn_Analog::channel[i];
	vrpn_Analog::channel[i] = (unsigned short) buffer[i];
    }

    // get the button bits and make sense of them

    buttonBits = buffer[DATA_RECORD_LENGTH-1];
    for (i = 0; i < _numbuttons; i++) {
	vrpn_Button::lastbuttons[i] = vrpn_Button::buttons[i];
	vrpn_Button::buttons[i]	= 
	    (buttonBits & (1 << i)) ? VRPN_BUTTON_OFF : VRPN_BUTTON_ON;
    }

    report_changes();
    gettimeofday(&timestamp, NULL);	// Set watchdog now

    return 1;
}

void vrpn_Tng3::report_changes(vrpn_uint32 class_of_service)
{
    vrpn_Analog::timestamp = timestamp;
    vrpn_Button::timestamp = timestamp;

    vrpn_Analog::report_changes(class_of_service);
    vrpn_Button::report_changes();
}

void	vrpn_Tng3::report(vrpn_uint32 class_of_service)
{
    vrpn_Analog::timestamp = timestamp;
    vrpn_Button::timestamp = timestamp;

    vrpn_Analog::report(class_of_service);
    vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the TNG3,
// either trying to reset it or trying to get a reading from it.
void vrpn_Tng3::mainloop(void)
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
	    while (get_report()) {};	// Keep getting reports as long as they come
	    struct timeval current_time;
	    gettimeofday(&current_time, NULL);
	    if ( duration(current_time,timestamp) > MAX_TIME_INTERVAL) {
		    fprintf(stderr,"CerealBox failed to read... current_time=%ld:%ld, timestamp=%ld:%ld\n",current_time.tv_sec, current_time.tv_usec, timestamp.tv_sec, timestamp.tv_usec);
		    send_text_message("Too long since last report, resetting", current_time, vrpn_TEXT_ERROR);
		    status = STATUS_RESETTING;
	    }
	}
	break;

	default:
	    cerr << "vrpn_Tng3: Unknown mode (internal error) " << endl;
	    break;
    }
}


// synchronize the data stream
// seconds determines how long the process is permitted to continue

int vrpn_Tng3::syncDatastream (double seconds) {

    struct timeval miniDelay;
    miniDelay.tv_sec = 0;
    miniDelay.tv_usec = 50000;

    unsigned long maxDelay = 1000000L * (long) seconds;
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    int loggedOn = 0;
    int numRead;

    if (serial_fd < 0)
	return 0;

    // ensure that the packet start byte is valid
    if ( bDataPacketStart != 0x55 &&
	 bDataPacketStart != 0xAA )
	bDataPacketStart = 0x55;

    vrpn_flush_input_buffer(serial_fd);

//    vrpn_write_characters(serial_fd, (const unsigned char *)"E", 1);
    pause (0.01);

    while (!loggedOn) {
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	if (duration(current_time, start_time) > maxDelay ) {
	    // if we've timed out, go back unhappy
	    cout << "vrpn_Tng3::syncBaudrate timeout expired: " << seconds 
		 <<	" secs " << endl;
	    return 0;  // go back unhappy
	}
	
	// get a byte
	if (1 != vrpn_read_available_characters(serial_fd,	buffer, 1, &miniDelay))
	    continue;
	// if not a record start, skip
	if (buffer[0] != bDataPacketStart)
	    continue;
	// invert the packet start byte for the next test
	bDataPacketStart ^= 0xFF;
	
	// get an entire report
	numRead = vrpn_read_available_characters(serial_fd, 
						 buffer, DATA_RECORD_LENGTH, &miniDelay);

	if (numRead < DATA_RECORD_LENGTH) 
	    continue;

	// get the start byte for the next packet
	if (1 != vrpn_read_available_characters(serial_fd, buffer, 1, &miniDelay))
	    continue;

	// if not the anticipated record start, things are not yet sync'd
	if (buffer[0] != bDataPacketStart)
	    continue;

	// invert the packet start byte in anticipation of normal operation
	bDataPacketStart ^= 0xFF;

	// get an entire report
	numRead = vrpn_read_available_characters(serial_fd, 
						 buffer, DATA_RECORD_LENGTH, &miniDelay);

	if (numRead < DATA_RECORD_LENGTH) 
	    continue;

	return 1;
    }
    return 0;
}
