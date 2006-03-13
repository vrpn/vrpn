#ifndef VRPN_KEYMOUSE_H
#define VRPN_KEYMOUSE_H

///////////////////////////////////////////////////////////////////////////
// vrpn_KeyMouse is a VRPN server class to publish events from the PC's mouse
// and keyboard.
// It provides a x-channel vrpn_Analog depending on the config file, and a
// x-channel vrpn_Button for mouse buttons or keyboard buttons.
//
// This implementation is Windows-specific, as it leverages the windows mouse calls
//
// Use vrpn_Mouse if you are using linux

#include "vrpn_Connection.h"
#include "vrpn_Analog.h"
#include "vrpn_Button.h"

class VRPN_API vrpn_KeyMouse: public vrpn_Analog, public vrpn_Button
{
  public:
	vrpn_KeyMouse (const char * name, vrpn_Connection * c, int num_buttons, int num_channels);

	~vrpn_KeyMouse () ;

	/// Called once through each main loop iteration to handle updates.
	virtual void mainloop ();

	void SetAnalogParams(int channelNr,int button1,int button2, float scale);
	void SetButtonParams( int buttonNr, int button1);

  protected:
	  int *m_ButtonCodesLMove; //< keeps track of keys needed to lower the analog value
	  int *m_ButtonCodesRMove; //< keeps track of keys needed to raise the analog value
	  bool *m_ButtonCodesUseMouse; //< is true when a analog channel uses a mouse movement
	  float *m_Scale; //< keeps track of the size of change on the analog value per key press
	  int *m_ButtonCodes; //< keeps track of keys needed as a button value
#ifdef _WIN32
	  POINT *m_PrevPos; //< keeps track of the mouse position when the mouse is inactive
#endif

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

};

#endif
