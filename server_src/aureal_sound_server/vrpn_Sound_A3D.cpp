#include "vrpn_Sound_A3D.h"

vrpn_Sound_Server_A3D::vrpn_Sound_Server_A3D(const char * name, vrpn_Connection * c, HWND hWin) 
						   : vrpn_Sound_Server(name, c){

	// initialize the goodies
	a3droot     = NULL;
    a3dgeom     = NULL;
    a3dlis      = NULL;
    
	A3dRegister();

	// detect the hardware
	/*
	int hw = A3dDetectHardware ();
	switch (hw) {
	case 0: printf("No audio hardware detected\n"); return;
	case 1: printf("Found A3D hardeare!\n"); break;
	case 2: printf("Found generic hardware\n"); return;
	};
	*/
	// initialize COM 
	CoInitialize(NULL);

	// init the a3d root
	lasterror = A3dCreate(NULL, (void **)&a3droot, NULL, A3D_1ST_REFLECTIONS | A3D_OCCLUSIONS);


//	lasterror = CoCreateInstance(CLSID_A3dApi, NULL, CLSCTX_INPROC_SERVER,
//	                      IID_IA3d4, (void **)&a3droot);
    if (FAILED(lasterror)) {
		printf("Error in CoCreateInstance in constructor\n");
		return;
	}

	// check that features are available
/*
	if (a3droot->IsFeatureAvailable(A3D_1ST_REFLECTIONS)==FALSE)
		printf("Reflections not available with current default audio device\n");

	if (a3droot->IsFeatureAvailable(A3D_OCCLUSIONS)==FALSE)
		printf("Occlusions not available with current default audio device\n");

	// initialize a3d 

	a3droot->Init(NULL, NULL/*A3D_1ST_REFLECTIONS |  A3D_OCCLUSIONS, A3DRENDERPREFS_DEFAULT);
*/

	a3droot->SetCooperativeLevel(hWin, A3D_CL_NORMAL);

	lasterror = a3droot->QueryInterface(IID_IA3dListener, (void **)&a3dlis);

    if (FAILED(lasterror)) {
		printf("Couldnt get Listener in constructor\n");
		return;
	}

	lasterror = a3droot->QueryInterface(IID_IA3dGeom, (void **)&a3dgeom);
	if (FAILED(lasterror)) {
		printf("Couldnt get Geometry in constructor\n");
		return;
	}

	a3dgeom->Enable(A3D_OCCLUSIONS);
	a3dgeom->Enable(A3D_1ST_REFLECTIONS);


	// set some global default values
	// default DistanceModelScale is 1.0 which means gain is reducded 6dB for doubling in distance from listener
	// default DopplerScale is 1.0 (340m/s)
	// default Equalization is 1.0
	// default MaxReflectionDelayTime is .3 seconds which according to
	//   the A3D document is adequate for models smaller than a football stadium
	// default NumFallBackSources is 12 (more than 12 sources and we go to software)
	// default RMPriorityBias is .5 means we equally weigh audibility and priority when deciding whether to play source
	// default UnitsPerMeter is 1.0 (we use meters)

	// default coordinate system is RIGHT HANDED
	lasterror = a3droot->SetCoordinateSystem(A3D_RIGHT_HANDED_CS);

	lasterror = a3droot->SetOutputGain(1.0);  // max gain
	
	// default to using headphones
	lasterror = a3droot->SetOutputMode(OUTPUT_HEADPHONES,OUTPUT_HEADPHONES,OUTPUT_MODE_STEREO);

	// set up our playlist
	maxSounds = 15;
	numSounds = -1;
	
	a3dsamples = new (IA3dSource*);

	// initialize playlist
	for (int i(0); i < maxSounds; i++) {
		a3dsamples[i] = NULL;
	}

}
					
vrpn_Sound_Server_A3D::~vrpn_Sound_Server_A3D() {

	// free up COM
	CoUninitialize();

	a3droot->Shutdown();
}

	
void vrpn_Sound_Server_A3D::playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef) {
    printf("Playing sound %d\n", id);
	if (id < numSounds) {
	  if (repeat > 1)
	    lasterror = a3dsamples[id]->Play(A3D_LOOPED);
	  else 
		lasterror = a3dsamples[id]->Play(A3D_SINGLE);
	}
}

