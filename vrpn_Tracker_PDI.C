// $Header: /PDIvrpn.root/2.0.0/PDIVRPN/vrpn/vrpn_Tracker_G4.cpp 1     6/05/12 12:21p Ben $
#include "vrpn_Tracker_PDI.h"

VRPN_SUPPRESS_EMPTY_OBJECT_WARNING()

#ifdef  VRPN_USE_PDI

using namespace std;

#define	CM_TO_METERS	(1/100.0)

// Constructor
vrpn_Tracker_G4::vrpn_Tracker_G4 (const char *name, vrpn_Connection *c, const char *filepath, vrpn_float64 Hz, const char *rcmd, vrpn_Tracker_G4_HubMap * pHMap) :
  vrpn_Tracker(name, c), update_rate(Hz), m_pHMap(pHMap)
  {
		srcCalPath = LPCTSTR(filepath);
	    cmd = (char*)(rcmd); // extra commands - See SendCommand(char *scmd)
		register_server_handlers();
		if(!(Initialize()))
		{
			cout<<"G4: Could not initialize\r\n";
			status = vrpn_TRACKER_FAIL;
		}
		else if (pHMap && !(InitDigIOBtns()))
		{
			cout<<"G4: Could not configure DigIO buttons\r\n";
			status = vrpn_TRACKER_FAIL;
		}
		else if(!(Connect()))
		{
			cout<<"G4: Could not connect\r\n";
			isCont = false;
			status = vrpn_TRACKER_FAIL;
		}
		else if(!(SetupDevice()))
		{
			cout<<"G4: Could not setup device\r\n";
			status = vrpn_TRACKER_FAIL;
		}
		else if(!(StartCont())){
			cout<<"G4: Failed to enter continuous mode\r\n";
			status = vrpn_TRACKER_FAIL;
		}
		else {
			cout<<"G4: Initialization Complete\r\n";
			status = vrpn_TRACKER_REPORT_READY;
		}
  }

// Deconstructor
vrpn_Tracker_G4::~vrpn_Tracker_G4(void){
	if(isCont)
		StopCont();
	Disconnect();

	if (m_pHMap)
	{
          try {
            delete m_pHMap;
          } catch (...) {
            fprintf(stderr, "vrpn_Tracker_G4::~vrpn_Tracker_G4(): delete failed\n");
            return;
          }
	}
}


// Called by the vrpn_Generic_Sever class in order to report its status.
void	vrpn_Tracker_G4::mainloop()
{
	struct timeval current_time;

	// Call the generic server mainloop routine, since this is a server
	server_mainloop();

	// See if its time to generate a new report
	vrpn_gettimeofday(&current_time, NULL);
	if ( vrpn_TimevalDuration(current_time,timestamp) >= 1000000.0/update_rate) {
		DisplayCont(current_time); // Sending a report is handled in ParseG4NativeFrame
	}
}

  // NOTE: you need to be sure that if you are sending vrpn_float64 then 
//       the entire array needs to remain aligned to 8 byte boundaries
//	 (malloced data and static arrays are automatically alloced in
//	  this way).  Assumes that there is enough room to store the
//	 entire message.  Returns the number of characters sent.
int	vrpn_Tracker_G4::encode_to(char *buf)
{
	char *bufptr = buf;
	int  buflen = 1000;

	// Message includes: long sensor, long scrap, vrpn_float64 pos[3], vrpn_float64 quat[4]
	// Byte order of each needs to be reversed to match network standard

	vrpn_buffer(&bufptr, &buflen, d_sensor);
	vrpn_buffer(&bufptr, &buflen, d_sensor); // This is just to take up space to align

	vrpn_buffer(&bufptr, &buflen, pos[0]);
	vrpn_buffer(&bufptr, &buflen, pos[1]);
	vrpn_buffer(&bufptr, &buflen, pos[2]);

	vrpn_buffer(&bufptr, &buflen, quat[0]);
	vrpn_buffer(&bufptr, &buflen, quat[1]);
	vrpn_buffer(&bufptr, &buflen, quat[2]);
	vrpn_buffer(&bufptr, &buflen, quat[3]);

	return 1000 - buflen;
}

//////////////////////////////// PDI Stuff /////////////////////////////////////////////////////

// Initialize the device and some variables
BOOL vrpn_Tracker_G4::Initialize(VOID){

	hContEvent = NULL;
	hwnd = NULL;
	dwOverflowCount = 0;

	BOOL bRet = TRUE;

	pdiG4.Trace(TRUE, 5);

	bCnxReady = FALSE;
	dwStationMap = 0;

	return bRet;
}


BOOL vrpn_Tracker_G4::InitDigIOBtns()
{
	BOOL bRet = TRUE;

	if (m_pHMap)
	{

		HUBMAP_ENTRY * pHub = m_pHMap->Begin();

		while (pHub)
		{
			if (pHub->nBtnCount)
			{
				try { pHub->pBtnSrv = new vrpn_Button_Server(pHub->BtnName, d_connection, pHub->nBtnCount ); }
				catch (...)
				{
					cout << "Cannot create button device " << pHub->BtnName << endl;
					bRet = FALSE;
					break;
				}
			}
			pHub = pHub->Next();
		}
	}

	return bRet;
}

// Connect to the G4 using the filepath provided in the config file.
BOOL vrpn_Tracker_G4::Connect(VOID)
{
	if (!(pdiG4.CnxReady()))
	{
		if(pdiG4.ConnectG4(srcCalPath)){
			cout<<"G4: Connected\r\n";
		}
	}
	else{
		cout<<"G4: Already Connected\r\n";
	}

	return pdiG4.CnxReady();
}

// End the connection to the G4
VOID vrpn_Tracker_G4::Disconnect(VOID)
{
    string msg;
    if (!(pdiG4.CnxReady()))
    {
        cout << "G4: Already disconnected\r\n";
    }
    else
    {
        pdiG4.Disconnect();
        cout<< "G4: Disconnected\r\n";
    }
    
}

// Set G4 to collect correct data, eg. cm and quaternions
BOOL vrpn_Tracker_G4::SetupDevice( VOID )
{
	int i = 0;
	pdiG4.GetStationMap( dwStationMap );
	while(dwStationMap == 0 && i<30)
	{ // Make sure that the G4 can see its hubs and sensors
		Sleep(50);// RF signals take a moment to work themselves out after connection
		pdiG4.GetStationMap( dwStationMap );
		i++;// Escape in the case that it can not find hubs 7 sensors
	}

	OriUnits = E_PDI_ORI_QUATERNION;
	pdiG4.SetPNOOriUnits( OriUnits );

	PosUnits = E_PDI_POS_METER; //ePDIposUnits(2);
	pdiG4.SetPNOPosUnits( PosUnits );

	pdiG4.SetPnoBuffer( pMotionBuf, VRPN_PDI_BUFFER_SIZE );
	pLastBuf = 0;
	dwLastSize = 0;
	
	//pdiG4.StartPipeExport();
	UpdateStationMap();

	if(strlen(cmd)>0)
	{
		bool end = true;
		char *pch;
		char *pcmd = cmd;
		pch = strtok (pcmd,"\n");
		while (pch != NULL)
		{
			pcmd += strlen(pch) + 1;
			sendCommand(pch);
			//printf ("%s\n",pch);
			pch = strtok (pcmd, "\n");
		}
	}

	//CPDIbiterr cBE;
	//pdiG4.GetBITErrs( cBE );

	//CHAR szc[100];
	//LPTSTR sz = LPTSTR(szc);
	//cBE.Parse( sz, 100 );

	//if (!(cBE.IsClear()))
	//{
	//	pdiG4.ClearBITErrs();
	//}

	UpdateStationMap();
	if(dwStationMap != 0)
	{
		return TRUE;
	}
	return FALSE; // G4 has connected but cannot see any hubs or sensors
}

// Updates the map of hubs and sensors
VOID vrpn_Tracker_G4::UpdateStationMap( VOID )
{
	pdiG4.GetStationMap( dwStationMap );
	printf("Set GetStationMap Result: %s\r\n", pdiG4.GetLastResultStr() );
}

