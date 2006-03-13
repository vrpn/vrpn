#include <math.h>
#include <string.h>
#include "vrpn_Keyboard_Mouse.h"
#include "vrpn_Shared.h"
#ifdef	_WIN32
#include <windows.h>
#pragma comment (lib, "user32.lib")
#endif


// turn on for debugging code, leave off otherwise
#undef VERBOSE

#if defined(VERBOSE) 
#include <ctype.h> // for isprint()
#define DEBUG 1
#endif

// Defines the modes in which the box can find itself.
#define	STATUS_RESETTING       (-1) // Resetting the device
#define	STATUS_SYNCING		(0) // Looking for the first char of report
#define	STATUS_READING		(1) // Looking for the rest of the report
#define MAX_TIME_INTERVAL (2000000) // max time between reports (usec)

#define	MC_INFO(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_NORMAL) ; if (d_connection) d_connection->send_pending_reports(); }
#define	MC_WARNING(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_WARNING) ; if (d_connection) d_connection->send_pending_reports(); }
#define	MC_ERROR(msg)	{ send_text_message(msg, timestamp, vrpn_TEXT_ERROR) ; if (d_connection) d_connection->send_pending_reports(); }

#ifndef	M_PI
const	double	M_PI = acos(1.0) * 2;
#endif

inline float pcos(float x) {return (float)cos(double(x)*M_PI/180.0f);}
inline float psin(float x) {return (float)sin(double(x)*M_PI/180.0f);}

// This creates a vrpn_KeyMouse and sets it to reset mode. It opens
// the serial device using the code in the vrpn_Serial_Analog constructor.
vrpn_KeyMouse::vrpn_KeyMouse (const char * name, vrpn_Connection * c, int num_buttons, int num_channels):
		vrpn_Analog(name, c),
		vrpn_Button(name, c)
{
	// Set the parameters in the parent classes
	vrpn_Button::num_buttons = num_buttons;
	m_ButtonCodes = new int[num_buttons];
	   
	vrpn_Analog::num_channel = num_channels;
	m_ButtonCodesLMove = new int[num_channels];
	m_ButtonCodesRMove = new int[num_channels];
	m_ButtonCodesUseMouse = new bool[num_channels];
	m_Scale = new float[num_channels];
#ifdef _WIN32
	m_PrevPos = new POINT[num_channels];
#endif
    for( int i = 0; i < vrpn_Analog::num_channel; i++)
    {
		vrpn_Analog::channel[i] = vrpn_Analog::last[i] = 0;
		m_ButtonCodesLMove[i] = 0;
		m_ButtonCodesRMove[i] = 0;
		m_ButtonCodesUseMouse[i] = false;
		m_Scale[i] = 1.0f; 
    }
	
	m_ButtonCodesLMove = new int[num_channels];
	m_ButtonCodesRMove = new int[num_channels];
	m_ButtonCodesUseMouse = new bool[num_channels];
	m_Scale = new float[num_channels];

#ifndef _WIN32
	fprintf(stderr,"vrpn_KeyMouse:: Not implement on this architecture\n");
#endif
}


vrpn_KeyMouse::~vrpn_KeyMouse()
{
		delete [] m_ButtonCodes;
		delete [] m_ButtonCodesLMove;
		delete [] m_ButtonCodesRMove;
		delete [] m_ButtonCodesUseMouse;
		delete [] m_Scale;
}

void vrpn_KeyMouse::SetAnalogParams(int channelNr,int button1,int button2, float scale)
{
	if (channelNr>=vrpn_Analog::num_channel)
		return;
	m_ButtonCodesLMove[channelNr] = button1;
	m_ButtonCodesRMove[channelNr] = button2;
	m_Scale[channelNr] = scale;
	if (button2==0xFF0 || button2==0xFF1)
		m_ButtonCodesUseMouse[channelNr]=true;
	else
		m_ButtonCodesUseMouse[channelNr]=false;
}

void vrpn_KeyMouse::SetButtonParams( int buttonNr, int button1)
{
	m_ButtonCodes[buttonNr]=button1;
}

//   This function will read characters until it has a full report, then
// put that report into the time, analog, or button fields and call
// the report methods on these. The time stored is that of
// the first character received as part of the report.
//   Reports start with different characters, and the length of the report
// depends on what the first character of the report is.  We switch based
// on the first character of the report to see how many more to expect and
// to see how to handle the report.
//   Returns 1 if there is a complete report found, 0 otherwise.  This is
// so that the calling routine can know to check again at the end of complete
// reports to see if there is more than one report buffered up.
   
int vrpn_KeyMouse::get_report(void)
{
	status = STATUS_READING;        // ready to process event packet
	struct timeval time;
	vrpn_gettimeofday(&time, NULL); // set timestamp of this event
	vrpn_Analog::timestamp = time;
	int i;
#ifdef	_WIN32
	for(i=0;i<vrpn_Analog::num_channel;i++) 
	{
          if(m_ButtonCodesUseMouse[i])  // one button and mouse movement
		  { // mouse movement
			if((0x8000 & GetKeyState(m_ButtonCodesLMove[i]))) 
			{
				POINT posi;
				GetCursorPos(&posi);
				if(m_ButtonCodesRMove[i]==0xFF0) // x mouse movement
					channel[i] = (posi.x-m_PrevPos[i].x)*m_Scale[i];
				else
					channel[i] = (posi.y-m_PrevPos[i].y)*m_Scale[i];
				report_changes();        // Report updates to VRPN
			} 
			else // set the previous position
			{
				GetCursorPos(&m_PrevPos[i]);
			}
		  } 
		  else // double key movement
		  {
			if((0x8000 & GetKeyState(m_ButtonCodesLMove[i])))  
			{
				channel[i] = -1 * m_Scale[i];
				report_changes();        // Report updates to VRPN
			} 
			else if((0x8000 & GetKeyState(m_ButtonCodesRMove[i]))) 
			{
				channel[i] = 1*m_Scale[i];
			}
			else
				channel[i] = 0;
		}
	}
	
	for(i=0;i<vrpn_Button::num_buttons;i++) 
	{
		if((0x8000 & GetKeyState(m_ButtonCodes[i]))) 
			buttons[i]  = 1; 
		else if(buttons[i]==1) 
			buttons[i]  = 0; 
	}
	
	report_changes();        // Report updates to VRPN
#endif

	return 1;
}

void vrpn_KeyMouse::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Button::timestamp = vrpn_Analog::timestamp;

    vrpn_Analog::report_changes( class_of_service );
    vrpn_Button::report_changes();
}

void vrpn_KeyMouse::report(vrpn_uint32 class_of_service)
{
	vrpn_Button::timestamp = vrpn_Analog::timestamp;

    vrpn_Analog::report( class_of_service );
    vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the KeyMouse,
// either trying to reset it or trying to get a reading from it.
void vrpn_KeyMouse::mainloop()
{
	server_mainloop();
 	get_report();
}