void vrpn_Sound_Server_A3D::stopSound(vrpn_SoundID id) {
	printf("Stopping sound %d\n",id);
	lasterror = a3dsamples[id]->Stop();

}

void vrpn_Sound_Server_A3D::loadSound(char* filename, vrpn_SoundID id) {

	printf("Loading sound #%d: %s\n",numSounds+1,filename);
   numSounds++;
   if (numSounds == maxSounds) {
		maxSounds = 2 * maxSounds;
		IA3dSource ** temp = new (IA3dSource*);

		for (int i(0); i < maxSounds; i++) {
			if (i <= numSounds)
			  temp[i] = a3dsamples[i];
			else temp[i] = NULL;
		}
		delete [] a3dsamples;
		a3dsamples = temp;
	}

   lasterror = a3droot->NewSource( A3DSOURCE_INITIAL_RENDERMODE_A3D , &a3dsamples[numSounds]);

  if (FAILED(lasterror)) {
    printf("Error making new source\n");
	return;
  }

  lasterror = a3dsamples[numSounds]->LoadWaveFile(filename);
  
  if (FAILED(lasterror)) {
    printf("Error making new source\n");
	return;
  }

  // initialize sound params

  a3dsamples[numSounds]->SetCone(0,0,1);
  // these values are defaults:
  // default ModelScale of 6dB for each doubling of distance
  // default DopplerScale of 1.0
  // default Equalization of 1.0
  // default Gain is 1.0 
  // default orientation doesnt matter since we have no directional (cone) source
  // default pitch is 1.0 (unaltered pitch)
  // default priority of .5 on a scale from 0 to 1
  // default ReflectionDelayScale is 1.0 (normal delay in reflections [.3 seconds])
  // default RefelectionGainScale is 1.0 
  // default RenderMode uses occlusion, reflection, a3d (??)
  // default TransformMode is relative to origin
    
  
  a3dsamples[numSounds]->SetMinMaxDistance(1.0, 6.0, A3D_MUTE);

  // default position is at the origin
  a3dsamples[numSounds]->SetPosition3f(0.0,0.0,0.0);
}

void vrpn_Sound_Server_A3D::unloadSound(vrpn_SoundID id) {
  printf("Unloading sound %d\n", id);
  a3dsamples[id]->Release();

  for (int i(id); i < numSounds; i++) {
	a3dsamples[i] = a3dsamples[i+1];
  }
}

void vrpn_Sound_Server_A3D::changeListenerStatus(vrpn_ListenerDef listenerDef) {
	q_vec_type 	   angles;
	q_matrix_type  colMatrix;
	A3DVAL         X_val, Y_val, Z_val;

	printf("Setting listener coords\n");
	// go from the listeners orientation quaternion to euler angles
	// A3D wants angles in DEGREES!!
	q_to_col_matrix (colMatrix, listenerDef.pose.orientation);
	q_col_matrix_to_euler(angles,colMatrix);

	X_val = angles[0]*180.0/Q_PI;
	Y_val = angles[1]*180.0/Q_PI;
	Z_val = angles[2]*180.0/Q_PI;
	// uses Y(up),X(right),Z(out)
	a3dlis->SetOrientationAngles3f(Y_val,X_val,Z_val);

	a3dlis->SetPosition3f(listenerDef.pose.position[0],listenerDef.pose.position[1],listenerDef.pose.position[2]);
	// right now we ignore velocity
}
						
void vrpn_Sound_Server_A3D::changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef) {
	q_vec_type 	   angles;
	q_matrix_type  colMatrix;
	A3DVAL         X_val, Y_val, Z_val;

	printf("Setting sound coords\n");
	// for right now we ignore everything but position since we assume omnidirectional sounds
	a3dsamples[id]->SetPosition3f(soundDef.pose.position[0],soundDef.pose.position[1],soundDef.pose.position[2]);

	// do angles as in listener.. look there for a bit more commentation
	q_to_col_matrix (colMatrix, soundDef.pose.orientation);
	q_col_matrix_to_euler(angles,colMatrix);

	X_val = angles[0]*180.0/Q_PI;
	Y_val = angles[1]*180.0/Q_PI;
	Z_val = angles[2]*180.0/Q_PI;
	a3dsamples[id]->SetOrientationAngles3f(Y_val, X_val, Z_val);
	
	a3dsamples[id]->SetMinMaxDistance(soundDef.max_front_dist, soundDef.max_front_dist, A3D_MUTE);
	return;
}