// Send additional commands to the G4
//	Originally called during setup, but can be called anytime by client software
//	since it is a public function.
void vrpn_Tracker_G4::sendCommand(char *scmd)
{
	char command = scmd[0];
	printf("G4: Received Command: %s\n",scmd);
	switch(command)
	{
	case 'B':// Boresight
		DoBoresightCmd(&scmd[1]);
		break;
	case 'X'://Pos Filter
	case 'Y'://Att Filter
		DoFilterCmd(scmd);
		break;
	case 'T'://Translation FOR
	case 'R'://Rotation FOR
		DoFORCmd(scmd);
		break;
	case 'I'://Increment,AutoIncrement
		DoIncrementCmd(scmd);
		break;
	case 'N':
		DoTipOffsetCmd(scmd);
		break;
	case 'U':// Set Position Units
		//action = scmd[1] - 48;
		//if(action<4)
		//{
		//	PosUnits = ePDIposUnits(action);
		//	pdiG4.SetPNOPosUnits( PosUnits );

		//}
		//else
		//{
		//	printf("Improper Set Units Command\r\n");
		//}
		printf("\tIgnoring 'U' command: VRPN Position Units standard is Meters.\r\n");
		break;
	case 'O':// Set Orientation Units
		//action = scmd[1] - 48;
		//if(action<3)
		//{
		//	OriUnits = ePDIoriUnits(action);
		//	pdiG4.SetPNOOriUnits(OriUnits);
		//}
		//else
		//{
		//	printf("Improper Set Orientation Command\r\n");
		//}
		printf("\tIgnoring 'O' command: VRPN Orientation standard is Quaternion XYZW.\r\n");
		break;
	default:
		printf("\tUnrecognized Command: %c\r\n", scmd[1]);
		break;
	}
	Sleep(50);
}
#define CMD_ACTION_SET		1
#define CMD_ACTION_RESET	2
void vrpn_Tracker_G4::DoBoresightCmd(char *scmd)
{
	PDI4vec fP;
	PDIori threeVec = {0.0,0.0,0.0};		// default Euler 
	PDI4vec fourVec = {1.0,0.0,0.0,0.0};	// default Quaternion WXYZ for G4

	char delims[] = ",\n";
	char eol[] = "\r\n";
	char comma[] = ",";
	char *pArgs = scmd;
	//strcpy(pArgs, scmd);

	char *pAct=NULL;
	char *pHub=NULL; 
	char *pSens=NULL;
	char *pRef=NULL;
	bool bValid = TRUE;

	int nParams=0;
	int	nAction;  
	int nHub;
	int nSensor;

	pAct = strtok(pArgs,comma);
	if (pAct == NULL)
	{ 
		bValid = FALSE;
	}
	else
	{
		nAction = pAct[0] - '0';
		nParams++;
		pHub = strtok(NULL,comma);
		if (pHub == NULL )
		{
			bValid = FALSE;
		}
		else
		{
			nHub = (pHub[0] == '*') ? -1 : atoi(pHub);
			nParams++;
			pSens = strtok(NULL, delims);
			if (pSens == NULL)
			{
				bValid = FALSE;
			}
			else
			{
				nSensor = (pSens[0] == '*') ? -1 : atoi(pSens);
				nParams++;
				pRef = strtok(NULL, eol);
				if (pRef != NULL)
				{
					nParams += sscanf( pRef, "%f,%f,%f,%f", &fP[1],&fP[2],&fP[3],&fP[0] );
				}
			}
		}
	}
	   
	if (!bValid)
	{
		printf("\tERROR: Invalid Boresight Command Syntax : B%s\r\n", scmd);
	}
	else
	{
		switch (nAction)
		{
		case (CMD_ACTION_SET):
			if (nParams == 7)
			{
				pdiG4.SetSBoresight(nHub, nSensor, fP);
				printf("\tSet Boresight Result: %s\r\n", pdiG4.GetLastResultStr() );
			}
			else if (nParams == 3)
			{
				pdiG4.SetSBoresight(nHub, nSensor, fourVec);
				printf("\tSet Boresight Result: %s\r\n", pdiG4.GetLastResultStr() );
			}
			else
			{
				printf("\tERROR: Unexpected Boresight Argument count: %d\r\n", nParams);
			}
			break;
		case (CMD_ACTION_RESET):
			pdiG4.ResetSBoresight(nHub, nSensor);
			printf("\tReset Boresight Result: %s\r\n", pdiG4.GetLastResultStr() );
			break;
		default:
			printf("\tERROR: Unrecognized Boresight Action: %s\r\n", pAct);
			break;
		}
	}
}


void vrpn_Tracker_G4::DoFilterCmd(char *scmd)
{
	char delims[] = ",\n\r\\";
	char eol[] = "\r\n";
	char comma[] = ",";
	char * pArgs = &scmd[1];
	char cmd = scmd[0];

	char *pAct=NULL;
	char *pHub=NULL; 
	char *pLev=NULL;
	char *pCust=NULL;
	bool bValid = TRUE;

	int nParams=0;
	int	nAction;  
	int nHub;
	int nLev;
	float F=0, FLow=0, FHigh=0, Factor=0;
	CPDIfilter f;
	char * pLevelName = 0;

	pAct = strtok(pArgs,comma);
	if (pAct == NULL)
	{ 
		bValid = FALSE;
	}
	else
	{
		nAction = pAct[0] - '0';
		nParams++;

		pHub = strtok(NULL,delims);
		if (pHub == NULL )
		{
			bValid = FALSE;
		}
		else
		{
			nHub = (pHub[0] == '*') ? -1 : atoi(pHub);
			nParams++;

			if (nAction == CMD_ACTION_SET)
			{
				pLev = strtok(NULL, delims);
				if (pLev == NULL)
				{
					bValid = FALSE;
				}
				else
				{
					nLev = atoi(pLev);
					nParams++;

					if (nLev == E_PDI_FILTER_CUSTOM)
					{
						pCust = strtok(NULL, eol);
						if (pCust == NULL)
						{
							bValid = FALSE;
						}
						else
						{
							nParams += sscanf( pCust, "%f,%f,%f,%f", &f.m_fSensitivity,
																	 &f.m_fLowValue,
																	 &f.m_fHighValue,
																	 &f.m_fMaxTransRate );
							if (nParams != 7)
							{
								printf("\tERROR: Unexpected Filter Argument count: %d\r\n", nParams);
								bValid = FALSE;
							}
						}
					}
				}
			}
		}
	}
	   
	if (!bValid)
	{
		printf("\tERROR: Invalid Filter Command Syntax: %s\r\n", scmd);
	}
	else
	{
		if (nLev == E_PDI_FILTER_NONE)
		{
			nAction = CMD_ACTION_RESET;
		}

		switch (nAction)
		{
		case (CMD_ACTION_SET):
			SetFilterPreset( nLev, f, &pLevelName);
			if (cmd == 'X')
			{
				pdiG4.SetHPosFilter( nHub, f);
			}
			else
			{
				pdiG4.SetHAttFilter( nHub, f);
			}
			printf("\tSet Filter Cmd %c %s Result: %s\r\n", cmd, pLevelName, pdiG4.GetLastResultStr() );
			break;

		case (CMD_ACTION_RESET):
			if (cmd == 'X')
			{
				pdiG4.ResetHPosFilter( nHub );
			}
			else
			{
				pdiG4.ResetHAttFilter( nHub );
			}
			printf("\tReset Filter Cmd %c Result: %s\r\n", cmd, pdiG4.GetLastResultStr() );
			break;

		default:
			printf("\tERROR: Unrecognized Filter Action: %c\r\n", *pAct);
			break;
		}

	}	
}

CPDIfilter FilterPresets[4] = 
{
	CPDIfilter(0.0f,  1.0f,  0.0f, 0.0f),	// none
	CPDIfilter(0.2f,  0.2f,  0.8f, 0.95f),	// low,
	CPDIfilter(0.05f, 0.05f, 0.8f, 0.95f),	// med,
	CPDIfilter(0.02f, 0.02f, 0.8f, 0.95f)	// heavy
};
char * PresetNames[5] = 
{
	"NONE", "LIGHT", "MEDIUM", "HEAVY", "CUSTOM"
};

void vrpn_Tracker_G4::SetFilterPreset( int nLev, CPDIfilter & f, char **pLevName )
{
	switch (nLev)
	{
	case E_PDI_FILTER_NONE:	
	case E_PDI_FILTER_LIGHT:
	case E_PDI_FILTER_MED:
	case E_PDI_FILTER_HEAVY:
		f = FilterPresets[nLev];
		*pLevName = PresetNames[nLev];
		break;
	case E_PDI_FILTER_CUSTOM:
		*pLevName = PresetNames[nLev];
		break;
	default:
		f = FilterPresets[E_PDI_FILTER_HEAVY];
		*pLevName = PresetNames[E_PDI_FILTER_HEAVY];
		break;
	}
}

