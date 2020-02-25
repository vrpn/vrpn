//	This is a driver for the 5dt Data Glove 16 sensors
// Look at www.5dt.com for more informations about this product
// Manuals are avalaible freely from this site
// The original code for 5dt was written by Philippe DAVID with the help of Yves GAUVIN
// This version for the 16 sensor version was written by Tom De Weyer
// naming convention used in this file:
//   l_ is the prefixe for local variables
//   g_ is the prefixe for global variables
//   p_ is the prefixe for parameters

#include <stdio.h>                      // for sprintf, fprintf, stderr

#include "vrpn_5DT16.h"
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR, etc
#include "vrpn_Serial.h"
#include "vrpn_Shared.h"                // for timeval, vrpn_SleepMsecs, etc
#include "vrpn_MessageMacros.h"         // for VRPN_MSG_INFO, VRPN_MSG_WARNING, VRPN_MSG_ERROR

#undef VERBOSE

// Defines the modes in which the device can find itself.
#define	STATUS_RESETTING	(-1)	// Resetting the device
#define	STATUS_SYNCING		(0)	// Looking for the first character of report
#define	STATUS_READING		(1)	// Looking for the rest of the report

#define MAX_TIME_INTERVAL       (2000000) // max time between reports (usec)


/******************************************************************************
 * NAME      : vrpn_5dt16::vrpn_5dt16
 * ROLE      : This creates a vrpn_5dt16 and sets it to reset mode. It opens
 *             the serial device using the code in the vrpn_Serial_Analog constructor.
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
vrpn_5dt16::vrpn_5dt16 (const char * p_name, vrpn_Connection * p_c, const char * p_port, int p_baud):
  vrpn_Serial_Analog (p_name, p_c, p_port, p_baud, 8, vrpn_SER_PARITY_NONE),
  _numchannels (17)	// This is an estimate; will change when reports come
{
  // Set the parameters in the parent classes
  vrpn_Analog::num_channel = _numchannels;

  vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

  // Set the status of the buttons and analogs to 0 to start
  clear_values();

  // Set the mode to reset
  _status = STATUS_RESETTING;
}


/******************************************************************************
 * NAME      : vrpn_5dt16::clear_values
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : 
 ******************************************************************************/
void vrpn_5dt16::clear_values (void)
{
  int i;

  for (i = 0; i < _numchannels; i++) {
      vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
  }
}


/******************************************************************************
 * NAME      : vrpn_5dt16::reset
 * ROLE      : This routine will reset the 5DT
 * ARGUMENTS : 
 * RETURN    : 0 else -1 in case of error
 ******************************************************************************/
int vrpn_5dt16::reset (void)
{
  unsigned char	l_inbuf [45];
  int		l_ret;
  bool		found=false;
  struct timeval start, now;

  vrpn_gettimeofday(&start, NULL);
  while(!found) // read the begin sequence of an info packet "<I"
  {
	l_ret = vrpn_read_available_characters (serial_fd, l_inbuf, 1);
	if(l_ret!=1) {
		vrpn_SleepMsecs (100);  //Give it time to send data
	} else if(l_inbuf[0] == '<' ) {
		while(!vrpn_read_available_characters (serial_fd, l_inbuf, 1)) {
			vrpn_SleepMsecs (100);  //Give it time to send data
		}
		if(l_inbuf[0]=='I') {
			found=true;
		}
	}
	// See if we've timed out on the reset
	vrpn_gettimeofday(&now, NULL);
	if (now.tv_sec > start.tv_sec + 2) {
		fprintf(stderr,"vrpn_5dt16::reset(): Timeout during reset\n");
		return -1;
	}
  }
  vrpn_SleepMsecs (100);  //Give it time to send data
  l_ret=vrpn_read_available_characters (serial_fd, l_inbuf, 5); // read the rest of the data
  char text[50];
  sprintf(text,"Hardware Version %i.0%i",l_inbuf[0],l_inbuf[1]); // hardware version
  VRPN_MSG_WARNING(text);
  if (l_inbuf[2] | 1) //right or left glove
  {
  	  VRPN_MSG_WARNING ("A right glove is ready");
  } else {
	  VRPN_MSG_WARNING ("A left glove is ready");
  }
  if (l_inbuf[3] | 1) //wireless glove or wired
  {
	  VRPN_MSG_WARNING ("no wireless glove");
  } else {
	  VRPN_MSG_WARNING ("wireless glove");
  }

  // We're now entering the syncing mode which send the read command to the glove
  _status = STATUS_READING;
  _bufcount = 0;

  vrpn_gettimeofday (&timestamp, NULL);	// Set watchdog now
  return 0;
}

