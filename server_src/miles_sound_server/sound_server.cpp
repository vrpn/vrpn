#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#include <conio.h>
#include "resource.h"
#include <commctrl.h>
#endif

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <iostream>

#ifdef _WIN32
#include "Console.hpp"
#endif

#ifndef _USE_OLD_OSTREAMS
using namespace std;
#endif

#include "vrpn_Connection.h"
#include "vrpn_Tracker.h"
#include "vrpn_Sound.h"

#ifdef _WIN32
#include "vrpn_Sound_Miles.h"
#endif
#include "vrpn_ForwarderController.h"

#define SOUND_DEBUG 1

//Sound Server Specifics
#ifdef _WIN32
vrpn_Sound_Server_Miles *soundServer = NULL;

#define cboxTech     100
#define btnPlay      101
#define btnStop      110
#define btnClose     112
#define ErrorBox	 114
#define VolumeBox    115
#define ReplayBox    116
#define SoundIdBox   131

#define PosX		 120
#define PosY		 121
#define PosZ		 122

#define LPosX		 141
#define LPosY		 142
#define LPosZ		 143

#define FMin		 151
#define FMax		 152
#define BMin		 153
#define BMax		 154

#define SoundIdTBox	 161
#define MoreInfoBtn  162

#define SorX		 300
#define SorY		 301
#define SorZ		 302
#define LorX		 303
#define LorY		 304
#define LorZ		 305

#define UpdateTimerId 1001
#define DialogTimerId 1002
int         blanked     =  0;		
int         ProviderSet = 0;		

HWND        SoundProvideCombo;
HWND        SoundIdCombo;
HWND		SoundWnd;
HWND		InfoWnd;

vrpn_Tracker_Remote * tracker_connection;
char tracker_device[512];
char tracker_name[512];

int USE_TRACKER;

enum BoxAction{add, del};

vrpn_int32 CurrentSoundId = -1;
#endif

vrpn_Connection * connection;

// TCH October 1998
// Use Forwarder as remote-controlled multiple connections.
vrpn_Forwarder_Server * forwarderServer;

//Console for Win32 version of server
#ifdef _WIN32
CConsole *con;
#endif

// install a signal handler to shut down the trackers and buttons
#ifndef WIN32
#include <signal.h>
void closeDevices();

void sighandler (int)
{
    closeDevices();
    delete connection;
    exit(0);
}
#endif

void closeDevices (void) {
	cerr << endl << "All devices closed. Exiting ..." << endl;
}

void handle_tracker_report(void *, const vrpn_TRACKERCB track_rep) {
  static vrpn_TRACKERCB last_rep; // last report
  vrpn_ListenerDef listener;

  listener.pose.position[0]    = last_rep.pos[0];
  listener.pose.position[1]    = last_rep.pos[1];
  listener.pose.position[2]    = last_rep.pos[2];
  listener.pose.orientation[0] = last_rep.quat[0];
  listener.pose.orientation[1] = last_rep.quat[1];
  listener.pose.orientation[2] = last_rep.quat[2];
  listener.pose.orientation[3] = last_rep.quat[3];
  
  // velocity _should_ be automatically set
  
  soundServer->changeListenerStatus(listener);
  last_rep = track_rep;
  return;
}

int handle_dlc (void *, vrpn_HANDLERPARAM p)
{
    closeDevices();
    delete connection;
    exit(0);
    return 0;
}

void shutDown (void)
{
    closeDevices();
#ifdef _WIN32
	if (soundServer)
	   soundServer->shutDown();
	con->DestroyConsole();
#endif
    delete connection;
    exit(0);
    return;
}

#ifdef _WIN32

void ChangeSoundIdBox(vrpn_int32 action, vrpn_int32 newId) {
	char numbuf[3];

	sprintf(numbuf,"%d",newId);
	ComboBox_AddString(SoundIdCombo,numbuf);
}