void vrpn_Tracker_G4::DoFORCmd( char *scmd )
{
	char delims[] = ",\n\r\\";
	char eol[] = "\r\n";
	char comma[] = ",";
	char * pArgs = &scmd[1];
	char cmd = scmd[0];

	char *pAct=NULL;
	char *pFOR=NULL;
	bool bValid = TRUE;

	int nParams=0;
	int nParamSpec=0;
	int	nAction;  
	PDIpos pos;
	PDIqtrn qtrn;

	pAct = strtok(pArgs,comma);
	if (pAct == NULL)
	{ 
		bValid = FALSE;
	}
	else
	{
		nAction = pAct[0] - '0';
		nParams++;

		if (nAction == CMD_ACTION_SET)
		{
			pFOR = strtok( NULL, eol );
			if (pFOR == NULL)
			{
				bValid = FALSE;
			}
			else if (cmd == 'T')
			{
				nParams += sscanf( pFOR, "%f,%f,%f", &pos[0], &pos[1], &pos[2] );
				nParamSpec = 4;
			}
			else
			{
				nParams += sscanf( pFOR, "%f,%f,%f,%f", &qtrn[1], &qtrn[2], &qtrn[3], &qtrn[0] );
				nParamSpec = 5;
			}

			if (nParams != nParamSpec)
			{
				printf("\tERROR: Unexpected Frame of Reference %c Argument count: %d\r\n", cmd, nParams);
				bValid = FALSE;
			}
		}
	}
	   
	if (!bValid)
	{
		printf("\tERROR: Invalid Frame of Reference Command Syntax: %s\r\n", scmd);
	}
	else
	{
		switch (nAction)
		{
		case (CMD_ACTION_SET):
			if (cmd == 'T')
			{
				pdiG4.SetFrameOfRefTRANS( pos );
			}
			else
			{
				pdiG4.SetFrameOfRefROT( qtrn );
			}
			printf("\tSet Frame of Ref %c Result: %s\r\n", cmd, pdiG4.GetLastResultStr() );
			break;

		case (CMD_ACTION_RESET):
			if (cmd == 'T')
			{
				pdiG4.ResetFrameOfRefTRANS();
			}
			else
			{
				pdiG4.ResetFrameOfRefROT();
			}
			printf("\tReset Frams of Ref Cmd %c Result: %s\r\n", cmd, pdiG4.GetLastResultStr() );
			break;

		default:
			printf("\tERROR: Unrecognized Frame Of Reference Action: %c\r\n", *pAct);
			break;
		}

	}	

}



void vrpn_Tracker_G4::DoIncrementCmd(char *scmd)
{
	char cmd = scmd[0];

	char delims[] = " ,\r\n\\";
	char eol[] = "\r\n";
	char comma[] = ",";
	char *pArgs = &scmd[1];
	//strcpy(pArgs, scmd);

	char *pAct=NULL;
	char *pHub=NULL; 
	char *pSens=NULL;
	char *pIncrs=NULL;
	bool bValid = TRUE;

	int nParams=0;
	int nParamSpec=5;
	int	nAction;  
	int nHub;
	int nSensor;
	float fPosIncr, fOriIncr;


	pAct = strtok(pArgs,comma);
	if (pAct == NULL)
	{ 
		bValid = FALSE;
	}
	else
	{
		nAction = pAct[0] - '0';
		nParams++;
		pHub = strtok(NULL,comma);
		if (pHub == NULL )
		{
			bValid = FALSE;
		}
		else
		{
			nHub = (pHub[0] == '*') ? -1 : atoi(pHub);
			nParams++;
			pSens = strtok(NULL, delims);
			if (pSens == NULL)
			{
				bValid = FALSE;
			}
			else
			{
				nSensor = (pSens[0] == '*') ? -1 : atoi(pSens);
				nParams++;

				if (nAction == CMD_ACTION_SET)
				{
					pIncrs = strtok(NULL, eol);
					if (pIncrs == NULL)
					{
						bValid = FALSE;
					}
					else
					{
						nParams += sscanf( pIncrs, "%f,%f", &fPosIncr, &fOriIncr );
					}

					if (nParams != nParamSpec)
					{
						printf("\tERROR: Unexpected Increment Cmd Argument count: %d\r\n", nParams);
						bValid = FALSE;
					}
				}
			}
		}
	}
	   
	if (!bValid)
	{
		printf("\tERROR: Invalid Increment Command Syntax : %s\r\n", scmd);
	}
	else
	{
		pdiG4.SetPNOOriUnits(E_PDI_ORI_EULER_DEGREE);
		printf("\tSet Ori Units DEGREES Result: %s\r\n", pdiG4.GetLastResultStr() );
		switch (nAction)
		{
		case (CMD_ACTION_SET):
			pdiG4.SetSIncrement(nHub, nSensor, fPosIncr, fOriIncr);
			printf("\tSet Increment Result: %s\r\n", pdiG4.GetLastResultStr() );
			break;

		case (CMD_ACTION_RESET):
			pdiG4.ResetSIncrement(nHub, nSensor);
			printf("\tReset Increment Result: %s\r\n", pdiG4.GetLastResultStr() );
			break;

		default:
			printf("\tERROR: Unrecognized Increment Action: %s\r\n", pAct);
			break;
		}
		pdiG4.SetPNOOriUnits(E_PDI_ORI_QUATERNION);
		printf("\tSet Ori Units QUATERNION Result: %s\r\n", pdiG4.GetLastResultStr() );
	}
}



void vrpn_Tracker_G4::DoTipOffsetCmd(char *scmd)
{
	char cmd = scmd[0];

	char delims[] = " ,\n\r\\";
	char eol[] = "\r\n";
	char comma[] = ",";
	char *pArgs = &scmd[1];
	//strcpy(pArgs, scmd);

	char *pAct=NULL;
	char *pHub=NULL; 
	char *pSens=NULL;
	char *pTO=NULL;
	bool bValid = TRUE;

	int nParams=0;
	int nParamSpec=6;
	int	nAction;  
	int nHub;
	int nSensor;
	PDIpos pos;


	pAct = strtok(pArgs,comma);
	if (pAct == NULL)
	{ 
		bValid = FALSE;
	}
	else
	{
		nAction = pAct[0] - '0';
		nParams++;
		pHub = strtok(NULL,comma);
		if (pHub == NULL )
		{
			bValid = FALSE;
		}
		else
		{
			nHub = (pHub[0] == '*') ? -1 : atoi(pHub);
			nParams++;
			pSens = strtok(NULL, delims);
			if (pSens == NULL)
			{
				bValid = FALSE;
			}
			else
			{
				nSensor = (pSens[0] == '*') ? -1 : atoi(pSens);
				nParams++;

				if (nAction == CMD_ACTION_SET)
				{
					pTO = strtok(NULL, eol);
					if (pTO == NULL)
					{
						bValid = FALSE;
					}
					else
					{
						nParams += sscanf( pTO, "%f,%f,%f", &pos[0], &pos[1], &pos[2] );
					}

					if (nParams != nParamSpec)
					{
						printf("\tERROR: Unexpected Tip Offset Cmd Argument count: %d\r\n", nParams);
						bValid = FALSE;
					}
				}
			}
		}
	}
	   
	if (!bValid)
	{
		printf("\tERROR: Invalid Tip Offset Command Syntax : %s\r\n", scmd);
	}
	else
	{
		switch (nAction)
		{
		case (CMD_ACTION_SET):
			pdiG4.SetSTipOffset(nHub, nSensor, pos);
			printf("\tSet Tip Offset Result: %s\r\n", pdiG4.GetLastResultStr() );
			break;
		case (CMD_ACTION_RESET):
			pdiG4.ResetSTipOffset(nHub, nSensor);
			printf("\tReset Tip Offset Result: %s\r\n", pdiG4.GetLastResultStr() );
			break;
		default:
			printf("\tERROR: Unrecognized Tip Offset Action: %s\r\n", pAct);
			break;
		}
	}
}




// Start Continuous Mode for the G4
BOOL vrpn_Tracker_G4::StartCont( VOID )
{
	cout<<"G4: Start Continuous Mode\r\n";
	BOOL bRet = FALSE;


	if (!(pdiG4.StartContPnoG4(hwnd)))
	{
	}
	else
	{
		dwOverflowCount = 0;
		bRet = TRUE;
	}

	isCont = true;
	return bRet;
}

// Stops Continuous Mode for the G4
BOOL vrpn_Tracker_G4::StopCont( VOID )
{
	cout<<"G4: Stop Continuous Mode\r\n";
	BOOL bRet = FALSE;

	if (!(pdiG4.StopContPnoG4()))
	{
	}
	else
	{
		bRet = TRUE;
		Sleep(1000);
	}

	isCont = false;
	::ResetEvent(hContEvent);
	return bRet;
}

