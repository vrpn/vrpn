#ifndef VRPN_5DT_H
#define VRPN_5DT_H

#include "vrpn_Connection.h"
#include "vrpn_Analog.h"

class VRPN_API vrpn_5dt: public vrpn_Serial_Analog
{
public:
	vrpn_5dt (const char * name,
		  vrpn_Connection * c,
		  const char * port,
		  int baud = 19200,
		  int mode = 1,
		  bool tenbytes = false);

	/// Called once through each main loop iteration to handle updates.
	virtual void mainloop ();
	
	void syncing (void);

  protected:
	int _status;		    //< Reset, Syncing, or Reading
	int _numchannels;	    //< How many analog channels to open
	int _mode ;                  //< glove mode for reporting data (see glove manual)
	int _expected_chars;	    //< How many characters to expect in the report
	unsigned char _buffer[512]; //< Buffer of characters in report
	int _bufcount;		    //< How many characters we have so far
	bool  _tenbytes;	    //< Whether there are 10-byte responses (unusual, but seen)

	struct timeval timestamp;   //< Time of the last report from the device

	virtual int reset(void);		//< Set device back to starting config
	virtual	void get_report(void);		//< Try to read a report from the device

	virtual void clear_values(void);	//< Clears all channels to 0

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
