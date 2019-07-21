// Copyright 2015 by Russ Taylor, working for ReliaSolve.
// Based on the vrpn_Tng3.C source file.
// License: Standard VRPN.
//
// See the vrpn_streaming_arduino directory for a program that should be
// loaded onto the Arduino and be running for this device to connect to.

#include <math.h>                       // for floor
#include <iostream>
#include <sstream>

#include "vrpn_Streaming_Arduino.h"

#define VRPN_TIMESTAMP_MEMBER m_timestamp // Configuration required for vrpn_MessageMacros in this class.
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

#undef VERBOSE

#define MAX_TCHANNELS 8	

// Defines the modes in which the box can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the box
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report
#define MAX_TIME_INTERVAL  (2000000) // max time between reports (usec)


// This creates the device and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.

vrpn_Streaming_Arduino::vrpn_Streaming_Arduino(std::string name,
    vrpn_Connection * c,
    std::string port,
    int numchannels,
    int baud)
  :
    vrpn_Serial_Analog(name.c_str(), c, port.c_str(), baud),
    vrpn_Button_Filter(name.c_str(), c),
    m_numchannels(numchannels)
{
    // Verify the validity of the parameters
    if (m_numchannels > MAX_TCHANNELS) {
      std::cerr << "vrpn_Streaming_Arduino: Can only support "
        << MAX_TCHANNELS << " analog channels, not "
        << m_numchannels << std::endl;
	    m_numchannels = MAX_TCHANNELS;
    }

    // Set the parameters in the parent classes
    vrpn_Analog::num_channel = m_numchannels;

    // Set the status of the buttons, analogs and encoders to 0 to start
    clear_values();
    vrpn_gettimeofday(&m_timestamp, NULL);

    // Set the mode to reset
    m_status = STATUS_RESETTING;
}

void	vrpn_Streaming_Arduino::clear_values(void)
{
    int	i;

    for (i = 0; i < m_numchannels; i++) {
      vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
    }
}

// This routine will reset the device, asking it to send the requested number
// of analogs.
// It verifies that it can communicate with the device.

int vrpn_Streaming_Arduino::reset(void)
{
    //-----------------------------------------------------------------------
    // Set the values back to zero for all analogs
    clear_values();

    //-----------------------------------------------------------------------
    // Opening the port should cause the board to reset.  (We can also do this
    // by pulling DTR lines to low and then back high again.)
    // @todo Lower and raise DTR lines.  Or close and re-open the serial
    // port.

    // Wait for two seconds to let the device reset itself and then
    // clear the input buffer, both the hardware input buffer and
    // the local input buffer.
    vrpn_SleepMsecs(2100);
    vrpn_flush_input_buffer(serial_fd);
    m_buffer.clear();

    // Send a command telling how many ports we want.  Then make sure we
    // get a response within a reasonable amount of time.
    std::ostringstream msg;
    msg << m_numchannels;
    msg << "\n";
    vrpn_write_characters(serial_fd,
      static_cast<const unsigned char *>(
      static_cast<const void*>(msg.str().c_str())), msg.str().size());
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 10000;
    unsigned char buffer;
    int ret = vrpn_read_available_characters(serial_fd, &buffer, 1, &timeout);
    if (ret != 1) {
      std::cout << "vrpn_Streaming_Arduino: Could not reset" << std::endl;
      return -1;
    }
    m_buffer += buffer;

    // We already got the first byte of the record, so we drop directly
    // into reading.
    status = STATUS_READING;
    vrpn_gettimeofday(&m_timestamp, NULL);	// Set watchdog now
    return 0;
}

// This function will read characters until it has a full report, then
// put that report into the time, analog amd button fields and call
// the report methods on these. The time stored is that of
// the first character received as part of the report.  Each time
// through, it gets however many characters are available.
// 
// If we get a report that is not valid, we reset.
//
// The routine that calls this one
// makes sure we get a full reading often enough (ie, it is responsible
// for doing the watchdog timing to make sure the device hasn't simply
// stopped sending characters).
   