// Displays a frame of information taken from the continuous stream of 
// Continuous Mode by calling ParseG4NativeFrame - not functioning now
BOOL vrpn_Tracker_G4::DisplayCont( timeval ct )
{
	//cout<<"DisplayCont\r\n";
	BOOL bRet = FALSE;

	PBYTE pBuf;
	DWORD dwSize;

	if (!(pdiG4.LastPnoPtr(pBuf, dwSize)))
	{
	}
	else if ((pBuf == 0) || (dwSize == 0))
	{
	}
	else if (pLastBuf && (pBuf > pLastBuf))
	{
		ParseG4NativeFrame( pLastBuf+dwLastSize, dwSize+(pBuf-pLastBuf-dwLastSize), ct );
		pLastBuf = pBuf;
		dwLastSize = dwSize;
		bRet = TRUE;
	}
	else if (pBuf != pLastBuf) // it wrapped in buffer
	{
		if (pLastBuf)
			cout << "wrapped" << endl;

		pLastBuf = pBuf;
		dwLastSize = dwSize;
		ParseG4NativeFrame( pBuf, dwSize, ct );
		bRet = TRUE;
	}

	//cout<<"Leaving DisplayCont\r\n";
	return bRet;
}

// Displays a single frame of information from the G4 by collecting data
//	and calling ParseG4NativeFrame
BOOL vrpn_Tracker_G4::DisplaySingle( timeval ct )
{
	BOOL bRet = FALSE;
	PBYTE pBuf;
	DWORD dwSize;

	if (!(pdiG4.ReadSinglePnoBufG4(pBuf, dwSize)))
	{
		//cout<<"ReadSinglePno\r\n";
	}
	else if ((pBuf == 0) || (dwSize == 0))
	{
	}
	else 
	{
		ParseG4NativeFrame( pBuf, dwSize, ct );
		bRet = TRUE;
	}

	return bRet;

}

// Parses the data collected from a P or C command, packages it and sends it out to clients
// calling for it
void vrpn_Tracker_G4::ParseG4NativeFrame( PBYTE pBuf, DWORD dwSize, timeval current_time )
{
	DWORD dw= 0;
	char msgbuf[1000];
	vrpn_int32 len;
	LPG4_HUBDATA pHubFrame;

	while (dw < dwSize )
	{
		pHubFrame = (LPG4_HUBDATA)(&pBuf[dw]);

		dw += sizeof(G4_HUBDATA);

		UINT	nHubID = pHubFrame->nHubID;
		UINT	nFrameNum =  pHubFrame->nFrameCount;
		UINT	nSensorMap = pHubFrame->dwSensorMap;
		UINT	nDigIO = pHubFrame->dwDigIO;

		HUBMAP_ENTRY * pH = 0;
		int nButtons = 0;
		// handle digios if necessary
		if (m_pHMap && (pH = m_pHMap->Find( nHubID )) && (nButtons = pH->nBtnCount) )
		{
			for (int i=0; i<nButtons; i++)
			{
				pH->pBtnSrv->set_button(i, (nDigIO & (1<<i)) >> i);
				pH->pBtnSrv->mainloop();
			}
		}

		UINT	nSensMask = 1;

		for (int j=0; j<G4_MAX_SENSORS_PER_HUB; j++)
		{
			if (((nSensMask << j) & nSensorMap) != 0)
			{
				G4_SENSORDATA * pSD = &(pHubFrame->sd[j]);
				d_sensor = (nHubID*10)+j;
				// transfer the data from the G4 data array to the vrpn_Tracker array
				pos[0] = pSD->pos[0]; 
				pos[1] = pSD->pos[1]; 
				pos[2] = pSD->pos[2];

				quat[0] = pSD->ori[1]; 
				quat[1] = pSD->ori[2]; 
				quat[2] = pSD->ori[3]; 
				quat[3] = pSD->ori[0];
				
				// Grab the current time and create a timestamp
				timestamp.tv_sec = current_time.tv_sec;
				timestamp.tv_usec = current_time.tv_usec;
				// check the connection and then send a message out along it.
				if (d_connection) {
					// Pack position report
					len = encode_to(msgbuf);
					d_connection->pack_message(len, timestamp,
						position_m_id, d_sender_id, msgbuf,
						vrpn_CONNECTION_LOW_LATENCY);
				}
			}
		}

	
	
	} // end while dwsize
}

// Constructor
vrpn_Tracker_FastrakPDI::vrpn_Tracker_FastrakPDI (const char * name, vrpn_Connection *cn,
	           vrpn_float64 Hz, const char * rcmd, unsigned int nStylusMap) :
  vrpn_Tracker(name, cn), update_rate(Hz) 
	, m_nStylusMap(nStylusMap)
	, m_nHeaderSize(3)
	, m_nFrameSize(nStylusMap?33:31)
  {
	    cmd = (char*)(rcmd);
		register_server_handlers();
		if(!(Initialize())){
			status = vrpn_TRACKER_FAIL;
		}
		else if (nStylusMap & !(InitStylusBtns()))
		{
			status = vrpn_TRACKER_FAIL;
		}
		else if(!(Connect())){
			status = vrpn_TRACKER_FAIL;
		}
		else if(!(SetupDevice())){
			status = vrpn_TRACKER_FAIL;
			cout << "FasTrakPDI: Device setup failed\r\n";
		}
		else if(!(StartCont())){
			isCont = FALSE;
			cout << "FastrakPDI: Failed to enter continuous mode\r\n";
			status = vrpn_TRACKER_FAIL;
		}
		else {
			cout << "FastrakPDI: Initialization Complete\r\n";
			status = vrpn_TRACKER_REPORT_READY;
		}
  }

// Deconstructor
vrpn_Tracker_FastrakPDI::~vrpn_Tracker_FastrakPDI(void)
{
	  if(isCont)
		  StopCont();
	  Disconnect();

	  for (int i=0; i<FT_MAX_SENSORS; i++)
	  {
            if (FTstylusBtns[i]) {
              try {
                delete FTstylusBtns[i];
              } catch (...) {
                fprintf(stderr, "vrpn_Tracker_FastrakPDI::~vrpn_Tracker_FastrakPDI(): delete failed\n");
                return;
              }
            }
	  }
  }

// Called by the vrpn_Generic_Sever class in order to report its status.
VOID vrpn_Tracker_FastrakPDI::mainloop()
{
	struct timeval current_time;

	// Call the generic server mainloop routine, since this is a server
	server_mainloop();

	// See if its time to generate a new report
	vrpn_gettimeofday(&current_time, NULL);
	if ( vrpn_TimevalDuration(current_time,timestamp) >= 1000000.0/update_rate) {
		DisplayCont(current_time);
	}
}

// NOTE: you need to be sure that if you are sending vrpn_float64 then 
//       the entire array needs to remain aligned to 8 byte boundaries
//	     (malloced data and static arrays are automatically alloced in
//	     this way).  Assumes that there is enough room to store the
//	     entire message.  Returns the number of characters sent.
int	vrpn_Tracker_FastrakPDI::encode_to(char *buf)
{
   char *bufptr = buf;
   int  buflen = 1000;

   // Message includes: long sensor, long scrap, vrpn_float64 pos[3], vrpn_float64 quat[4]
   // Byte order of each needs to be reversed to match network standard

   vrpn_buffer(&bufptr, &buflen, d_sensor);
   vrpn_buffer(&bufptr, &buflen, d_sensor); // This is just to take up space to align

   vrpn_buffer(&bufptr, &buflen, pos[0]);
   vrpn_buffer(&bufptr, &buflen, pos[1]);
   vrpn_buffer(&bufptr, &buflen, pos[2]);

   vrpn_buffer(&bufptr, &buflen, d_quat[0]);
   vrpn_buffer(&bufptr, &buflen, d_quat[1]);
   vrpn_buffer(&bufptr, &buflen, d_quat[2]);
   vrpn_buffer(&bufptr, &buflen, d_quat[3]);

   return 1000 - buflen;
}


// Initialize the device and some variables
BOOL vrpn_Tracker_FastrakPDI::Initialize(VOID)
{
	BOOL	bRet = TRUE;
	
	pos[0]=0; pos[1]=0;	pos[2]=0;
	d_quat[0]=0; d_quat[1]=0; d_quat[2]=0; d_quat[3]=0;

	memset( FTstylusBtns, 0, sizeof(FTstylusBtns));

	hContEvent = NULL;
	hwnd = NULL;
	dwOverflowCount = 0;


	pdiDev.Trace(TRUE, 5); // Report debugging information to IDE output

	bCnxReady = FALSE;
	dwStationMap = 0;

	return bRet;
}