/******************************************************************************
 * NAME      : vrpn_5dt16::get_report
 * ROLE      : This function will read characters until it has a full report, then
 *             put that report into analog fields and call the report methods on these.   
 * ARGUMENTS : void
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt16::get_report (void)
{
  int  l_ret;		// Return value from function call to be checked

  // The official doc (http://www.5dt.com/downloads/5DTDataGlove5Manual.pdf)
  // says we should get 36 bytes, but once in a while an info packet arrives and we should 
  // ignore that packet.
  _expected_chars = 36;

  //--------------------------------------------------------------------
  // Read as many bytes of this report as we can, storing them
  // in the buffer.  We keep track of how many have been read so far
  // and only try to read the rest.
  //--------------------------------------------------------------------

  l_ret = vrpn_read_available_characters (serial_fd, &_buffer [_bufcount],
					  _expected_chars - _bufcount);
  if (l_ret == -1) {
      VRPN_MSG_ERROR ("Error reading the glove");
      _status = STATUS_RESETTING;
      return;
  }
#ifdef	VERBOSE
  if (l_ret != 0) printf("... got %d characters (%d total)\n",l_ret, _bufcount);
#endif

  //--------------------------------------------------------------------
  // The time of the report is the time at which the first character for
  // the report is read.
  //--------------------------------------------------------------------

  if ( (l_ret > 0) && (_bufcount == 0) ) {
	vrpn_gettimeofday(&timestamp, NULL);
  }

  //--------------------------------------------------------------------
  // We keep track of how many characters we have received and keep
  // going back until we get as many as we expect.
  //--------------------------------------------------------------------

  _bufcount += l_ret;
  if (_bufcount < _expected_chars) {	// Not done -- go back for more
      return;
  }

  //--------------------------------------------------------------------
  // We now have enough characters to make a full report.  First check to
  // make sure that the first one is what we expect.

  if (!( (_buffer[0] == '<') && (_buffer[1] == 'D') ) ) //first characters need to be <D
  {
      VRPN_MSG_INFO ("Unexpected first character in report, probably info packet (recovering)");
      for(int i=0;i<29;i++) {
		  _buffer[i]=_buffer[i+7];
      }
      _bufcount=29;
      return;
  }

#ifdef	VERBOSE
   printf ("Got a complete report (%d of %d)!\n", _bufcount, _expected_chars);
#endif

   //--------------------------------------------------------------------
   // Decode the report and store the values in it into the analog values
   // if appropriate.
   //--------------------------------------------------------------------

   for(int i=0;i<16;i++) {
	   channel[i]=_buffer[i*2+2]*256+_buffer[i*2+3];
   }
   char text[512];

   sprintf(text,"original %f|%f|%f|%f|%f|%f|%f|%f|%f|%f|%f|%f|%f|%f|%f|%f\n",channel[0],channel[1],channel[2],channel[3],channel[4],
	   channel[5],channel[6],channel[7],channel[8],channel[9],
	   channel[10],channel[11],channel[12],channel[13],channel[14],
	   channel[15]);
   VRPN_MSG_ERROR(text);

   //--------------------------------------------------------------------
   // Done with the decoding, send the reports and go back to syncing
   //--------------------------------------------------------------------

   report_changes();
  _bufcount =0;
}


/******************************************************************************
 * NAME      : vrpn_5dt16::report_changes
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt16::report_changes (vrpn_uint32 class_of_service)
{
  vrpn_Analog::timestamp = timestamp;
  vrpn_Analog::report_changes(class_of_service);
}


/******************************************************************************
 * NAME      : vrpn_5dt16::report
 * ROLE      : 
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt16::report (vrpn_uint32 class_of_service)
{
	vrpn_Analog::timestamp = timestamp;
	vrpn_Analog::report(class_of_service);
}


/******************************************************************************
 * NAME      : vrpn_5dt16::mainloop
 * ROLE      :  This routine is called each time through the server's main loop. It will
 *              take a course of action depending on the current status of the device,
 *              either trying to reset it or trying to get a reading from it.  It will
 *              try to reset the device if no data has come from it for a couple of
 *              seconds
 * ARGUMENTS : 
 * RETURN    : void
 ******************************************************************************/
