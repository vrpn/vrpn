#include <math.h>
#include <string.h>
#include "vrpn_Keyboard_Mouse.h"
#include "vrpn_Shared.h"
#ifdef	_WIN32
#include <windows.h>
#pragma comment (lib, "user32.lib")     // We'll need the user32 DLL
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
vrpn_KeyMouse::vrpn_KeyMouse (const char * name, vrpn_Connection * c, int num_buttons):
		vrpn_Tracker(name, c),
		vrpn_Button(name, c),
		_numbuttons(8)
{
	// Set the parameters in the parent classes
	vrpn_Button::num_buttons = _numbuttons;
	
        int i;
	for(i=0;i<3;i++)
	{
		m_bTranslations[i]=false;
		m_bRotations[i]=false;
	}
	// Set the status of the buttons and analogs to 0 to start
	clear_values();
	
	// Set the mode to reset
	status = STATUS_RESETTING;
}

void vrpn_KeyMouse::SetDeviceParams(char source[512],int button1,int button2, float scale)
{
	if(!strcmp(source,"X"))
	{
		m_Translations[0]=button1;
		m_Translations[1]=button2;
		m_Scale[0]=scale;
		if(button2==0xFF0 || button2==0xFF1)
			m_bTranslations[0]=true;
	}
	else if (!strcmp(source,"Y"))
	{
		m_Translations[2]=button1;
		m_Translations[3]=button2;
		m_Scale[1]=scale;
		if(button2==0xFF0 || button2==0xFF1)
			m_bTranslations[1]=true;
	}
	else if (!strcmp(source,"Z"))
	{
		m_Translations[4]=button1;
		m_Translations[5]=button2;
		m_Scale[2]=scale;
		if(button2==0xFF0 || button2==0xFF1)
			m_bTranslations[2]=true;
	}
	if(!strcmp(source,"RX"))
	{
		m_Rotations[0]=button1;
		m_Rotations[1]=button2;
		m_Scale[3]=scale;
		if(button2==0xFF0 || button2==0xFF1)
			m_bRotations[0]=true;
	}
	else if (!strcmp(source,"RY"))
	{
		m_Rotations[2]=button1;
		m_Rotations[3]=button2;
		m_Scale[4]=scale;
		if(button2==0xFF0 || button2==0xFF1)
			m_bRotations[1]=true;
	}
	else if (!strcmp(source,"RZ"))
	{
		m_Rotations[4]=button1;
		m_Rotations[5]=button2;
		m_Scale[5]=scale;
		if(button2==0xFF0 || button2==0xFF1)
			m_bRotations[2]=true;
	}
	else if(!strcmp(source,"RESET"))
	{
		m_Reset=button1;
	}
	else if(!strncmp(source,"BUTTON",6))
	{
		m_buttonCodes[int(source[6])-48]=button1;
	}
}

void	vrpn_KeyMouse::clear_values(void)
{

}

// This routine will reset the KeyMouse, zeroing the Origin position,
// mode.
int	vrpn_KeyMouse::reset(void) 
{

	// We're now waiting for a response from the box
	status = STATUS_READING;

	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

	pos[0] = pos[1] = pos[2] = 0;
	d_quat[0] = 1;
	d_quat[1] = d_quat[2] = d_quat[3] = 0;
	buttons[0] = buttons[1] = 0;
	

	return 1;
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
	vrpn_gettimeofday(&timestamp, NULL); // set timestamp of this event
#ifdef	_WIN32
        int i;
	for(i=0;i<3;i++) {
          if(m_bTranslations[i]) { // mouse movement
			if((0x8000 & GetKeyState(m_Translations[i*2]))) {
				POINT posi;
				GetCursorPos(&posi);
				if(m_Translations[i*2+1]==0xFF0)
					pos[i]+=(posi.x-m_prevPos[i].x)/1000.0f;
				else
					pos[i]+=(posi.y-m_prevPos[i].y)/1000.0f;
				report_changes();        // Report updates to VRPN
			} else {
				GetCursorPos(&m_prevPos[i]);
			}
		} else {
			if((0x8000 & GetKeyState(m_Translations[i*2]))) {
				pos[i]+= 0.0001*m_Scale[i];
				report_changes();        // Report updates to VRPN
			} else if((0x8000 & GetKeyState(m_Translations[i*2+1]))) {
				pos[i]-= 0.0001*m_Scale[i];
				report_changes();        // Report updates to VRPN
			}
		}
	}
		
	float ori[3]={0,0,0};
	for(i=0;i<3;i++) {
          if(m_bRotations[i]) { // mouse movement
			if((0x8000 & GetKeyState(m_Rotations[i*2]))) {
				POINT posi;
				GetCursorPos(&posi);
				if(m_Rotations[i*2+1]==0xFF0)
					ori[i]+=posi.x-m_prevPos[i].x;
				else
					ori[i]+=posi.y-m_prevPos[i].y;
				ConvertOriToQuat(ori);
				AddPreviousOri(m_dQuatPrev[i]);
				report_changes();        // Report updates to VRPN
			} else {
				GetCursorPos(&m_prevPos[i]);
				m_dQuatPrev[i][0] = static_cast<float>(d_quat [0]);
				m_dQuatPrev[i][1] = static_cast<float>(d_quat [1]);
				m_dQuatPrev[i][2] = static_cast<float>(d_quat [2]);
				m_dQuatPrev[i][3] = static_cast<float>(d_quat [3]);
			}
		} else {
			if((0x8000 & GetKeyState(m_Rotations[i*2]))) {
				ori[i]+= 1*m_Scale[i+3];
				ConvertOriToQuat(ori);
				report_changes();        // Report updates to VRPN
			} else if((0x8000 & GetKeyState(m_Rotations[i*2+1]))) {
				ori[i]-= 1*m_Scale[i+3];
				ConvertOriToQuat(ori);
				report_changes();        // Report updates to VRPN
			}
		}
	}

	for(i=0;i<_numbuttons;i++) {
		if((0x8000 & GetKeyState(m_buttonCodes[i]))) {
			buttons[i]  = 1; 
			report_changes();        // Report updates to VRPN
		} else if(buttons[0]==1) {
			buttons[i]  = 0; 
			report_changes();        // Report updates to VRPN
		}
	}
	if((0x8000 & GetKeyState(m_Reset))) {
		reset();
		report_changes();        // Report updates to VRPN
	}
#endif

	return 1;
}

