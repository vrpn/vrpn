
#include <math.h>                       // for cos, sin
#include <stdio.h>                      // for fprintf, stderr
#include <string.h>                     // for strcmp, NULL

#include "vrpn_3DMicroscribe.h"
#include "vrpn_BaseClass.h"             // for ::vrpn_TEXT_ERROR
#include "vrpn_Shared.h"                // for timeval, vrpn_gettimeofday
#include "vrpn_MessageMacros.h"

#ifdef VRPN_USE_MICROSCRIBE
#include "armdll32.h"
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

#define MM_TO_METERS 0.001

#define VR_PI  3.14159265359
inline float pcos(float x) {return (float)cos(double(x)*VR_PI/180.0f);}
inline float psin(float x) {return (float)sin(double(x)*VR_PI/180.0f);}

// This creates a vrpn_3DMicroscribe and sets it to reset mode.
vrpn_3DMicroscribe::vrpn_3DMicroscribe (const char * name, vrpn_Connection * c,
					const char * Port, long int BaudRate,
					float OffsetX/* = 0.0f*/,
					float OffsetY/* = 0.0f*/,
					float OffsetZ/* = 0.0f*/,
					float Scale/*=1.0f*/):
		vrpn_Tracker(name, c),
		vrpn_Button_Filter(name, c),
		_numbuttons(2)
{
	// Set the parameters in the parent classes
	vrpn_Button::num_buttons = _numbuttons;
	
	if(!strcmp(Port, "COM1") )
		m_PortNumber=1;
	else if(!strcmp(Port, "COM2") )
		m_PortNumber=2;
	else if(!strcmp(Port, "COM3") )
		m_PortNumber=3;
	else if(!strcmp(Port, "COM4") )
		m_PortNumber=4;
	m_BaudRate=BaudRate;
	m_OffSet[0]=OffsetX; m_OffSet[1]=OffsetY; m_OffSet[2]=OffsetZ;
	m_Scale=Scale;

	// Set the status of the buttons and analogs to 0 to start
	clear_values();

        vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now

#ifdef VRPN_USE_MICROSCRIBE
	int iResult;
	iResult=ArmStart(NULL);
	if(ARM_SUCCESS != iResult)
	{
		//error starting the MicroScribe drivers
		VRPN_MSG_ERROR( "Unable to start MicroScribe ArmDll32." );
		return;
	}

	//don't use error handlers
	iResult = ArmSetErrorHandlerFunction(NO_HCI_HANDLER, NULL);
	iResult = ArmSetErrorHandlerFunction(BAD_PORT_HANDLER, NULL);
	iResult = ArmSetErrorHandlerFunction(CANT_OPEN_HANDLER, NULL);
	iResult = ArmSetErrorHandlerFunction(CANT_BEGIN_HANDLER, NULL);

	//connect to the correct port
	switch(m_PortNumber)
	{
	case 1:
		iResult = ArmConnect(1, m_BaudRate);
		break;
	case 2:
		iResult = ArmConnect(2, m_BaudRate);
		break;
	case 3:
		iResult = ArmConnect(3, m_BaudRate);
		break;
	case 4:
		iResult = ArmConnect(4, m_BaudRate);
		break;
	default:
		iResult = ArmConnect(0, 0); //try all available ports and baud rates
		break;
	}

	if(ARM_SUCCESS != iResult)
	{ 
		//error connecting, end the thread
		ArmEnd();
		VRPN_MSG_ERROR( "Unable to connect to the MicroScribe." );
		return;
	}

#endif
	// Set the mode to reset
	status = STATUS_RESETTING;
}

void	vrpn_3DMicroscribe::clear_values(void)
{
}

