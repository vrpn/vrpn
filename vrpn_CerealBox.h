#ifndef VRPN_CEREALBOX_H
#define VRPN_CEREALBOX_H

#include "vrpn_Connection.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"
#include "vrpn_Dial.h"

class vrpn_CerealBox: public vrpn_Serial_Analog
			,public vrpn_Button
			,public vrpn_Dial
{
public:
	vrpn_CerealBox (const char * name, vrpn_Connection * c,
			const char * port, int baud,
			const int numbuttons, const int numchannels, const int numencoders);

	~vrpn_CerealBox ();

	// Called once through each main loop iteration to handle
	// updates.
	virtual void mainloop (const struct timeval *timeout = NULL);

  protected:
	int _status;
	int _numbuttons;	// How many buttons to open
	int _numchannels;	// How many analog channels to open
	int _numencoders;	// How many encoders to open

	int _expected_chars;	// How many characters to expect in the report
	unsigned char _buffer[512];	// Buffer of characters in report
	int _bufcount;		// How many characters we have so far

	struct timeval timestamp;	// Time of the last report from the device

	virtual	void clear_values(void);	// Set all buttons, analogs and encoders back to 0
	virtual int reset(void);		// Set device back to starting config
	virtual	void get_report(void);		// Try to read a report from the device

        virtual void report_changes (void);	// send report iff changed
	virtual void report (void);		// send report whether or not changed
};

#endif