void vrpn_Sound_Server_A3D::initModel(vrpn_ModelDef modelDef) {}
	
void vrpn_Sound_Server_A3D::mainloop(const struct timeval * timeout) {
	lasterror = a3droot->Clear();
	if (!connection->connected())
	{
		unloadCSMap();
	}
	connection->mainloop(timeout);
	lasterror = a3droot->Flush();
}
bool vrpn_Sound_Server_A3D::noSounds(void) {return 0;}
void vrpn_Sound_Server_A3D::stopAllSounds() {
  for (int i(0); i<numSounds;i++)
	  a3dsamples[i]->Stop();
}

void vrpn_Sound_Server_A3D::shutDown() {
	a3droot->Shutdown();
}
vrpn_int32  vrpn_Sound_Server_A3D::GetCurrentVolume(const vrpn_int32 CurrentSoundId) {
	float val;
	lasterror = a3dsamples[CurrentSoundId]->GetGain(&val);
	return val;
}

char * vrpn_Sound_Server_A3D::GetLastError() {
	char buf[1024];
	if (FAILED(lasterror))
	sprintf(buf,"ERROR: %d",lasterror);
	return buf;
}

vrpn_int32  vrpn_Sound_Server_A3D::GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId) {
 return 1;
}
void vrpn_Sound_Server_A3D::GetCurrentPosition(const vrpn_int32 CurrentSoundId, float* X_val, float* Y_val, float* Z_val) {
	lasterror =  a3dsamples[CurrentSoundId]->GetPosition3f(X_val, Y_val, Z_val);
	return;
}
void vrpn_Sound_Server_A3D::GetListenerPosition(float* X_val, float* Y_val, float* Z_val) {
    lasterror =  a3dlis->GetPosition3f(X_val, Y_val, Z_val);
	return;
}

void vrpn_Sound_Server_A3D::GetCurrentDistances(const vrpn_int32 CurrentSoundId, float* FMin, float* FMax) {
	lasterror =  a3dsamples[CurrentSoundId]->GetMinMaxDistance(FMax, FMin,NULL);
	return;
}

void vrpn_Sound_Server_A3D::GetListenerOrientation(float* X_val, float *Y_val, float *Z_val) {
	// z y x is what we expect to get back..  these arent named right but deal for now
	lasterror =  a3dlis->GetOrientationAngles3f(Y_val, Z_val, X_val);
	return;
}
void vrpn_Sound_Server_A3D::GetCurrentOrientation(const vrpn_int32 CurrentSoundId,float *X_val, float *Y_val, float *Z_val) {
	// z y x is what we expect to get back..  these arent named right but deal for now
	lasterror =  a3dsamples[CurrentSoundId]->GetOrientationAngles3f(Y_val, Z_val, X_val);
	return;
}

	
const int CHAR_BUF_SIZE =1024;
HWND GetConsoleHwnd(void)
{
	// Get the current window title.
	char pszOldWindowTitle[CHAR_BUF_SIZE];
	GetConsoleTitle(pszOldWindowTitle, CHAR_BUF_SIZE);

	// Format a unique New Window Title.
	char pszNewWindowTitle[CHAR_BUF_SIZE];
	wsprintf(pszNewWindowTitle, "%d/%d", GetTickCount(), GetCurrentProcessId());

	// Change the current window title.
	SetConsoleTitle(pszNewWindowTitle);

	// Ensure window title has changed.
	Sleep(40);

	// Look for the new window.
	HWND hWnd = FindWindow(NULL, pszNewWindowTitle);

	// Restore orignal name
	SetConsoleTitle(pszOldWindowTitle);

	return hWnd;
}

