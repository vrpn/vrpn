// $Header: /PDIvrpn.root/2.0.0/PDIVRPN/vrpn/vrpn_Tracker_G4.h 1     6/05/12 12:21p Ben $
#ifndef VRPN_TRACKER_PDI_H
#define VRPN_TRACKER_PDI_H

// See if we have defined the use of this code in vrpn_Configure.h or in CMake
#include "vrpn_Configure.h"
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"

#ifdef  VRPN_USE_PDI

#include "tchar.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>

#include "PDI.h"

#define VRPN_PDI_BUFFER_SIZE 0x1FA400   // 30 seconds of xyzaer+fc 8 sensors at 240 hz

#define VRPN_G4_HUB_NAME_SIZE 64
#define VRPN_G4_POWERTRAK_BUTTON_COUNT 4

//vrpn_Tracker_G4_HubMap_El and vrpn_Tracker_G4_HubMap
//classes are used in place of STL vector or list for listing
//ordered non-contiguous hub identifiers for management of
//digital i/o inputs on those hubs.
//These classes could easily be used for listing stylus
//button stations for other PDI trackers instead of the bitmap
//approach used now.  
class VRPN_API vrpn_Tracker_G4_HubMap_El
{
public:
	vrpn_Tracker_G4_HubMap_El( int nHub )
		: pBtnSrv(NULL), nHubID(nHub), pNext(NULL), nBtnCount(0) 
	{
		memset(BtnName, 0, sizeof(BtnName));
	};
	~vrpn_Tracker_G4_HubMap_El()
	{
          if (pBtnSrv) {
            try {
              delete pBtnSrv;
            } catch (...) {
              fprintf(stderr, "vrpn_Tracker_G4_HubMap_El::~vrpn_Tracker_G4_HubMap_El(): delete failed\n");
              return;
            }
          }
	}

	void ButtonName( char * NewName )
	{
		if (!NewName) {
                } else {
                        vrpn_strcpy(BtnName, NewName);
		}
	}

	void ButtonCount( int n )
	{
		nBtnCount = n;
	}


	vrpn_Tracker_G4_HubMap_El * Next()
	{ return pNext; }

	void SetNext( vrpn_Tracker_G4_HubMap_El *pn)
	{ pNext = pn; }

	friend class vrpn_Tracker_G4_HubMap;
	friend class vrpn_Tracker_G4;

private:
	int nHubID;
	vrpn_Button_Server * pBtnSrv;
	vrpn_Tracker_G4_HubMap_El * pNext;
	char BtnName[VRPN_G4_HUB_NAME_SIZE];
	int  nBtnCount;
};

typedef vrpn_Tracker_G4_HubMap_El HUBMAP_ENTRY;

class VRPN_API vrpn_Tracker_G4_HubMap
{
public:
	vrpn_Tracker_G4_HubMap():p_hub_map(NULL){};

	~vrpn_Tracker_G4_HubMap()
	{
		while (p_hub_map != NULL)
		{
			HUBMAP_ENTRY *next = p_hub_map->Next();
                        try {
                          delete p_hub_map;
                        } catch (...) {
                          fprintf(stderr, "vrpn_Tracker_G4_HubMap::~vrpn_Tracker_G4_HubMap(): delete failed\n");
                          return;
                        }
			p_hub_map = next;
		}
	}

	void Add( int nHub )
	{
		HUBMAP_ENTRY *next = p_hub_map;
                p_hub_map = new HUBMAP_ENTRY(nHub);
		p_hub_map->SetNext(next);
	}

	HUBMAP_ENTRY * Find( int nHub )
	{
		HUBMAP_ENTRY * pH = p_hub_map;
		while (pH && (pH->nHubID != nHub))
			pH = pH->Next();
		return pH;
	}

	void ButtonInfo( int nHub, char * BtnName, int nBtnCount )
	{
		HUBMAP_ENTRY * pHub = Find( nHub );
		if (pHub)
		{
			pHub->ButtonName( BtnName );
			pHub->ButtonCount( nBtnCount );
		}
	}

	vrpn_Tracker_G4_HubMap_El * Begin()
	{
		return p_hub_map;
	}

	HUBMAP_ENTRY * p_hub_map;
};

class VRPN_API vrpn_Tracker_G4: public vrpn_Tracker {
  public:
	vrpn_Tracker_G4 (const char *name, vrpn_Connection *cn, const char *filepath,
		vrpn_float64 Hz = 10.0, const char *rcmd = NULL, vrpn_Tracker_G4_HubMap * pHMap=NULL);
   
	~vrpn_Tracker_G4(void);
	virtual void mainloop();
	int encode_to(char *buf);	
	BOOL Initialize ( VOID );
	BOOL InitDigIOBtns( void );
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
	vrpn_Tracker_G4_HubMap * m_pHMap;

	HANDLE  hContEvent;
	HWND	hwnd;
	DWORD	dwOverflowCount;
	BYTE	pMotionBuf[VRPN_PDI_BUFFER_SIZE];
	PBYTE	pLastBuf;
	DWORD	dwLastSize;
};

class VRPN_API vrpn_Tracker_FastrakPDI: public vrpn_Tracker {
  public:
	vrpn_Tracker_FastrakPDI (const char * name, vrpn_Connection * cn, 
		vrpn_float64 Hz = 10, const char * rcmd = NULL, unsigned int nStylusMap = 0);
   
	~vrpn_Tracker_FastrakPDI(void);
	virtual void mainloop();
	int encode_to(char *buf);	
	BOOL Initialize ( VOID );
	BOOL InitStylusBtns();
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
	DWORD		m_nStylusMap; // Map of sensors with stylus
	int			m_nHeaderSize;// Num byte in P&O frame header
	int			m_nFrameSize; // Num bytes in entire P&O frame (inc header)
	vrpn_Button_Server	* FTstylusBtns[FT_MAX_SENSORS];	//< Pointer to button on each sensor (NULL if none)

	HANDLE  hContEvent;
	HWND	hwnd;
	DWORD	dwOverflowCount;
	BYTE	pMotionBuf[VRPN_PDI_BUFFER_SIZE];
	PBYTE	pLastBuf;
	DWORD	dwLastSize;
};

class VRPN_API vrpn_Tracker_LibertyPDI: public vrpn_Tracker {
  public:
	vrpn_Tracker_LibertyPDI (const char * name, vrpn_Connection * cn, 
		vrpn_float64 Hz = 10, const char * rcmd = NULL, unsigned int nStylusMap = 0);
   
	~vrpn_Tracker_LibertyPDI(void);
	virtual void mainloop();
	int encode_to(char *buf);	
	BOOL Initialize ( VOID );
	BOOL InitStylusBtns();
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
	DWORD		m_nStylusMap; // Map of sensors with stylus
	int			m_nHeaderSize;// Num byte in P&O frame header
	int			m_nFrameSize; // Num bytes in entire P&O frame (inc header)

	vrpn_Button_Server	* StylusBtns[LIBERTY_MAX_SENSORS];	//< Pointer to button on each sensor (NULL if none)

	HANDLE  hContEvent;
	HWND	hwnd;
	DWORD	dwOverflowCount;
	BYTE	pMotionBuf[VRPN_PDI_BUFFER_SIZE];
	PBYTE	pLastBuf;
	DWORD	dwLastSize;
};

#endif
#endif