BOOL vrpn_Tracker_FastrakPDI::InitStylusBtns()
{
	BOOL bRet = TRUE;
	int mask = 1;
	for (int i=0; i<FT_MAX_SENSORS; i++)
	{
		if (((1<<i) & m_nStylusMap) != 0)
		{
			char btnName[512];
			sprintf( btnName, "%sStylus%d", d_servicename, i+1);
			try { FTstylusBtns[i] = new vrpn_Button_Server( btnName, d_connection, 1 ); }
			catch (...)
			{
				cout << "Cannot create button device " << btnName << endl;
				bRet = FALSE;
			}
			else
			{
				cout << "Button device " << btnName << " created." << endl;
			}
		}
	}
	return bRet;
}
// Connect to the Fastrak using the filepath provided in the config file
// Sets tracker to default VRPN units and frames
BOOL vrpn_Tracker_FastrakPDI::Connect( VOID )
{
    if (!(pdiDev.CnxReady()))
    {
		pdiDev.SetSerialIF( &pdiSer );

        ePiCommType eType;
		
		eType = pdiDev.DiscoverCnx();
		DWORD attempts = 0;
		while (eType != PI_CNX_USB && eType != PI_CNX_SERIAL && attempts < 10){
			switch (eType)
			{
			case PI_CNX_USB:
				cout << "FastrakPDI: USB Connection\r\n";
				status = vrpn_TRACKER_SYNCING;
				break;
			case PI_CNX_SERIAL:
				cout << "FastrakPDI: Serial Connection\r\n";
				status = vrpn_TRACKER_SYNCING;
				break;
			default:
				printf("FastrakPDI: %s\r\n", pdiDev.GetLastResultStr() );
				eType = pdiDev.DiscoverCnx();
				attempts++;
				break;
			}
			Sleep(3000);
		}

		UpdateStationMap();
		num_sensors = pdiDev.StationCount();

		CPDIbiterr cBE;
		pdiDev.GetBITErrs( cBE );

		CHAR sz[100];
		cBE.Parse( sz, 100 );

		if(!(cBE.IsClear()))
			pdiDev.ClearBITErrs();
	
		// Set VRPN defaults for position, orientation and frame structure
		//m_nFrameSize = 28;
		pdiMDat.Empty();
		if (m_nStylusMap)
		{
			pdiMDat.Append(PDI_MODATA_STYLUS);
		}
		pdiMDat.Append( PDI_MODATA_POS );
		pdiMDat.Append( PDI_MODATA_QTRN );
		pdiDev.SetSDataList( -1, pdiMDat );
		pdiDev.SetMetric(TRUE);
		isMetric = TRUE;
		isBinary = TRUE;

		pdiDev.SetPnoBuffer( pMotionBuf, VRPN_PDI_BUFFER_SIZE );
		pLastBuf = 0;
		dwLastSize = 0;

		bCnxReady = pdiDev.CnxReady();
	}
	else{
		cout << "FastrakPDI: Already connected\r\n";
		bCnxReady = TRUE;
	}

	return bCnxReady;
}

// End the connection to the Fastrak
VOID vrpn_Tracker_FastrakPDI::Disconnect(VOID)
{
    string msg;
    if (!(pdiDev.CnxReady()))
    {
        cout << "FastrakPDI: Already disconnected\r\n";
    }
    else
    {
        pdiDev.Disconnect();
        cout << "FastrakPDI: Disconnected\r\n";
    }
    
}

// Parses rcmd and calls SendCommand for each line
BOOL vrpn_Tracker_FastrakPDI::SetupDevice( VOID )
{
	char * pcmd = cmd;
	if(strlen(pcmd) > 0){
		char * pch = strchr(pcmd,'\\');
		while (pch != NULL){
			pch[0] = '\0';
			SendCommand(pcmd);
			pcmd = pch + sizeof(char);
			pch = strchr(pcmd,'\\');
		}
		SendCommand(pcmd);
	}
	
	// warn the user if tracker is in ASCII mode
	if (isBinary == FALSE)
		cout << "FastrakPDI: Warning! Tracker is still in ASCII mode!\r\n";

	return TRUE;
}

// Updates the map of hubs and sensors
VOID vrpn_Tracker_FastrakPDI::UpdateStationMap( VOID )
{
	pdiDev.GetStationMap( dwStationMap );
}

// Sends commands to the tracker, the syntax is explained in vrpn_FastrakPDI.cfg
VOID vrpn_Tracker_FastrakPDI::SendCommand(char *scmd){	
	char szCmd[PI_MAX_CMD_BUF_LEN] = "\0";
	DWORD dwRspSize = 5000;
	char szRsp[5000] = "\0";
	bool parseErr = false;
	
	// print scmd to server screen
	cout << "FastrakPDI: reset command ";
	unsigned int i = 0;
	while (scmd[i] != '\0'){
		// ignore carriage returns and new lines (cleans up output)
		if (scmd[i] != '\n' && scmd[i] != '\r')
			cout << scmd[i];
		i++;
	}

	for (i = 0; i < strlen(scmd); i++){
		switch (scmd[i]){
			case '^': // convert ^ to control+char ascii equivalent
				i++;
				switch (scmd[i]){
					case 'K':
						strncat(szCmd, "\x0b", 1);
						break;
					case 'Q':
						strncat(szCmd, "\x11", 1);
						break;
					case 'S':
						strncat(szCmd, "\x13", 1);
						break;
					case 'Y':
						strncat(szCmd, "\x19", 1);
						break;
					default: // no match, flip parseErr
						i = strlen(scmd);
						parseErr = true;
				}
				break;
			case '<': // check for <>, which represents a carriage return
				i++;
				if (scmd[i] != '>'){
					i = strlen(scmd);
					parseErr = true;
				}
				else
					strncat(szCmd, "\r\n", 1);
				break;
			case '\r': // ignore carriage returns.
				break;
			case '\n': // ignore new lines
				break;
			case '\xff': // ignore eof
				break;
			default:
				strncat(szCmd, &scmd[i], 1);
		}
	}
	
	// CPDIdev::TxtCmd() and CPDIdev::SetBinary() functions are not exposed in CPDIfastrak
	// but can be called from the base class (CPDIdev)

	// Typecasts pdiDev as a CPDIdev in pointer pDev
	CPDIdev * pDev = (CPDIdev*) (&pdiDev);

	if (parseErr == true)
		cout << "\r\n\t<unrecognized command>\r\n";
	else{
		strncat(szCmd, "\0", 1);
		switch (szCmd[0]){
			case 'C': // Continuous pno command
			case 'c': // Ignored since this would conflict with VRPN directly
				cout << "\r\n\t<command ignored, use P instead>\r\n";
				break;
			case 0x19: // reset command
				pDev->TxtCmd(szCmd,dwRspSize,szRsp);
				cout << "\r\n\t<tracker will be set to VRPN defaults on reconnect>\r\n";
				Disconnect();
				Sleep(2500);
				Connect();
				break;
			case 'U': // units command
			case 'u':
				pDev->TxtCmd(szCmd,dwRspSize,szRsp);
				if (isBinary == TRUE)
					pdiDev.GetMetric(isMetric);
				else{
					// if tracker is in ASCII mode, switch to binary for GetMetric function
					pDev->SetBinary(TRUE);
					pdiDev.GetMetric(isMetric);
					pDev->SetBinary(FALSE);
				}
				if (isMetric == FALSE)
					cout << "\r\n\t<position units set to inches>\r\n";
				else
					cout << "\r\n\t<position units set to meters>\r\n";
				break;
			case 'F': // format (binary or ascii) command
			case 'f':
				pDev->TxtCmd(szCmd,dwRspSize,szRsp);
				pdiDev.GetBinary(isBinary);
				if (isBinary == FALSE)
					cout << "\r\n\t<response frames set to ASCII>\r\n";
				else
					cout << "\r\n\t<response frames set to binary>\r\n";
				break;
			case 'O': // data list format command
				cout << "\r\n\t<pno frame format changed (use extreme caution)>";
			case 'P': // print single pno command
				if (isBinary == TRUE)
					cout << "\r\n\t<suggestion: use ASCII response frames (reset command F)>";
			default:
				pDev->TxtCmd(szCmd,dwRspSize,szRsp);
				if (dwRspSize != 0 && dwRspSize != 5000){
					if (isBinary == TRUE)
						cout << "\r\n\t<binary response bgn>\r\n\t";
					else
						cout << "\r\n\t<ASCII response bgn>\r\n\t";
					for (i = 0; i < dwRspSize; i++){
						if (szRsp[i] != '\r')
							printf("%c",szRsp[i]);
						if (szRsp[i] == '\n')
							printf("\t");
					}
					if (isBinary == TRUE)
						cout << "\r\n\t<binary response end>\r\n";
					else
						cout << "\r\n\t<ASCII response end>\r\n";
				}
				else
					cout << "\r\n\t<command sent>\r\n";
		}
	}
	//Sleep(500);
}