int got_report;
void main(int argc, char **argv) {

HWND hWin;
vrpn_Sound_Server_A3D * soundServer = NULL;
vrpn_Tracker_Remote   * tracker_connection;
char                    tracker_device[512];
char                    tracker_name[512];
vrpn_Connection       * connection;
vrpn_Connection       * trackerCon;

int USE_TRACKER;

	char	* config_file_name = "vrpn.cfg";
	FILE	* config_file;
	char 	* client_name   = NULL;
	int	      client_port;
	int	      bail_on_error = 1;
	int	      verbose       = 1;
	int	      auto_quit     = 0;
	int	      realparams    = 0;
	int 	  loop          = 0;
	int	      port          = vrpn_DEFAULT_LISTEN_PORT_NO;

	connection = new vrpn_Synchronized_Connection (port);
	
	// Open the configuration file
	if (verbose) printf("Reading from config file %s\n", config_file_name);
	
	if ( (config_file = fopen(config_file_name, "r")) == NULL) 
	{
		perror("Cannot open config file");
		printf("  (filename %s)\n", config_file_name);
		return;
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
				printf("Line too long in config file: %s\n",line);
				if (bail_on_error) { return; }
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
				printf("%s\n",pch); 
				next();
				printf("%s\n",pch);
				if (sscanf(pch,"%511s\t%d\t%511s\t%511s",s2,&USE_TRACKER,tracker_name, tracker_device) != 4) 
				{
					printf("Bad vrpn_Server_Sound line: %s\n",line);
					if (bail_on_error) 
					{ 
						return; 
					}
					else 
					{ 
						continue; 
					}	// Skip this line
				}

				hWin = GetConsoleHwnd();

				printf("about to init sound server\n");	
				soundServer = NULL;
				soundServer = new vrpn_Sound_Server_A3D(s2, connection,hWin);
				if (soundServer == NULL) 
					printf("Can't create sound server\n");
				
			}
#endif
		}


	}

	fclose(config_file);

	// Open remote tracker if we are to use one

	if (USE_TRACKER) {
		
		char newname[1024];
		sprintf(newname,"%s@%s",(const char*)tracker_device, (const char*)tracker_name);
		printf("Using tracker: %s\n",newname);
		trackerCon = vrpn_get_connection_by_name(tracker_name);
		tracker_connection = new vrpn_Tracker_Remote((const char *) newname);
		// SET UP TRACKER HANDLER
		if (trackerCon->doing_okay()) {
			printf( "TC OK.\n");
		} else {
			printf( "TC Not OK.\n");
		}
	}
	else printf("Not using tracker\n");

	// Open the Forwarder Server

	loop = 0;
	
	if (client_name) 
	{
		printf( "vrpn_serv: connecting to client: %s:%d\n",
			client_name, client_port);
		if (connection->connect_to_client(client_name, client_port))
		{
			printf( "server: could not connect to client %s:%d\n", client_name, client_port);
		}
	}

	// for testing we load up a sound and play it loopingly
		
// ********************************************************************
// **                                                                **
// **                MAIN LOOP                                       **
// **                                                                **
// ********************************************************************
float fPrevTime = 0.0f;
float fFrameTime;
float fTime;
int counter = 0;
int stopNow = 0;

	printf("Begin main loop\n");
	while (!stopNow && 	!_kbhit()) {
	  if (!strncmp(soundServer->GetLastError(),"ERROR",5))
		 printf(soundServer->GetLastError());
		

      counter++;
  	  // record time since last frame 
      if (counter==300) {
	    fTime = (float)timeGetTime();
	    counter = 0;

	    fFrameTime = (fTime - fPrevTime) * 0.001f;
	  
	    printf("Running at %4.2f Hz\n", 300.0/fFrameTime);

        fPrevTime = fTime;
	  }
        Sleep(1);  // might take this out once we do the rendering stuff
		soundServer->mainloop();
						
		// ensure we get a new report!
		if (USE_TRACKER) {
		  tracker_connection->mainloop();
		  got_report = 0;
		  if (trackerCon->doing_okay())
		    while (!got_report) 
			  tracker_connection->mainloop(); 
		}

		// Send and receive all messages
		connection->mainloop();
		if (!connection->doing_okay())  {
			soundServer->shutDown();
		    stopNow =1;
		}
	}

	// print some debug message stuff
	soundServer->shutDown();
}