void UpdateDialog(HWND SoundWnd) {
	
	float posx, posy, posz, orx, ory, orz;
	char buf[15];
	
	SetDlgItemText(SoundWnd,ErrorBox,soundServer->GetLastError());

    soundServer->GetCurrentPosition(CurrentSoundId, &posx, &posy, &posz);
	sprintf(buf,"%4.3f",posx);
	SetDlgItemText(SoundWnd,PosX,buf);
	sprintf(buf,"%4.3f",posy);
	SetDlgItemText(SoundWnd,PosY,buf);
	sprintf(buf,"%4.3f",posz);
	SetDlgItemText(SoundWnd,PosZ,buf);	
	
	soundServer->GetListenerPosition(&posx, &posy, &posz);
	sprintf(buf,"%4.3f",posx);
	SetDlgItemText(SoundWnd,LPosX,buf);
	sprintf(buf,"%4.3f",posy);
	SetDlgItemText(SoundWnd,LPosY,buf);
	sprintf(buf,"%4.3f",posz);
	SetDlgItemText(SoundWnd,LPosZ,buf);	

	soundServer->GetCurrentOrientation(CurrentSoundId, &orx, &ory, &orz);
	sprintf(buf,"%4.3f",orx);
	SetDlgItemText(SoundWnd,SorX,buf);
	sprintf(buf,"%4.3f",ory);
	SetDlgItemText(SoundWnd,SorY,buf);
	sprintf(buf,"%4.3f",orz);
	SetDlgItemText(SoundWnd,SorZ,buf);	
	
	soundServer->GetListenerOrientation(&orx, &ory, &orz);
	sprintf(buf,"%4.3f",orx);
	SetDlgItemText(SoundWnd,LorX,buf);
	sprintf(buf,"%4.3f",ory);
	SetDlgItemText(SoundWnd,LorY,buf);
	sprintf(buf,"%4.3f",orz);
	SetDlgItemText(SoundWnd,LorZ,buf);	


}

/******************************************************************************
 Sound Server Window Section
 *****************************************************************************/
LRESULT AILEXPORT SoundServerProc(HWND SoundWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  HWND h;
  float fmin, fmax, bmin, bmax;
  char buf[15];

  switch (message)
      {

      case WM_SETFOCUS:    // deal with the focus in this weird dialog-window
          h=GetWindow(SoundWnd,GW_CHILD);
          while (h) {
            if ((GetWindowLong(h,GWL_STYLE)&0x2f)==BS_DEFPUSHBUTTON) {
              SetFocus(h);
              goto found;
            }
            h=GetNextWindow(h,GW_HWNDNEXT);
          }
          SetFocus(GetWindow(SoundWnd,GW_CHILD));
       found:
          break;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
          SetBkColor((HDC)wParam,RGB(192,192,192));
          return((LRESULT)GetStockObject(LTGRAY_BRUSH));

      case WM_HSCROLL:
         return 0;

      case WM_COMMAND:

         switch (LOWORD(wParam))
         {

          case cboxTech:
             if (HIWORD(wParam) == CBN_SELENDOK)
				 if (ComboBox_GetCurSel(SoundProvideCombo)-1 > 0) {
					 ProviderSet = 1;
                   soundServer->setProvider(ComboBox_GetCurSel(SoundProvideCombo)-1);
				 } else ProviderSet = 0;
	
			 break;
		  case MoreInfoBtn:
		  	SetDlgItemInt(InfoWnd,VolumeBox,soundServer->GetCurrentVolume(CurrentSoundId),1);
			SetDlgItemInt(InfoWnd,ReplayBox,soundServer->GetCurrentPlaybackRate(CurrentSoundId),1);
			soundServer->GetCurrentDistances(CurrentSoundId, &fmin, &fmax, &bmin, &bmax);
			sprintf(buf,"%4.3f",fmin);
			SetDlgItemText(InfoWnd,FMin,buf);
			sprintf(buf,"%4.3f",fmax);
			SetDlgItemText(InfoWnd,FMax,buf);
			sprintf(buf,"%4.3f",bmin);
			SetDlgItemText(InfoWnd,BMin,buf);	
			sprintf(buf,"%4.3f",bmax);
			SetDlgItemText(InfoWnd,BMax,buf);	
			if (CurrentSoundId >= 0)
			  sprintf(buf,"Sound: %d",CurrentSoundId);
			else 
			  sprintf(buf,"Sound: none selected");
			SetWindowText(InfoWnd,buf);
			ShowWindow(InfoWnd,SW_SHOW);
			SetTimer(SoundWnd,DialogTimerId,3000,NULL);
		  break;

          case SoundIdBox:
			  if (HIWORD(wParam) == CBN_SELENDOK) {
				  if (ComboBox_GetCurSel(SoundProvideCombo)-1 > 0) {
				  CurrentSoundId = ComboBox_GetCurSel(SoundIdCombo);
				  UpdateDialog(SoundWnd);
				  }
			  }
			 break;


          case btnStop:
			 soundServer->stopAllSounds();
             break;
		 
          case IDCLOSE:
			ShowWindow(InfoWnd,SW_HIDE);
			break;
          case btnClose:
			 
             DestroyWindow(SoundWnd);
			 soundServer->shutDown();
			 break;
         }
         return 0;

      case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
      case WM_TIMER:
		  // if there is no provider set then updating will cause an error!
		  if (wParam == UpdateTimerId) {
		    if (ProviderSet)
		      UpdateDialog(SoundWnd);
		  }
		  else {
			  ShowWindow(InfoWnd, SW_HIDE);
			  KillTimer(SoundWnd, DialogTimerId);
		  }
		    break;
      }
  
   return DefWindowProc(SoundWnd,message,wParam,lParam);
}