// Start Continuous Mode for the Fastrak
BOOL vrpn_Tracker_FastrakPDI::StartCont( VOID )
{
	cout << "FastrakPDI: Start Continuous Mode\r\n";
	BOOL bRet = FALSE;


	if (!(pdiDev.StartContPno(hwnd)))
	{
	}
	else
	{
		isCont = TRUE;
		dwOverflowCount = 0;
		bRet = TRUE;
	}

	return bRet;
}

// Stops Continuous Mode for the Fastrak
BOOL vrpn_Tracker_FastrakPDI::StopCont( VOID )
{
	cout << "FastrakPDI: Stop Continuous Mode\r\n";
	BOOL bRet = FALSE;

	if (!(pdiDev.StopContPno()))
	{
	}
	else
	{
		isCont = FALSE;
		bRet = TRUE;
		Sleep(1000);
	}

	::ResetEvent(hContEvent);
	return bRet;
}

// Displays a frame of information taken from the continuous stream of 
// Continuous Mode by calling displayFrame
BOOL vrpn_Tracker_FastrakPDI::DisplayCont( timeval ct )
{
	BOOL bRet = FALSE;

	PBYTE pBuf;
	DWORD dwSize;

	if (!(pdiDev.LastPnoPtr(pBuf, dwSize)))
	{
	}
	else if ((pBuf == 0) || (dwSize == 0))
	{
	}
	else if (pLastBuf && (pBuf > pLastBuf))
	{
		ParseFastrakFrame( pLastBuf+dwLastSize, dwSize+(pBuf-pLastBuf-dwLastSize), ct );
		pLastBuf = pBuf;
		dwLastSize = dwSize;
		bRet = TRUE;
	}
	else if (pBuf != pLastBuf) // it wrapped in buffer
	{
		if (pLastBuf)
			cout << "wrapped\r\n";

		pLastBuf = pBuf;
		dwLastSize = dwSize;
		ParseFastrakFrame( pBuf, dwSize, ct );
		bRet = TRUE;
	}
	
	return bRet;
}

// Displays a single frame of information from the Fastrak by collecting data
//	and calling DisplayFrame
VOID vrpn_Tracker_FastrakPDI::DisplaySingle( timeval ct )
{
	BOOL bExit = FALSE;

	PBYTE pBuf;
	DWORD dwSize;

	cout << endl;

	if (!(pdiDev.ReadSinglePnoBuf(pBuf, dwSize)))
	{
		bExit = TRUE;
	}
	else if ((pBuf == 0) || (dwSize == 0))
	{
	}
	else 
	{
		ParseFastrakFrame( pBuf, dwSize, ct );
	}
}

// Parses the data collected from a DisplaySingle or DisplayContinuous command, packages it and sends it 
//  out to clients calling for it
VOID vrpn_Tracker_FastrakPDI::ParseFastrakFrame( PBYTE pBuf, DWORD dwSize, timeval current_time )
{

	DWORD index = 0;
	char msgbuf[1000];
	vrpn_int32 len;

    while (index < dwSize){		
		DWORD dw = index;
		BYTE ucSensor = pBuf[dw+1];
		BYTE ucInitCommand = pBuf[dw];
		BYTE ucErrorNum = pBuf[dw+2];
		d_sensor = atoi((char*)(&ucSensor));
		
        // skip rest of header
        dw += m_nHeaderSize;//3;

		// Catch command response frames sent when tracker is in continuous mode
		// and don't parse them but do provide output to the server screen
		if (ucInitCommand != '0'){
			printf("FastrakPDI: received record type %x while in continuous mode, record error byte was %x \r\n", ucInitCommand, ucErrorNum);
		}
		else{

			if (m_nStylusMap)
			{
				if ((m_nStylusMap & (1 << (d_sensor-1))) != 0) //FTstylusBtns[ucSensor-1])
				{
					CHAR StyFlag = pBuf[dw+1];

					FTstylusBtns[d_sensor-1]->set_button(0, StyFlag -  '0');	
					FTstylusBtns[d_sensor-1]->mainloop();
				}
				dw +=2;
			}

			PFLOAT pPno = (PFLOAT)(&pBuf[dw]); // Position and Orientation data

			if (isMetric == TRUE){
				pos[0] = float(pPno[0])*CM_TO_METERS;
				pos[1] = float(pPno[1])*CM_TO_METERS;
				pos[2] = float(pPno[2])*CM_TO_METERS;
			}
			else{
				pos[0] = float(pPno[0]);
				pos[1] = float(pPno[1]);
				pos[2] = float(pPno[2]);
			}

			// Fastrak quaternion format is WXYZ, VRPN is XYZW
			d_quat[0] = float(pPno[4]);
			d_quat[1] = float(pPno[5]);
			d_quat[2] = float(pPno[6]);
			d_quat[3] = float(pPno[3]);


			// Grab the current time and create a timestamp
			timestamp.tv_sec = current_time.tv_sec;
			timestamp.tv_usec = current_time.tv_usec;
			if (d_connection) {
				// Pack position report
				len = encode_to(msgbuf);
				d_connection->pack_message(len, timestamp,
					position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY);
			}		
		}
		index += m_nFrameSize;//28;
	}
}

// Constructor
vrpn_Tracker_LibertyPDI::vrpn_Tracker_LibertyPDI (const char * name, vrpn_Connection *cn,
	           vrpn_float64 Hz, const char * rcmd, unsigned int nStylusMap) :
  vrpn_Tracker(name, cn), update_rate(Hz)
 	, m_nStylusMap(nStylusMap)
	, m_nHeaderSize(8)
	, m_nFrameSize(nStylusMap?40:36)
 {
	    cmd = (char*)(rcmd);
		register_server_handlers();
		if(!(Initialize())){
			status = vrpn_TRACKER_FAIL;
		}
		else if (nStylusMap & !(InitStylusBtns()))
		{
			status = vrpn_TRACKER_FAIL;
		}
		else if(!(Connect())){
			status = vrpn_TRACKER_FAIL;
		}
		else if(!(SetupDevice())){
			status = vrpn_TRACKER_FAIL;
		}
		else if(!(StartCont())){
			isCont = FALSE;
			cout << "LibertyPDI: Failed to enter continuous mode\r\n";
			status = vrpn_TRACKER_FAIL;
		}
		else {
			cout << "LibertyPDI: Initialization Complete\r\n";
			status = vrpn_TRACKER_REPORT_READY;
		}
  }

// Deconstructor
vrpn_Tracker_LibertyPDI::~vrpn_Tracker_LibertyPDI(void){
	  if(isCont)
		  StopCont();
	  Disconnect();

	  for (int i=0; i<LIBERTY_MAX_SENSORS; i++)
	  {
            if (StylusBtns[i]) {
              try {
                delete StylusBtns[i];
              } catch (...) {
                fprintf(stderr, "vrpn_Tracker_LibertyPDI::~vrpn_Tracker_LibertyPDI(): delete failed\n");
                return;
              }
            }
	  }
  }

// Called by the vrpn_Generic_Sever class in order to report its status.
VOID vrpn_Tracker_LibertyPDI::mainloop()
{
	struct timeval current_time;

	// Call the generic server mainloop routine, since this is a server
	server_mainloop();

	// See if its time to generate a new report
	vrpn_gettimeofday(&current_time, NULL);
	if ( vrpn_TimevalDuration(current_time,timestamp) >= 1000000.0/update_rate) {
		DisplayCont(current_time);
	}
}

