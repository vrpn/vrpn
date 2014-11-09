#ifndef VRPN_YEI_3SPACE_H
#define VRPN_YEI_3SPACE_H

#include "vrpn_Analog.h"
#include "vrpn_Analog_Output.h"
#include "vrpn_Tracker.h"

/** @brief Class to support reading data from serial YEI 3Space units.
*/
class VRPN_API vrpn_YEI_3Space_Sensor: public vrpn_Serial_Analog
{
public:
	/** @brief Constructor.
		@param name Name for the device
		@param c Connection to use.
		@param port serial port to connect to
		@param baud Baud rate - 115200 is default.
		@param calibrate_gyros_on_setup - true to cause this to happen
		@param tare_on_setup - true to cause this to happen
		@param frames_per_second - How many frames/second to read
	*/
	vrpn_YEI_3Space_Sensor (const char * name,
		  vrpn_Connection * c,
		  const char * port,
		  int baud = 115200,
		  bool calibrate_gyros_on_setup = false,
		  bool tare_on_setup = false,
		  double frames_per_second = 50);

	/// Called once through each main loop iteration to handle updates.
	virtual void mainloop ();
	
	void syncing (void);

  protected:
    double  d_frames_per_second;    //< How many frames/second do we want?

	bool _announced;		//< Did we make the note about potential warnings yet?
	bool _wireless;			//< Whether this glove is using the wireless protocol
	bool _gotInfo;			//< Whether we've sent a message about this wireless glove
	int _status;		    //< Reset, Syncing, or Reading
	int _numchannels;	    //< How many analog channels to open
	int _mode ;                  //< glove mode for reporting data (see glove manual)
	unsigned _expected_chars;	    //< How many characters to expect in the report
	unsigned char _buffer[512]; //< Buffer of characters in report
	unsigned _bufcount;		    //< How many characters we have so far
	bool  _tenbytes;	    //< Whether there are 10-byte responses (unusual, but seen)

	struct timeval timestamp;   //< Time of the last report from the device

	virtual int reset(void);		//< Set device back to starting config
	virtual	void get_report(void);		//< Try to read a report from the device

	/// Compute the CRC for the message, append it, and send message.
	/// Returns 0 on success, -1 on failure.
	int send_command(const unsigned char *cmd, int len);

	/// send report iff changed
        virtual void report_changes
                   (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
        /// send report whether or not changed
        virtual void report
                   (vrpn_uint32 class_of_service = vrpn_CONNECTION_LOW_LATENCY);
};

#endif