static void add_providers()
{
   char* name;
   HPROVIDER provider;

   HPROENUM next = HPROENUM_FIRST;

   SoundProvideCombo=GetDlgItem(SoundWnd,cboxTech);
   
   ComboBox_AddString(SoundProvideCombo,"Choose a provider...");

   while (AIL_enumerate_3D_providers(&next, &provider, &name))
   {
      ComboBox_AddString(SoundProvideCombo,name);
	  soundServer->addProvider(provider);
   }

   ComboBox_SetCurSel(SoundProvideCombo,0);
}

bool InitSoundServerWindow(HINSTANCE  hInstance)
{
	WNDCLASS    wc;
	BOOL        rc;
	static char szAppName[] = "SoundServerWin32";
	
	wc.lpszClassName = szAppName;
	wc.lpfnWndProc = (WNDPROC) SoundServerProc;
	wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance,"VRPN");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
	wc.cbClsExtra = 0;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.lpszMenuName = NULL;
	
	rc = RegisterClass(&wc);
	if (!rc)
		return false;

	InitCommonControls();
	
	SoundWnd = CreateDialog(hInstance,(LPCSTR)"SOUNDSERVERWIN32",0,NULL);
	DWORD error = GetLastError();
	if (!SoundWnd) {
		fprintf(stderr,"%ld\n",error);
		return false;
	}
	// set a timer to update server stats
	SetTimer(SoundWnd,UpdateTimerId,1000,NULL);
	SoundIdCombo=GetDlgItem(SoundWnd, SoundIdBox);	
	
	ShowWindow(SoundWnd,SW_SHOW);
	InfoWnd = CreateDialog(hInstance,(LPCSTR)"SND_INFO",0,NULL);
	error = GetLastError();
	if (!InfoWnd) {
		fprintf(stderr,"%ld\n",error);
		return false;
	}

	ShowWindow(InfoWnd,SW_HIDE);

	return true;
}

#endif //Win32
/******************************************************************************
 End Sound Server Window Section
 *****************************************************************************/

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
#ifdef _WIN32
	MSG      msg;
#endif
	char	* config_file_name = "vrpn.cfg";
	FILE	* config_file;
	char 	* client_name = NULL;
	int	client_port;
	int	bail_on_error = 1;
	int	verbose = 1;
	int	auto_quit = 0;
	int	realparams = 0;
	int 	loop = 0;
	int	port = vrpn_DEFAULT_LISTEN_PORT_NO;
#ifdef WIN32
	WSADATA wsaData; 
	int status;
	if ((status = WSAStartup(MAKEWORD(1,1), &wsaData)) != 0) 
	{
		fprintf(stderr, "WSAStartup failed with %d\n", status);
		return(1);
	}
#else
#ifdef sgi
	sigset( SIGINT, sighandler );
	sigset( SIGKILL, sighandler );
	sigset( SIGTERM, sighandler );
	sigset( SIGPIPE, sighandler );
#else
	signal( SIGINT, sighandler );
	signal( SIGKILL, sighandler );
	signal( SIGTERM, sighandler );
	signal( SIGPIPE, sighandler );