//   NOTE: you need to be sure that if you are sending vrpn_float64 then 
//   the entire array needs to remain aligned to 8 byte boundaries
//	 (malloced data and static arrays are automatically alloced in
//	 this way).  Assumes that there is enough room to store the
//	 entire message.  Returns the number of characters sent.
int	vrpn_Tracker_LibertyPDI::encode_to(char *buf)
{
   char *bufptr = buf;
   int  buflen = 1000;

   // Message includes: long sensor, long scrap, vrpn_float64 pos[3], vrpn_float64 quat[4]
   // Byte order of each needs to be reversed to match network standard

   vrpn_buffer(&bufptr, &buflen, d_sensor);
   vrpn_buffer(&bufptr, &buflen, d_sensor); // This is just to take up space to align

   vrpn_buffer(&bufptr, &buflen, pos[0]);
   vrpn_buffer(&bufptr, &buflen, pos[1]);
   vrpn_buffer(&bufptr, &buflen, pos[2]);

   vrpn_buffer(&bufptr, &buflen, d_quat[0]);
   vrpn_buffer(&bufptr, &buflen, d_quat[1]);
   vrpn_buffer(&bufptr, &buflen, d_quat[2]);
   vrpn_buffer(&bufptr, &buflen, d_quat[3]);

   return 1000 - buflen;
}


// Initialize the device and some variables
BOOL vrpn_Tracker_LibertyPDI::Initialize(VOID){
	
	pos[0]=0; pos[1]=0;	pos[2]=0;
	d_quat[0]=0; d_quat[1]=0; d_quat[2]=0; d_quat[3]=0;

	memset( StylusBtns, 0, sizeof(StylusBtns));

	hContEvent = NULL;
	hwnd = NULL;
	dwOverflowCount = 0;

	BOOL	bRet = TRUE;

	pdiDev.Trace(TRUE, 5); // Report debugging information to IDE output

	bCnxReady = FALSE;
	dwStationMap = 0;

	return bRet;
}


BOOL vrpn_Tracker_LibertyPDI::InitStylusBtns()
{
	BOOL bRet = TRUE;
	int mask = 1;
	for (int i=0; i<LIBERTY_MAX_SENSORS; i++)
	{
		if (((1<<i) & m_nStylusMap) != 0)
		{
			char btnName[512];
			sprintf( btnName, "%sStylus%d", d_servicename, i+1);
			try { StylusBtns[i] = new vrpn_Button_Server( btnName, d_connection, 1 ); }
			catch (...)
			{
				cout << "Cannot create button device " << btnName << endl;
				bRet = FALSE;
			}
			else
			{
				cout << "Button device " << btnName << " created." << endl;
			}
		}
	}
	return bRet;
}

// Connect to the Liberty using the filepath provided in the config file.
// Sets tracker to default VRPN units and frames
BOOL vrpn_Tracker_LibertyPDI::Connect( VOID )
{
    if (!(pdiDev.CnxReady()))
    {
		pdiDev.SetSerialIF( &pdiSer );

        ePiCommType eType;
		
		eType = pdiDev.DiscoverCnx();
		DWORD attempts = 0;
		while (eType != PI_CNX_USB && eType != PI_CNX_SERIAL && attempts < 10){
			switch (eType)
			{
			case PI_CNX_USB:
				cout << "LibertyPDI: USB Connection\r\n";
				status = vrpn_TRACKER_SYNCING;
				break;
			case PI_CNX_SERIAL:
				cout << "LibertyPDI: Serial Connection\r\n";
				status = vrpn_TRACKER_SYNCING;
				break;
			default:
				printf("LibertyPDI: %s\r\n", pdiDev.GetLastResultStr() );
				eType = pdiDev.DiscoverCnx();
				attempts++;
				break;
			}
			Sleep(3000);
		}

		UpdateStationMap();
		num_sensors = pdiDev.StationCount();

		CPDIbiterr cBE;
		pdiDev.GetBITErrs( cBE );

		CHAR sz[100];
		cBE.Parse( sz, 100 );

		if(!(cBE.IsClear()))
			pdiDev.ClearBITErrs();
	
		// Set VRPN defaults for position, orientation and frame structure
		pdiMDat.Empty();
		if (m_nStylusMap)
		{
			pdiMDat.Append(PDI_MODATA_STYLUS);
		}
		pdiMDat.Append( PDI_MODATA_POS );
		pdiMDat.Append( PDI_MODATA_QTRN );
		pdiDev.SetSDataList( -1, pdiMDat );
	
		pdiDev.SetMetric(TRUE);
		isMetric = TRUE;
		isBinary = TRUE;

		// The CPDIdev class will use this space, even if we don't access it directly,
		// which allows us to specify the size of the buffer
		pdiDev.SetPnoBuffer( pMotionBuf, VRPN_PDI_BUFFER_SIZE );
		pLastBuf = 0;
		dwLastSize = 0;

		bCnxReady = pdiDev.CnxReady();
	}
	else{
		cout << "LibertyPDI: Already connected\r\n";
		bCnxReady = TRUE;
	}

	return bCnxReady;
}

// End the connection to the Liberty
VOID vrpn_Tracker_LibertyPDI::Disconnect(VOID)
{
    string msg;
    if (!(pdiDev.CnxReady()))
    {
        cout << "LibertyPDI: Already disconnected\r\n";
    }
    else
    {
        pdiDev.Disconnect();
        cout << "LibertyPDI: Disconnected\r\n";
    }
    
}

// Send reset commands to tracker
BOOL vrpn_Tracker_LibertyPDI::SetupDevice( VOID )
{
	char * pcmd = cmd;
	if(strlen(pcmd) > 0){
		char * pch = strchr(pcmd,'\\');
		while (pch != NULL){
			pch[0] = '\0';
			SendCommand(pcmd);
			pcmd = pch + sizeof(char);
			pch = strchr(pcmd,'\\');
		}
		SendCommand(pcmd);
	}

	// warn the user if tracker is in ASCII mode
	if (isBinary == FALSE)
		cout << "LibertyPDI: Warning! Tracker is still in ASCII mode!\r\n";

	return TRUE;
}

// Updates the map of hubs and sensors
VOID vrpn_Tracker_LibertyPDI::UpdateStationMap( VOID )
{
	pdiDev.GetStationMap( dwStationMap );
}