void vrpn_5dt16::mainloop ()
{
  char l_errmsg[256];

  server_mainloop();

  switch (_status) {
    case STATUS_RESETTING:
      if (reset()== -1)
	{
	  VRPN_MSG_ERROR ("vrpn_Analog_5dt: Cannot reset the glove!");
	}
      break;

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
	  get_report();
	  struct timeval current_time;
	  vrpn_gettimeofday(&current_time, NULL);
	  if ( vrpn_TimevalDuration(current_time,timestamp) > MAX_TIME_INTERVAL)
	  {
	    sprintf (l_errmsg, "vrpn_5dt16::mainloop: Timeout... current_time=%ld:%ld, timestamp=%ld:%ld",
		     current_time.tv_sec,
		     static_cast<long>(current_time.tv_usec),
		     timestamp.tv_sec,
		     static_cast<long>(timestamp.tv_usec));
	    VRPN_MSG_ERROR (l_errmsg);
	    _status = STATUS_RESETTING;
	  }
      }
      break;

    default:
      VRPN_MSG_ERROR ("vrpn_5dt16::mainloop: Unknown mode (internal error)");
      break;
  }
}

vrpn_Button_5DT_Server::vrpn_Button_5DT_Server(const char *name, const char *deviceName,vrpn_Connection *c, double threshold[16]) : vrpn_Button_Filter(name, c)
{
        num_buttons = 16;
        for(int i=0;i<num_buttons;i++) {
                m_threshold[i]=threshold[i];
	}
        d_5dt_button = NULL;
        d_5dt_button = new vrpn_Analog_Remote(deviceName, d_connection);
#ifdef  VERBOSE
        printf("vrpn_Button_5DT_Server: Adding local analog %s\n",name);
#endif
        // Set up the callback handler for the channel
        d_5dt_button->register_change_handler(this, handle_analog_update);
}

vrpn_Button_5DT_Server::~vrpn_Button_5DT_Server(void)
{
}

// This routine handles updates of the analog values. The value coming in is
// adjusted per the parameters in the full axis description, and then used to
// update the value there. The value is used by the matrix-generation code in
// mainloop() to update the transformations; that work is not done here.

void    vrpn_Button_5DT_Server::handle_analog_update (void *userdata, const vrpn_ANALOGCB info)
{
        vrpn_Button_5DT_Server  *full = (vrpn_Button_5DT_Server *)userdata;
        for(int i=0;i<16;i++) { // adjust with range and center value
                if(info.channel[i]>full->m_threshold[i] ) {
                        full->buttons[i]=true;
                } else {
                        full->buttons[i]=false;
		}
        }
}

void vrpn_Button_5DT_Server::mainloop()
{
        struct timeval current_time;

        // Call the generic server mainloop, since we are a server
        server_mainloop();

        d_5dt_button->mainloop();

        vrpn_gettimeofday(&current_time, NULL);
        // Send reports. Stays the same in a real server.
        report_changes();
}

