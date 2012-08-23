// $Header: /PDIvrpn.root/2.0.0/PDIVRPN/vrpn/vrpn_Tracker_G4.h 1     6/05/12 12:21p Ben $
#ifndef VRPN_TRACKER_PDI_H
#define VRPN_TRACKER_PDI_H

// See if we have defined the use of this code in vrpn_Configure.h or in CMake
#include "vrpn_Tracker.h"
#ifdef  VRPN_USE_PDI

#include "tchar.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "PDI.h"

#define BUFFER_SIZE 0x1FA400   // 30 seconds of xyzaer+fc 8 sensors at 240 hz

class VRPN_API vrpn_Tracker_G4: public vrpn_Tracker {
  public:
	vrpn_Tracker_G4 (const char *name, vrpn_Connection *cn, const char *filepath,
		vrpn_float64 Hz = 10.0, const char *rcmd = NULL);
   
	~vrpn_Tracker_G4(void);
	virtual void mainloop();
	int encode_to(char *buf);	
	BOOL Initialize ( VOID );
	BOOL Connect ( VOID );
	VOID Disconnect ( VOID );
	BOOL SetupDevice ( VOID );
	VOID UpdateStationMap ( VOID );
	BOOL DisplaySingle( timeval ct );
	BOOL StartCont ( VOID );
	BOOL StopCont ( VOID );	
	BOOL DisplayCont ( timeval ct );
	void sendCommand(char * scmd);
	void DoBoresightCmd(char *scmd);
	void DoFilterCmd(char *scmd);
	void SetFilterPreset( int nLev, CPDIfilter & f, char **pLevName );
	void DoFORCmd( char *scmd );
	void DoIncrementCmd( char *scmd );
	void DoTipOffsetCmd( char *scmd );

	void ParseG4NativeFrame( PBYTE pBuf, DWORD dwSize, timeval current_time );

  protected:
	vrpn_float64 update_rate;
	vrpn_float64 pos[3], quat[4];
	int status;
	char *cmd;	// additional commands for the tracker
	bool isCont;	// Keeps track of whether or not device is in continuous mode
	CPDIg4 pdiG4;	// PDI object for the device
	CPDImdat pdiMDat;	// Map of output format
	LPCTSTR srcCalPath;	// Filepath of the Source Calibration file
	BOOL bCnxReady;	// Keeps track of wheter the connection is active
	DWORD dwStationMap;	// map of the hubs and sensors
	ePDIoriUnits OriUnits;	// Orientaion Units e.g. Euler, Quaternion
	ePDIposUnits PosUnits;	// Positions Units e.g. Inches, Meters

	HANDLE  hContEvent;
	HWND	hwnd;
	DWORD	dwOverflowCount;
	BYTE	pMotionBuf[BUFFER_SIZE];
};

class VRPN_API vrpn_Tracker_FastrakPDI: public vrpn_Tracker {
  public:
	vrpn_Tracker_FastrakPDI (const char * name, vrpn_Connection * cn, 
		vrpn_float64 Hz = 10, const char * rcmd = NULL);
   
	~vrpn_Tracker_FastrakPDI(void);
	virtual void mainloop();
	int encode_to(char *buf);	
	BOOL Initialize ( VOID );
	BOOL Connect ( VOID );
	VOID Disconnect ( VOID );
	BOOL SetupDevice ( VOID );
	VOID UpdateStationMap ( VOID );
	VOID DisplaySingle ( timeval current_time );
	BOOL StartCont ( VOID );
	BOOL StopCont ( VOID );	
	BOOL DisplayCont ( timeval current_time );
	VOID SendCommand( char *scmd );

	VOID ParseFastrakFrame ( PBYTE pBuf, DWORD dwSize, timeval current_time );

  protected:
	vrpn_float64 update_rate;
	vrpn_float64 pos[3], d_quat[4];
	int status;
	char *cmd; // pointer to individual command, member of rcmd, for tracker
	BOOL isBinary; // keeps track of whether or not the device is in binary mode
	BOOL isCont; // Keeps track of whether or not device is in continuous mode
	BOOL isMetric; // tracks pos units
	CPDIfastrak	pdiDev; // PDI object for the device
	CPDImdat    pdiMDat; // Map of output format
	CPDIser		pdiSer; // Serial connection object for the device
	BOOL		bCnxReady; // Keeps track of whether the connection is active
	DWORD		dwStationMap; // Map of the hubs and sensors

	HANDLE  hContEvent;
	HWND	hwnd;
	DWORD	dwOverflowCount;
	BYTE	pMotionBuf[BUFFER_SIZE];
};

class VRPN_API vrpn_Tracker_LibertyPDI: public vrpn_Tracker {
  public:
	vrpn_Tracker_LibertyPDI (const char * name, vrpn_Connection * cn, 
		vrpn_float64 Hz = 10, const char * rcmd = NULL);
   
	~vrpn_Tracker_LibertyPDI(void);
	virtual void mainloop();
	int encode_to(char *buf);	
	BOOL Initialize ( VOID );
	BOOL Connect ( VOID );
	VOID Disconnect ( VOID );
	BOOL SetupDevice ( VOID );
	VOID UpdateStationMap ( VOID );
	VOID DisplaySingle ( timeval current_time );
	BOOL StartCont ( VOID );
	BOOL StopCont ( VOID );	
	BOOL DisplayCont ( timeval current_time );
	VOID SendCommand( char *scmd );

	VOID ParseLibertyFrame ( PBYTE pBuf, DWORD dwSize, timeval current_time );

  protected:
	vrpn_float64 update_rate;
	vrpn_float64 pos[3], d_quat[4];
	int status;
	char *cmd; // pointer to individual command, member of rcmd, for tracker
	BOOL isBinary; // keeps track of whether or not the device is in binary mode
	BOOL isCont; // Keeps track of whether or not device is in continuous mode
	BOOL isMetric; // tracks pos units
	CPDIdev		pdiDev; // PDI object for the device
	CPDImdat    pdiMDat; // Map of output format
	CPDIser		pdiSer; // Serial connection object for the device
	BOOL		bCnxReady; // Keeps track of whether the connection is active
	DWORD		dwStationMap; // Map of the hubs and sensors

	HANDLE  hContEvent;
	HWND	hwnd;
	DWORD	dwOverflowCount;
	BYTE	pMotionBuf[BUFFER_SIZE];
};

#endif
#endif