// Sends commands to the tracker, the syntax is explained in vrpn_LibertyPDI.cfg
VOID vrpn_Tracker_LibertyPDI::SendCommand(char *scmd)
{	
	char szCmd[PI_MAX_CMD_BUF_LEN] = "\0";
	DWORD dwRspSize = 5000;
	char szRsp[5000] = "\0";
	bool parseErr = false;

	// print scmd to server screen
	cout << "LibertyPDI: reset command ";
	unsigned int i = 0;
	while (scmd[i] != '\0'){
		// ignore carriage returns and new lines (cleans up output)
		if (scmd[i] != '\n' && scmd[i] != '\r')
			cout << scmd[i];
		i++;
	}

	// parse scmd and create a liberty friendly command string szCmd
	for (i = 0; i < strlen(scmd); i++){
		switch (scmd[i]){
			case '^': // convert ^ to control+char ascii equivalent
				i++;
				switch (scmd[i]){
					case 'A':
						strncat(szCmd, "\x01", 1);
						break;
					case 'B':
						strncat(szCmd, "\x02", 1);
						break;
					case 'D':
						strncat(szCmd, "\x04", 1);
						break;
					case 'E':
						strncat(szCmd, "\x05", 1);
						break;
					case 'F':
						strncat(szCmd, "\x06", 1);
						break;
					case 'G':
						strncat(szCmd, "\x07", 1);
						break;
					case 'K':
						strncat(szCmd, "\x0b", 1);
						break;
					case 'L':
						strncat(szCmd, "\x0c", 1);
						break;
					case 'N':
						strncat(szCmd, "\x0e", 1);
						break;
					case 'O':
						strncat(szCmd, "\x0f", 1);
						break;
					case 'P':
						strncat(szCmd, "\x10", 1);
						break;
					case 'R':
						strncat(szCmd, "\x12", 1);
						break;
					case 'S':
						strncat(szCmd, "\x13", 1);
						break;
					case 'T':
						strncat(szCmd, "\x14", 1);
						break;
					case 'U':
						strncat(szCmd, "\x15", 1);
						break;
					case 'V':
						strncat(szCmd, "\x16", 1);
						break;
					case 'W':
						strncat(szCmd, "\x17", 1);
						break;
					case 'X':
						strncat(szCmd, "\x18", 1);
						break;
					case 'Y':
						strncat(szCmd, "\x19", 1);
						break;
					case 'Z':
						strncat(szCmd, "\x1a", 1);
						break;
					default: // no match, flip parseErr
						i = strlen(scmd);
						parseErr = true;
				}
				break;
			case '<': // check for <>, which represents a carriage return
				i++;
				if (scmd[i] != '>'){
					i = strlen(scmd);
					parseErr = true;
				}
				else
					strncat(szCmd, "\r\n", 1);
				break;
			case '\r': // ignore carriage returns.
				break;
			case '\n': // ignore new lines
				break;
			case '\xff': // ignore eof
				break;
			default:
				strncat(szCmd, &scmd[i], 1);
		}
	}

	if (parseErr == true)
		cout << "\r\n\t<unrecognized command>\r\n";
	else{
		strncat(szCmd, "\0", 1);
		switch (szCmd[0]){
			case 'C': // Continuous pno command
			case 'c': // Ignored since this would conflict with VRPN directly
				cout << "\r\n\t<command ignored, use P instead>\r\n";
				break;
			case 0x19: // reset command
				pdiDev.TxtCmd(szCmd,dwRspSize,szRsp);
				cout << "\r\n\t<tracker will be set to VRPN defaults on reconnect>\r\n";
				Disconnect();
				Sleep(2500);
				Connect();
				break;
			case 'U': // units command
				pdiDev.TxtCmd(szCmd,dwRspSize,szRsp);
				if (isBinary == TRUE)
					pdiDev.GetMetric(isMetric);
				else{
					pdiDev.SetBinary(TRUE);
					pdiDev.GetMetric(isMetric);
					pdiDev.SetBinary(FALSE);
				}
				if (isMetric == FALSE)
					cout << "\r\n\t<position units set to inches>\r\n";
				else
					cout << "\r\n\t<position units set to meters>\r\n";
				break;
			case 'F': // format (binary or ascii) command
				pdiDev.TxtCmd(szCmd,dwRspSize,szRsp);
				pdiDev.GetBinary(isBinary);
				if (isBinary == FALSE)
					cout << "\r\n\t<response frames set to ascii>\r\n";
				else
					cout << "\r\n\t<response frames set to binary>\r\n";
				break;
			case 'O': // data list format command
				pdiDev.TxtCmd(szCmd,dwRspSize,szRsp);
				cout << "\r\n\t<pno frame format changed (use extreme caution)>\r\n";
				break;
			default:
				pdiDev.TxtCmd(szCmd,dwRspSize,szRsp);
				if (isBinary == TRUE && dwRspSize != 0 && dwRspSize != 5000){
					char * pRsp = szRsp;
					pRsp += 4;
					if (*pRsp == 0x00 || *pRsp == 0x20)
						cout << "\r\n\t<binary response contained no error>\r\n";
					else{
						pRsp += 2;
						cout << "\r\n\t<binary error response bgn>\r\n\t";
						for (int i = 2; i < 2 + short(*pRsp); i++){
							if (szRsp[i] != '\r')
								printf("%c",pRsp[i]);
							if (szRsp[i] == '\n')
								printf("\t");
						}
						cout << "\r\n\t<binary error response end>\r\n";
					}
				}
				else if (dwRspSize != 0 && dwRspSize != 5000){
					cout << "\r\n\t<ASCII response bgn>\r\n\t";
					for (unsigned int i = 0; i < dwRspSize; i++){
						if (szRsp[i] != '\r')
							printf("%c",szRsp[i]);
						if (szRsp[i] == '\n')
							printf("\t");
					}
					cout << "\r\n\t<ASCII response end>\r\n";
				}
				else
					cout << "\r\n\t<command sent>\r\n";
		}
	}
}

// Start Continuous Mode for the Liberty
BOOL vrpn_Tracker_LibertyPDI::StartCont( VOID )
{
	cout << "LibertyPDI: Start Continuous Mode\r\n";
	BOOL bRet = FALSE;


	if (!(pdiDev.StartContPno(hwnd)))
	{
	}
	else
	{
		isCont = TRUE;
		dwOverflowCount = 0;
		bRet = TRUE;
	}

	return bRet;
}

// Stops Continuous Mode for the Liberty
BOOL vrpn_Tracker_LibertyPDI::StopCont( VOID )
{
	cout << "LibertyPDI: Stop Continuous Mode\r\n";
	BOOL bRet = FALSE;

	if (!(pdiDev.StopContPno()))
	{
	}
	else
	{
		isCont = FALSE;
		bRet = TRUE;
		Sleep(1000);
	}

	::ResetEvent(hContEvent);
	return bRet;
}

// Displays a frame of information taken from the continuous stream of 
// Continuous Mode by calling displayFrame
BOOL vrpn_Tracker_LibertyPDI::DisplayCont( timeval ct )
{
	BOOL bRet = FALSE;

	PBYTE pBuf;
	DWORD dwSize;

	if (!(pdiDev.LastPnoPtr(pBuf, dwSize)))
	{
	}
	else if ((pBuf == 0) || (dwSize == 0))
	{
	}
	else if (pLastBuf && (pBuf > pLastBuf))
	{
		ParseLibertyFrame( pLastBuf+dwLastSize, dwSize+(pBuf-pLastBuf-dwLastSize), ct );
		pLastBuf = pBuf;
		dwLastSize = dwSize;
		bRet = TRUE;
	}
	else if (pBuf != pLastBuf) // it wrapped in buffer
	{
		if (pLastBuf)
			cout << "wrapped\n";

		pLastBuf = pBuf;
		dwLastSize = dwSize;
		ParseLibertyFrame( pBuf, dwSize, ct );
		bRet = TRUE;
	}
	
	return bRet;
}

// Displays a single frame of information from the Liberty by collecting data
//	and calling DisplayFrame
VOID vrpn_Tracker_LibertyPDI::DisplaySingle( timeval ct )
{
	BOOL bExit = FALSE;

	PBYTE pBuf;
	DWORD dwSize;

	cout << endl;

	if (!(pdiDev.ReadSinglePnoBuf(pBuf, dwSize)))
	{
		bExit = TRUE;
	}
	else if ((pBuf == 0) || (dwSize == 0))
	{
	}
	else 
	{
		ParseLibertyFrame( pBuf, dwSize, ct );
	}
}

// Parses the data collected from a DisplaySingle or DisplayContinuous command, packages it and sends it 
// out to clients calling for it
VOID vrpn_Tracker_LibertyPDI::ParseLibertyFrame( PBYTE pBuf, DWORD dwSize, timeval current_time )
{

	DWORD index = 0;
	char msgbuf[1000];
	vrpn_int32 len;

    while (index < dwSize){		
		DWORD dw = index;
		BYTE ucSensor = pBuf[dw+2];
		BYTE ucInitCommand = pBuf[dw+3];
		BYTE ucErrorNum = pBuf[dw+4];
		SHORT shSize = pBuf[dw+6];
		d_sensor = unsigned short(ucSensor);
		
        // skip rest of header
        dw += m_nHeaderSize;//8;

		// Catch command response frames sent when tracker is in continuous mode
		// and don't parse them but do provide output to the server screen
		if (ucInitCommand != 'C' && ucInitCommand != 'P'){
			printf("LibertyPDI: received command %x while in continuous mode, tracker response was %x \r\n", ucInitCommand, ucErrorNum);
		}
		else{
			if (m_nStylusMap)
			{
				if ((m_nStylusMap & (1 << (d_sensor-1))) != 0) 
				{
					DWORD m_dwStylus = *((DWORD*)&pBuf[dw]);
 					
					StylusBtns[d_sensor-1]->set_button(0, m_dwStylus);	
					StylusBtns[d_sensor-1]->mainloop();
				}
				dw += sizeof(DWORD);;
			}


			PFLOAT pPno = (PFLOAT)(&pBuf[dw]); // Position and Orientation data

			if (isMetric == TRUE){
				pos[0] = float(pPno[0])*CM_TO_METERS;
				pos[1] = float(pPno[1])*CM_TO_METERS;
				pos[2] = float(pPno[2])*CM_TO_METERS;
			}
			else{
				pos[0] = float(pPno[0]);
				pos[1] = float(pPno[1]);
				pos[2] = float(pPno[2]);
			}

			// Liberty quaternion format is WXYZ, VRPN is XYZW
			d_quat[0] = float(pPno[4]);
			d_quat[1] = float(pPno[5]);
			d_quat[2] = float(pPno[6]);
			d_quat[3] = float(pPno[3]);

			// Grab the current time and create a timestamp
			timestamp.tv_sec = current_time.tv_sec;
			timestamp.tv_usec = current_time.tv_usec;
			if (d_connection) {
				// Pack position report
				len = encode_to(msgbuf);
				d_connection->pack_message(len, timestamp,
					position_m_id, d_sender_id, msgbuf, vrpn_CONNECTION_LOW_LATENCY);
			}		
		}
		index += m_nFrameSize;
	}
}

#endif