void vrpn_KeyMouse::ConvertOriToQuat(float ori[3])
{
	
	
	float real0,real1,real2,real;
	float imag0,imag1,imag2,imag[3];

	real0= pcos(ori[0]/2);
	real1= pcos(ori[1]/2);
	real2= pcos(ori[2]/2);
	
	imag0 = psin(ori[0]/2); 
	imag1 = psin(ori[1]/2); 
	imag2 = psin(ori[2]/2); 

	// merge the first two quats
	real = real0 * real1 ;

	if ( real > 1 )
		real = 1;
	else if ( real < -1 )
		real = -1;

	imag[0] = imag0 * real1;
	imag[1] = real0 * imag1;
	imag[2] = imag0 * imag1;

	// merge previous result with last quat

	d_quat[0] = real * real2 - imag[2] * imag2 ;

	if ( d_quat[0] > 1 )
		d_quat[0] = 1;
	else if ( d_quat[0] < -1 )
		d_quat[0] = -1;

	d_quat[1] = imag[0] * real2 + imag[1] * imag2;
	d_quat[2] = imag[1] * real2 - imag[0] * imag2;
	d_quat[3] = real * imag2 + imag[2] * real2;
	if(d_quat[1]==0 && d_quat[2]==0 && d_quat[3]==0)
		d_quat[0]=1;
 	
}

void vrpn_KeyMouse::AddPreviousOri(float ori[4])
{
  float dQuat[4]={ static_cast<float>(d_quat[0]),
                   static_cast<float>(d_quat[1]),
                   static_cast<float>(d_quat[2]),
                   static_cast<float>(d_quat[3]) };
  // merge previous
  d_quat[0] = dQuat[0] * ori[0] - dQuat[1] * ori[1] - dQuat[2] * ori[2] -dQuat[3] * ori[3] ;

  if ( d_quat[0] > 1 ) {
    d_quat[0] = 1;
  } else if ( d_quat[0] < -1 ) {
    d_quat[0] = -1;
  }

  d_quat[1] = dQuat[0] * ori[1] + dQuat[1] * ori[0] + dQuat[2] * ori[3] - dQuat[3] * ori[2];
  d_quat[2] = dQuat[0] * ori[2] + dQuat[2] * ori[0] - dQuat[1] * ori[3] + dQuat[3] * ori[1];
  d_quat[3] = dQuat[0] * ori[3] + dQuat[3] * ori[0] + dQuat[1] * ori[2] - dQuat[2] * ori[1];
}

void vrpn_KeyMouse::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Button::timestamp = timestamp;
	vrpn_Tracker::timestamp = timestamp;

	vrpn_Button::report_changes();
	if (d_connection) {
		char	msgbuf[1000];
		int	len = vrpn_Tracker::encode_to(msgbuf);
		if (d_connection->pack_message(len, timestamp,
			position_m_id, d_sender_id, msgbuf,
			vrpn_CONNECTION_LOW_LATENCY)) {
				MC_ERROR("Tracker: cannot write message: tossing\n");
			}
	} else {
		MC_ERROR("Tracker: No valid connection\n");
	} 
}

void vrpn_KeyMouse::report(vrpn_uint32 class_of_service)
{
	vrpn_Button::timestamp = timestamp;
	vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the KeyMouse,
// either trying to reset it or trying to get a reading from it.
void vrpn_KeyMouse::mainloop()
{
	server_mainloop();

  switch(status) 
  {
    case STATUS_RESETTING:
		reset();
		break;
    case STATUS_READING:
		get_report();
        break;
    default:
		fprintf(stderr,"vrpn_KeyMouse: Unknown mode (internal error)\n");
		break;
  }
}