#endif // sgi
#endif
	
	con = new CConsole(TRUE);
	con->RedirectToConsole(0);

	USE_TRACKER = 0;

	
	// Need to have a global pointer to it so we can shut it down
	// in the signal handler (so we can close any open logfiles.)

	connection = new vrpn_Synchronized_Connection (port);
	
	// Open the configuration file
	if (verbose) printf("Reading from config file %s\n", config_file_name);
	if ( (config_file = fopen(config_file_name, "r")) == NULL) 
	{
		perror("Cannot open config file");
		fprintf(stderr,"  (filename %s)\n", config_file_name);
		return -1;
	}
	
	// Read the configuration file, creating a device for each entry.
	// Each entry is on one line, which starts with the name of the
	//   class of the object that is to be created.
	// If we fail to open a certain device, print a message and decide
	//  whether we should bail.
	{	
		char	line[512];	// Line read from the input file
		char *pch;
		char    scrap[512], s2[512];
		
		// Read lines from the file until we run out
		while ( fgets(line, sizeof(line), config_file) != NULL ) 
		{
			
			// Make sure the line wasn't too long
			if (strlen(line) >= sizeof(line)-1) 
			{
				fprintf(stderr,"Line too long in config file: %s\n",line);
				if (bail_on_error) { return -1; }
				else { continue; }	// Skip this line
			}
			
			if ((strlen(line)<3)||(line[0]=='#')) 
			{
				// comment or empty line -- ignore
				continue;
			}
			
			// copy for strtok work
			strncpy(scrap, line, 512);
			// Figure out the device from the name and handle appropriately
			
			// WARNING: SUBSTRINGS WILL MATCH THE EARLIER STRING, SO 
			// ADD AN EMPTY SPACE TO THE END OF STATIC STRINGS!!!!
			
			//	  #define isit(s) !strncmp(line,s,strlen(s))
#define isit(s) !strcmp(pch=strtok(scrap," \t"),s)
#define next() pch += strlen(pch) + 1
	
#ifdef _WIN32
			if(isit("vrpn_Sound_Server"))
			{
				fprintf(stderr,"ss%s\n",pch); 
				next();
				fprintf(stderr,"%s\n",pch);
				if (sscanf(pch,"%511s\t%d\t%511s\t%511s",s2,&USE_TRACKER,tracker_name, tracker_device) != 4) 
				{
					fprintf(stderr,"Bad vrpn_Server_Sound line: %s\n",line);
					if (bail_on_error) 
					{ 
						return -1; 
					}
					else 
					{ 
						continue; 
					}	// Skip this line
				}
				fprintf(stderr,"ok\n");

				
				if (InitSoundServerWindow(hInstance) != true) 
					fprintf(stderr, "Didnt open window!\n"); 
				
				soundServer = NULL;
				soundServer = new vrpn_Sound_Server_Miles(s2, connection);
				if (soundServer == NULL) 
					fprintf(stderr,"Can't create sound server\n");
				add_providers();
			}
#endif			
			
		}
	}
	
	// Close the configuration file
	fclose(config_file);

	// Open remote tracker if we are to use one

	if (USE_TRACKER) {
		
		char newname[1024];
		sprintf(newname,"%s@%s",(const char*)tracker_device, (const char*)tracker_name);
		fprintf(stderr,"Using tracker: %s\n",newname);
		tracker_connection = new vrpn_Tracker_Remote((const char *) newname);
		tracker_connection->register_change_handler(NULL,handle_tracker_report);
	}
	else fprintf(stderr,"Not using tracker\n");

	// Open the Forwarder Server
    forwarderServer = new vrpn_Forwarder_Server (connection);

	loop = 0;
	if (auto_quit) 
	{
		int dlc_m_id = connection->register_message_type(
			vrpn_dropped_last_connection);
		connection->register_handler(dlc_m_id, handle_dlc, NULL);
	}
	
	if (client_name) 
	{
		fprintf(stderr, "vrpn_serv: connecting to client: %s:%d\n",
			client_name, client_port);
		if (connection->connect_to_client(client_name, client_port))
		{
			fprintf(stderr, "server: could not connect to client %s:%d\n", client_name, client_port);
			shutDown();
		}
	}
	
	// ********************************************************************
	// **                                                                **
	// **                MAIN LOOP                                       **
	// **                                                                **
	// ********************************************************************
	while (1) 
	{
		GetMessage(&msg, 0, 0, 0);
	    if (!IsDialogMessage(SoundWnd,&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
   
		// Let the sound server do it's thing 
		soundServer->mainloop();
		if (soundServer->noSounds()) {
			EnableWindow(SoundProvideCombo, true);
			if (!blanked) {
			  ComboBox_ResetContent(SoundIdCombo); 
			  blanked = 1;
			}
		}
		else {
			EnableWindow(SoundProvideCombo, false);
			blanked = 0;
		}
		
		
		// Send and receive all messages
		connection->mainloop();
		if (!connection->doing_okay()) shutDown();
		
		// Handle forwarding requests;  send messages
		// on auxiliary connections
		forwarderServer->mainloop();
	}
	
	return 0;
}