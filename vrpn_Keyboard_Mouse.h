#ifndef VRPN_KEYMOUSE_H
#define VRPN_KEYMOUSE_H

#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"

class VRPN_API vrpn_KeyMouse: public vrpn_Tracker, public vrpn_Button
{
  public:
	vrpn_KeyMouse (const char * name, vrpn_Connection * c, int num_buttons);

	~vrpn_KeyMouse () {};

	/// Called once through each main loop iteration to handle updates.
	virtual void mainloop ();

	virtual int reset(void);  ///< Set device back to starting config

	void SetDeviceParams(char source[512],int button1,int button2,float scale);
  protected:

	float m_dQuatPrev[3][4]; // previous rotation

	int _numbuttons;          ///< How many buttons to open
	unsigned char buf[512];	  ///< Buffer of characters in report,
	int bufpos;               ///< Current char pos in buffer 
        int packtype;             ///< What kind of packet we are decoding
        int packlen;              ///< Expected packet length
        int escapedchar;          ///< We're processing an escaped char
        int erroroccured;         ///< A device error has occured
        int resetoccured;         ///< A reset event has occured
	struct timeval timestamp; ///< Time of the last report from the device

	int m_buttonCodes[16];
	int m_Translations[6];
	float m_Scale[6];
	int m_Rotations[6];
	int m_Reset;
	bool m_bTranslations[3]; //true if mouse movements are used 
	bool m_bRotations[3]; //true if mouse movements are used 
#ifdef  _WIN32
        POINT m_prevPos[3]; // mouse movement x,y,z
#endif

	void ConvertOriToQuat(float ori[3]); //< directly put the values in the quat for message sending
	void AddPreviousOri(float ori[4]); //< add previous rotation to the current
	virtual	void clear_values(void); ///< Set all buttons, analogs and encoders back to 0

	/// Try to read reports from the device.  Returns 1 if a complete 
        /// report received, 0 otherwise.  Sets status to current mode.
	virtual	int get_report(void);

	/// send report if changed
        virtual void report_changes
                   (vrpn_uint32 class_of_service
                    = vrpn_CONNECTION_LOW_LATENCY);

        /// send report whether or not changed
        virtual void report
                   (vrpn_uint32 class_of_service
                    = vrpn_CONNECTION_LOW_LATENCY);

        // NOTE:  class_of_service is only applied to vrpn_Analog
        //  values, not vrpn_Button, which are always vrpn_RELIABLE
};

#endif