// This routine will reset the 3DMicroscribe, zeroing the Origin position,
// mode.
int	vrpn_3DMicroscribe::reset(void) 
{
#ifdef VRPN_USE_MICROSCRIBE
	int iResult;
	// ARM_FULL: ArmDll32 calculates and updates the Cartesian position and orientation of the stylus tip.
	iResult = ArmSetUpdate(ARM_FULL);
	
	if(iResult != ARM_SUCCESS) 
	{ 
		//error setting the update type, disconnect and end the thread
		VRPN_MSG_ERROR( "Unable to set the update type for the MicroScribe." );
		return -1;
	}

	//use mm instead of inches
	ArmSetLengthUnits(ARM_MM);
	//use radians instead of degrees
	//ArmSetAngleUnits(ARM_RADIANS);

	//get the position of the tip
	length_3D tipPosition;
	angle_3D tipVector;

	iResult = ArmGetTipPosition(&tipPosition); //retrieves the current stylus tip position in Cartesian coordinates
	iResult = ArmGetTipOrientationUnitVector(&tipVector); //retrieves the current stylus tip's unit vector orientation

	if(iResult == ARM_NOT_CONNECTED)
	{ 
		//error connecting
		VRPN_MSG_ERROR( "MicroScribe connection lost!" );
		return -1;
	}
	
#endif
	// We're now waiting for a response from the box
	status = STATUS_SYNCING;

	vrpn_gettimeofday(&timestamp, NULL);	// Set watchdog now
	return 0;
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
   
int vrpn_3DMicroscribe::get_report(void)
{
#ifdef VRPN_USE_MICROSCRIBE
	length_3D tipPosition;
	angle_3D tipOri;
	DWORD buts;
	int iResult = ArmGetTipPosition(&tipPosition); //retrieves the current stylus tip position in Cartesian coordinates
	iResult = ArmGetTipOrientation(&tipOri); //retrieves the current stylus tip's unit vector orientation
	iResult = ArmGetButtonsState(&buts);
	if(iResult == ARM_NOT_CONNECTED)
	{ 
		//error connecting
		VRPN_MSG_ERROR( "MicroScribe connection lost!" );
		return 0;
	}

	//set the position, considering the scale, offset and origin matrix
	pos[0] = (tipPosition.y * m_Scale + m_OffSet[0])* MM_TO_METERS ;
	pos[1] = (tipPosition.z * m_Scale + m_OffSet[1])* MM_TO_METERS;
	pos[2] = (tipPosition.x * m_Scale + m_OffSet[2])* MM_TO_METERS;
	//vPosition = m_Matrix * vPosition + m_vPlaneOffset; extending the microscribe onto a plane 

	//set the orientation, considering the origin matrix
	float ori[3]={tipOri.y, tipOri.z, tipOri.x};
	ConvertOriToQuat(ori);

	
    status = STATUS_READING;        // ready to process event packet
    vrpn_gettimeofday(&timestamp, NULL); // set timestamp of this event
  
    buttons[0]  = ((buts & 0x02) != 0); // button 1
    buttons[1]  = ((buts & 0x01) != 0); // button 2

#endif

  report_changes();        // Report updates to VRPN
  return 0;
}

void vrpn_3DMicroscribe::ConvertOriToQuat(float ori[3])
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
	
}

void vrpn_3DMicroscribe::report_changes(vrpn_uint32 class_of_service)
{
	vrpn_Button::timestamp = timestamp;
	vrpn_Tracker::timestamp = timestamp;

	vrpn_Button::report_changes();
	if (d_connection) {
		char	msgbuf[1000];
		int	len = vrpn_Tracker::encode_to(msgbuf);
		if (d_connection->pack_message(len, timestamp,
			position_m_id, d_sender_id, msgbuf,
			class_of_service)) {
				VRPN_MSG_ERROR("Tracker: cannot write message: tossing\n");
			}
	} else {
		VRPN_MSG_ERROR("Tracker: No valid connection\n");
	}
}

void vrpn_3DMicroscribe::report(vrpn_uint32 /*class_of_service*/)
{
	vrpn_Button::timestamp = timestamp;
	vrpn_Button::report_changes();
}

// This routine is called each time through the server's main loop. It will
// take a course of action depending on the current status of the 3DMicroscribe,
// either trying to reset it or trying to get a reading from it.
void	vrpn_3DMicroscribe::mainloop()
{
  server_mainloop();

  switch(status) {
    case STATUS_RESETTING:
	reset();
	break;

    case STATUS_SYNCING:
    case STATUS_READING:
	// Keep getting reports until all full reports are read.
	while (get_report()) {};
        break;

    default:
	fprintf(stderr,"vrpn_3DMicroscribe: Unknown mode (internal error)\n");
	break;
  }
}