int vrpn_Streaming_Arduino::get_report(void)
{
    unsigned int buttonBits = 0;

    // Zero timeout, poll for any available characters
    struct timeval timeout = {0, 0};

    // When we get the first byte and we're syncing, record the time
    // it arrived.
    if (status == STATUS_SYNCING) {
      unsigned char buffer;
      if (1 == vrpn_read_available_characters(serial_fd, &buffer, 1, &timeout)) {
        m_buffer += buffer;
	      status = STATUS_READING;
        vrpn_gettimeofday(&m_timestamp, NULL);
      }
    }

    // If we're not reading, then we have nothing to do
    if (STATUS_READING != status) {
    	return 0;
    }

    // Find out the time of the read.
    struct timeval read_time;
    vrpn_gettimeofday(&read_time, NULL);

    // We're reading now.  Look for a large number of characters and see
    // what we get.
    unsigned char buffer[1024];
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int result = vrpn_read_available_characters(serial_fd, 
		  buffer, sizeof(buffer)-1, &timeout);    
    if (result < 0) {
      VRPN_MSG_WARNING("Bad read, resetting");
      reset();
      return 0;
    }
    // Copy the resulting bytes into the buffer.
    buffer[result] = '\0';
    m_buffer += std::string(static_cast<char*>(static_cast<void*>(buffer)));

    // See if there is a carriage return in the buffer.  If so, we can parse
    // the report ahead of it and then remove it from the buffer.
    size_t cr = m_buffer.find('\n');
    while (cr != std::string::npos) {

      // Parse the report.
      std::istringstream s(m_buffer.substr(0, cr));
      for (size_t i = 0; i < m_numchannels; i++) {
        int val = 0;
        char comma;
        s >> val;
        if (s.fail()) {
          std::cerr << "vrpn_Streaming_Arduino: Can't parse "
            << s.str() << " for value " << i << std::endl;
          std::cerr << " Report was: " << m_buffer << std::endl;
          VRPN_MSG_WARNING("Bad value, resetting");
          reset();
          return 0;
        }
        if (i < m_numchannels - 1) { s >> comma; }
        if (s.fail()) {
          std::cerr << "vrpn_Streaming_Arduino: Can't read comma "
            "for value " << i << " (value parsed was " << val << ")" << std::endl;
          std::cerr << " Report was: " << m_buffer << std::endl;
          VRPN_MSG_WARNING("Bad value, resetting");
          reset();
          return 0;
        }
        vrpn_Analog::last[i] = vrpn_Analog::channel[i];
        vrpn_Analog::channel[i] = val;
      }

      // @todo parse any markers and put them into button reports.

      // Back-date the reports based on the transmission rate of the
      // characters from the Arduino -- the number of characters in
      // the buffer while we're parsing tells how many characters
      // arrived since that reading was taken.
      // The bit rate is 115200, with 10 bits/character making our
      // byte rate 11520 per second, resulting in around 87 micro-
      // seconds per character (a little under a tenth of a milli-
      // second).  So we get about 1ms of delay for every 10
      // characters.
      int offset_usec = static_cast<int>(87 * m_buffer.size());
      struct timeval offset = { 0 , offset_usec };
      m_timestamp = vrpn_TimevalDiff(read_time, offset);

      // Gobble up the first report and see if there is another.
      m_buffer.erase(0, cr + 1);
      cr = m_buffer.find('\n');

      // Report any changes.
      report_changes();
    }

    vrpn_gettimeofday(&m_timestamp, NULL);	// Set watchdog now

    status = STATUS_SYNCING;
    return 1;
}

void vrpn_Streaming_Arduino::report_changes(vrpn_uint32 class_of_service)
{
    vrpn_Analog::timestamp = m_timestamp;
    vrpn_Button::timestamp = m_timestamp;

    vrpn_Analog::report_changes(class_of_service);
    vrpn_Button::report_changes();
}

void	vrpn_Streaming_Arduino::report(vrpn_uint32 class_of_service)
{
    vrpn_Analog::timestamp = m_timestamp;
    vrpn_Button::timestamp = m_timestamp;

    vrpn_Analog::report(class_of_service);
    vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the TNG3,
// either trying to reset it or trying to get a reading from it.
void vrpn_Streaming_Arduino::mainloop(void)
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
	        vrpn_gettimeofday(&current_time, NULL);
	        if ( vrpn_TimevalDuration(current_time,m_timestamp) > MAX_TIME_INTERVAL) {
            std::cerr << "vrpn_Streaming_Arduino failed to read... current_time="
              << current_time.tv_sec << ":" << current_time.tv_usec
              << ", timestamp="
              << m_timestamp.tv_sec << ":" << m_timestamp.tv_usec
              << " (resetting)" << std::endl;
		        send_text_message("Too long since last report, resetting", current_time, vrpn_TEXT_ERROR);
		        status = STATUS_RESETTING;
	        }
	    }
	    break;

	    default:
	        fprintf(stderr,"vrpn_Streaming_Arduino: Unknown mode (internal error)\n");
	        break;
    }
}
