//	vrpn_Sound_DX9.cpp
//
//	July 2003- Matt McCallus

/// @todo Requires deprecated DXERR.lib for DXTrace and DXTRACE_ERR
#ifdef _WIN32


#include "vrpn_Sound_DX9.h"
#include <dxerr8.h>
#include <dsound.h>

#define NUM_SPIN 10000
#define MAX_NUM_SOUNDS 16
const int CHAR_BUF_SIZE = 1024;


HRESULT Get3DListenerInterface(LPDIRECTSOUND3DLISTENER8 *ppDSListener);


// DirectX9 stuff
LPDIRECTSOUND8			m_pDS					= NULL;
LPDIRECTSOUNDBUFFER		pDSBPrimary				= NULL;
LPDIRECTSOUND3DBUFFER   g_pDS3DBuffer			= NULL;		// 3D sound buffers
LPDIRECTSOUND3DLISTENER g_pDSListener			= NULL;		// 3D listener object                                                    
DS3DBUFFER              g_dsBufferParams;					// 3D buffer properties
DS3DLISTENER            g_dsListenerParams;					// Listener properties
BOOL                    g_bDeferSettings		= FALSE;
int						g_numSounds				= 0;
SoundMap				g_soundMap[MAX_NUM_SOUNDS];			
Sound					g_Sound[MAX_NUM_SOUNDS];			// wrapper for DX9Sound obj

vrpn_Sound_Server_DX9::vrpn_Sound_Server_DX9(const char      * name, 
											 vrpn_Connection * c, 
											 HWND              hWin) 
											 : vrpn_Sound_Server(name, c){
	try {

		HRESULT hr;

		for(int i=0; i<MAX_NUM_SOUNDS; i++)
			g_Sound[i].m_pSound = NULL;

		// Create IDirectSound using the primary sound device
		hr = DirectSoundCreate8( NULL, &m_pDS, NULL );

		// Set DirectSound coop level 
		hr = m_pDS->SetCooperativeLevel( hWin, DSSCL_PRIORITY );

		hr = m_pDS->SetSpeakerConfig( DSSPEAKER_HEADPHONE );

		// Get the primary buffer 
		DSBUFFERDESC dsbd;
		ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
		dsbd.dwSize        = sizeof(DSBUFFERDESC);
		dsbd.dwFlags       = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER ;
		dsbd.dwBufferBytes = 0;
		dsbd.lpwfxFormat   = NULL;

		hr = m_pDS->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL );

		// Get the 3D Listener Interface
		Get3DListenerInterface( &g_pDSListener );

		WAVEFORMATEX wfx;
		ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
		wfx.wFormatTag      = (WORD) WAVE_FORMAT_PCM; 
		wfx.nChannels       = 1; 
		wfx.nSamplesPerSec  = 22050; 
		wfx.wBitsPerSample  = 16; 
		wfx.nBlockAlign     = (WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
		wfx.nAvgBytesPerSec = (DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);

		hr = pDSBPrimary->SetFormat(&wfx);

		pDSBPrimary->Release();
		// Get listener parameters
		g_dsListenerParams.dwSize = sizeof(DS3DLISTENER);
		g_pDSListener->GetAllParameters( &g_dsListenerParams );

		// set up the sound map.
		g_numSounds = 0;

		for(i=0; i<MAX_NUM_SOUNDS; i++){
			g_soundMap[i].m_iSoundNum = -1;
			for(int j=0; j<80; j++)
				g_soundMap[i].m_DX9SoundName[j] = '\0';
		}

	}
	catch(char* szError) {
		// Display the message.
		printf("AudioInit() - %s.\n", szError);
		send_message("Error initializing sound card.. sound server did not start.",vrpn_TEXT_ERROR,0);
		exit(0);
	}
}



vrpn_Sound_Server_DX9::~vrpn_Sound_Server_DX9() {

	//release the interfaces..
	SAFE_RELEASE(g_pDSListener);
	SAFE_RELEASE(g_pDS3DBuffer);
	
	for(int i=0; i<MAX_NUM_SOUNDS; i++)
		SAFE_DELETE(g_Sound[i].m_pSound);
}


void vrpn_Sound_Server_DX9::playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef) {

	HRESULT hr;

	if(id<0 || id>g_numSounds){
		send_message("Error: playSound (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	int myid = g_soundMap[id].m_iSoundNum;
	g_Sound[myid].repeat = repeat;

	if (myid != -1) {
		if(g_Sound[id].m_pSound != NULL){

			if (repeat == 0)
				hr = g_Sound[myid].m_pSound->Play( 0, DSBPLAY_LOOPING , g_Sound[myid].m_pSound->GetBufferVolume() );
			else {
				hr = g_Sound[myid].m_pSound->Play( 0, NULL , g_Sound[myid].m_pSound->GetBufferVolume() );
				g_Sound[myid].repeat--;
				
			}
			send_message("Playing sound\n",vrpn_TEXT_NORMAL,0);
		}
		else send_message("Error: playSound (Sound not loaded)",vrpn_TEXT_WARNING,0);
	}
	else send_message("Error: playSound (Sound not loaded)",vrpn_TEXT_WARNING,0);
}



void vrpn_Sound_Server_DX9::stopSound(vrpn_SoundID id) {

	float x;
	float y;
	float z;
	GetCurrentOrientation( 0, &x, &y, &z );


	if(id<0 || id>g_numSounds){
		send_message("Error: stopSound (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	int myid = g_soundMap[id].m_iSoundNum;

	if ( myid != -1 && g_Sound[myid].m_pSound != NULL ) {
		hr = g_Sound[myid].m_pSound->Stop();
		hr = g_Sound[myid].m_pSound->Reset();
		send_message("Stopping sound",vrpn_TEXT_NORMAL,0);
	} 
	else{
		send_message("Invalid sound id",vrpn_TEXT_ERROR,0);
		printf("Invalid sound id\n");
	}
}



void vrpn_Sound_Server_DX9::loadSoundLocal(char* filename, vrpn_SoundID id, vrpn_SoundDef soundDef) {

	char tempbuf[1024];	
    GUID    guid3DAlgorithm = GUID_NULL;
    HRESULT hr; 

    // Free any previous sound, and make a new one
    SAFE_DELETE( g_Sound[id].m_pSound );

    // Verify the file is small
    HANDLE hFile = CreateFile( filename, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    if( hFile != NULL )
    {
        // If you try to open a 100MB wav file, you could run out of system memory with this
        // sample cause it puts all of it into a large buffer.  If you need to do this, then 
        // see the "StreamData" sample to stream the data from the file into a sound buffer.
        DWORD dwFileSizeHigh = 0;
        DWORD dwFileSize = GetFileSize( hFile, &dwFileSizeHigh );
        CloseHandle( hFile );

        if( dwFileSizeHigh != 0 || dwFileSize > 1000000 )
        {
			sprintf(tempbuf,"File too large.  You should stream large files.");
			printf("%s\n", tempbuf);
			send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

            return;
        }
    }

    DX9WaveFile waveFile;
    waveFile.Open( filename, NULL, 1 );
    WAVEFORMATEX* pwfx = waveFile.GetFormat();
    if( pwfx == NULL )
    {
		sprintf(tempbuf,"Invalid wave file format.");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

        return;
    }

    if( pwfx->nChannels > 1 )
    {
        // Too many channels in wave.  Sound must be mono when using DSBCAPS_CTRL3D
		sprintf(tempbuf,"Wave file must be mono for 3D control.");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

		return;
    }

    if( pwfx->wFormatTag != WAVE_FORMAT_PCM )
    {
        // Sound must be PCM when using DSBCAPS_CTRL3D
		sprintf(tempbuf,"Wave file must be PCM for 3D control.");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

        return;
    }

	// Set the proper DirectX 3D Audio Algorithm
    guid3DAlgorithm = DS3DALG_HRTF_FULL;

	// Load the sound id and name(path) into the sound map
	g_soundMap[g_numSounds].m_iSoundNum = (int)id;
	char* nameptr = g_soundMap[g_numSounds].m_DX9SoundName;
	strcpy(nameptr, filename);
	int myid = g_soundMap[g_numSounds].m_iSoundNum;

	g_numSounds++;

    // Load the wave file into a DirectSound buffer
	hr = CreateSecondaryBuffer( &g_Sound[myid].m_pSound, filename, DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME  | DSBCAPS_STICKYFOCUS, guid3DAlgorithm, 1 );

    if( FAILED( hr ) || hr == DS_NO_VIRTUALIZATION )
    {
        DXTRACE_ERR( TEXT("Create"), hr );
        if( DS_NO_VIRTUALIZATION == hr )
        {
			sprintf(tempbuf,"The 3D virtualization algorithm requested is not supported under this "
                        "operating system.  It is available only on Windows 2000, Windows ME, and Windows 98 with WDM "
                        "drivers and beyond.  Creating buffer with no virtualization.");
			printf("%s\n", tempbuf);
			send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
        }

        // Unknown error, but not a critical failure, so just update the status
		sprintf(tempbuf,"Could not create sound buffer.\n");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

        return; 
    }

    // Get the 3D buffer from the secondary buffer
    hr = g_Sound[myid].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );

    // Get the 3D buffer parameters
    g_dsBufferParams.dwSize = sizeof(DS3DBUFFER);
    g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

    // Set new 3D buffer parameters
    g_dsBufferParams.dwMode = DS3DMODE_NORMAL;

	// Set up sound parameters
	D3DVALUE pos_x = (float)soundDef.pose.position[0];
	D3DVALUE pos_y = (float)soundDef.pose.position[1];
	D3DVALUE pos_z = (float)soundDef.pose.position[2];
	g_pDS3DBuffer->SetPosition( pos_x, pos_y, pos_z, DS3D_IMMEDIATE );

	D3DVALUE ori_x = (float)soundDef.pose.orientation[0];
	D3DVALUE ori_y = (float)soundDef.pose.orientation[1];
	D3DVALUE ori_z = (float)soundDef.pose.orientation[2];
	g_pDS3DBuffer->SetConeOrientation(ori_x,ori_y,ori_z, DS3D_IMMEDIATE );

	long InsideConeAngle = (long)soundDef.cone_inner_angle;
	long OutsideConeAngle = (long)soundDef.cone_outer_angle;
	g_pDS3DBuffer->SetConeAngles(InsideConeAngle, OutsideConeAngle, DS3D_IMMEDIATE);

	long OutsideVolume = (long)soundDef.cone_gain;
	g_pDS3DBuffer->SetConeOutsideVolume(OutsideVolume, DS3D_IMMEDIATE);

	float MinDistance = (float)soundDef.min_front_dist;
	float MaxDistance = (float)soundDef.max_front_dist;
	g_pDS3DBuffer->SetMinDistance(MinDistance, DS3D_IMMEDIATE);
	g_pDS3DBuffer->SetMaxDistance(MaxDistance, DS3D_IMMEDIATE);

	D3DVALUE velocity_x = (float)soundDef.velocity[0];
	D3DVALUE velocity_y = (float)soundDef.velocity[1];
	D3DVALUE velocity_z = (float)soundDef.velocity[2];
	g_pDS3DBuffer->SetVelocity(velocity_x, velocity_y, velocity_z, DS3D_IMMEDIATE);

    DSBCAPS dsbcaps;
    ZeroMemory( &dsbcaps, sizeof(DSBCAPS) );
    dsbcaps.dwSize = sizeof(DSBCAPS);

    LPDIRECTSOUNDBUFFER pDSB = g_Sound[myid].m_pSound->GetBuffer( 0 );

	sprintf(tempbuf,"Loading sound #%d: %s                    \n",id,filename);
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

    pDSB->GetCaps( &dsbcaps );
	if( ( dsbcaps.dwFlags & DSBCAPS_LOCHARDWARE ) != 0 ) {
		sprintf(tempbuf,"File loaded using hardware mixing.           \n");
		printf("%s", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
	}
	else {
		sprintf(tempbuf,"File loaded using software mixing.             \n");
		printf("%s", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
	}

	GetSoundList();

	return;

}



// not supported
void vrpn_Sound_Server_DX9::loadSoundRemote(char* filename, vrpn_SoundID id, vrpn_SoundDef soundDef) {

	send_message("loadSoundRemote not supported",vrpn_TEXT_WARNING,0);
}




void vrpn_Sound_Server_DX9::unloadSound(vrpn_SoundID id) {

	if(id<0 || id>g_numSounds){
		send_message("Error: unloadSound (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	char tempbuf[1024];
	int myid = g_soundMap[id].m_iSoundNum;

	if (myid==-1 || g_Sound[myid].m_pSound == NULL) {
		sprintf(tempbuf,"Error: unloadSound(Invalid id)                    ");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

	// move everything down by one
	for(int i=myid; i<g_numSounds; i++){
		
		if(g_Sound[i].m_pSound->IsSoundPlaying()){
			g_Sound[i].m_pSound->Stop();
		}
		char* tempptr1 = g_soundMap[i].m_DX9SoundName;
		strcpy( tempptr1, &g_soundMap[i+1].m_DX9SoundName[0] );
		g_Sound[i].m_pSound = g_Sound[i+1].m_pSound;
		g_Sound[i].repeat = g_Sound[i+1].repeat;
	}

	g_numSounds--;

	sprintf(tempbuf,"Unloading sound: %d                     ",myid);
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

	GetSoundList();

	return;
}



// We use at and up vectors in DirectX...
// Use setListenerPosition and setListenerOrientation instead.
void vrpn_Sound_Server_DX9::setListenerPose(vrpn_PoseDef pose) {

	D3DVALUE pos_x = (float)pose.position[0];
	D3DVALUE pos_y = (float)pose.position[1];
	D3DVALUE pos_z = (float)pose.position[2];

	g_pDSListener->SetPosition( pos_x, pos_y, pos_z, DS3D_IMMEDIATE );

	D3DVECTOR up,at;
	q_matrix_type mat;

	q_to_col_matrix( mat, pose.orientation );

	at.x = -(float)mat[0][1];
	at.y = -(float)mat[1][1];
	at.z = (float)mat[2][1];

	up.x = (float)mat[0][2];
	up.y = (float)mat[1][2];
	up.z = -(float)mat[2][2];

//	printf("DEBUG: %f %f %f %f %f %f\n",
//			at.x,at.y, at.z, up.x, up.y, up.z);

	g_pDSListener->SetOrientation( at.x, at.y, at.z, up.x, up.y, up.z, DS3D_IMMEDIATE );

	return;
}




void vrpn_Sound_Server_DX9::setListenerVelocity(vrpn_float64 velocity[4]) {

	D3DVALUE velocity_x = (float)velocity[0];
	D3DVALUE velocity_y = (float)velocity[1];
	D3DVALUE velocity_z = (float)velocity[2];

	g_pDSListener->SetVelocity( velocity_x, velocity_y, velocity_z, DS3D_IMMEDIATE );

	return;
}




void vrpn_Sound_Server_DX9::changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef) {

	if(id<0 || id>g_numSounds){
		send_message("Error: changeSoundStatus (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	vrpn_int32     myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {

		sprintf(tempbuf,"Error: changeSoundStatus(Invalid id)          ");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

    // Get the 3D buffer from the secondary buffer
    g_Sound[id].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );

    // Get the 3D buffer parameters
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	// Set up sound parameters
	g_dsBufferParams.vPosition.x = (float)soundDef.pose.position[0];
	g_dsBufferParams.vPosition.y = (float)soundDef.pose.position[1];
	g_dsBufferParams.vPosition.z = (float)soundDef.pose.position[2];
 
	g_dsBufferParams.vConeOrientation.x = (float)soundDef.pose.orientation[0];
	g_dsBufferParams.vConeOrientation.y = (float)soundDef.pose.orientation[1];
	g_dsBufferParams.vConeOrientation.z = (float)soundDef.pose.orientation[2];

	g_dsBufferParams.dwInsideConeAngle = (long)soundDef.cone_inner_angle;
	g_dsBufferParams.dwOutsideConeAngle = (long)soundDef.cone_outer_angle;
	g_dsBufferParams.lConeOutsideVolume = (long)soundDef.cone_gain;

	g_dsBufferParams.flMinDistance = (float)soundDef.min_front_dist;
	g_dsBufferParams.flMaxDistance = (float)soundDef.max_front_dist;

	g_dsBufferParams.vVelocity.x = (float)soundDef.velocity[0];
	g_dsBufferParams.vVelocity.y = (float)soundDef.velocity[1];
	g_dsBufferParams.vVelocity.z = (float)soundDef.velocity[2];

	// Now apply all parameters.	
    hr = g_pDS3DBuffer->SetAllParameters( &g_dsBufferParams, DS3D_IMMEDIATE );

	return;
}




void vrpn_Sound_Server_DX9::setSoundPose(vrpn_SoundID id, vrpn_PoseDef pose) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundPose (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	int myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundPose(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

    // Get the 3D buffer from the secondary buffer
    hr = g_Sound[myid].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );

    // Get the 3D buffer parameters
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	D3DVALUE pos0 = (float)pose.position[0];
	D3DVALUE pos1 = (float)pose.position[1];
	D3DVALUE pos2 = (float)pose.position[2];
	D3DVALUE ori0 = (float)pose.orientation[0];
	D3DVALUE ori1 = (float)pose.orientation[1];
	D3DVALUE ori2 = (float)pose.orientation[2];

	g_pDS3DBuffer->SetPosition(pos0, pos1, pos2, DS3D_IMMEDIATE);
	g_pDS3DBuffer->SetConeOrientation(ori0, ori1, ori2, DS3D_IMMEDIATE);
	g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	return;
}




void vrpn_Sound_Server_DX9::setSoundVelocity(vrpn_SoundID id, vrpn_float64 *velocity) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundVelocity (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	vrpn_int32     myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {

		sprintf(tempbuf,"Error: setSoundVelocity(Invalid id)");
		printf("%s", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

	// Get the 3D buffer from the secondary buffer
    g_Sound[id].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );

    // Get the 3D buffer parameters
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	// Set up sound parameters
	g_dsBufferParams.vVelocity.x = (float)velocity[0];
	g_dsBufferParams.vVelocity.y = (float)velocity[1];
	g_dsBufferParams.vVelocity.z = (float)velocity[2];

	// Now apply all parameters.	
    hr = g_pDS3DBuffer->SetAllParameters( &g_dsBufferParams, DS3D_IMMEDIATE );

	return;
}




void vrpn_Sound_Server_DX9::setSoundDistInfo(vrpn_SoundID id, vrpn_float64 *distinfo) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundDistInfo (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}
	
	HRESULT hr;
	vrpn_int32     myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundDistInfo(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

    // Get the 3D buffer from the secondary buffer
    g_Sound[id].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );

    // Get the 3D buffer parameters
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	// Set up sound parameters
	// distinfo[2] is front min
	// distinfo[3] is front max
	g_dsBufferParams.flMinDistance = (float)distinfo[2];
	g_dsBufferParams.flMaxDistance = (float)distinfo[3];

	// Now apply all parameters.	
    hr = g_pDS3DBuffer->SetAllParameters( &g_dsBufferParams, DS3D_IMMEDIATE );

	return;
}




void vrpn_Sound_Server_DX9::setSoundConeInfo(vrpn_SoundID id, vrpn_float64 *coneinfo) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundConeInfo (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	vrpn_int32     myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundConeInfo(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

    // Get the 3D buffer from the secondary buffer
    g_Sound[id].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );

    // Get the 3D buffer parameters
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	// Set up sound parameters
	g_dsBufferParams.vConeOrientation.x = (float)coneinfo[0];
	g_dsBufferParams.vConeOrientation.y = (float)coneinfo[1];
	g_dsBufferParams.vConeOrientation.z = (float)coneinfo[2];

	g_dsBufferParams.dwInsideConeAngle = (long)coneinfo[3];
	g_dsBufferParams.dwOutsideConeAngle = (long)coneinfo[4];
	g_dsBufferParams.lConeOutsideVolume = (long)coneinfo[5];
	
	// Now apply all parameters.	
    hr = g_pDS3DBuffer->SetAllParameters( &g_dsBufferParams, DS3D_IMMEDIATE );

	return;
}



// With DirectSound this is really a property of the Listener,
// so vrpn_SoundID id is not used.
void vrpn_Sound_Server_DX9::setSoundDoplerFactor(vrpn_SoundID id, vrpn_float64 doplerfactor) {

	// Still check for sane user anyway.
	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundDoplerFactor (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	vrpn_int32     myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundDoplerFactor(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

    // Set the Listener parameters
	hr = g_pDSListener->SetDopplerFactor( (float)doplerfactor, DS3D_IMMEDIATE );

	return;
}



void vrpn_Sound_Server_DX9::setSoundEqValue(vrpn_SoundID id, vrpn_float64 eqvalue) {

	send_message("setSoundEqValue not supported",vrpn_TEXT_WARNING,0);

	return;
}




void vrpn_Sound_Server_DX9::setSoundPitch(vrpn_SoundID id, vrpn_float64 pitch) {

	send_message("setSoundPitch not supported",vrpn_TEXT_WARNING,0);

	return;
}




void vrpn_Sound_Server_DX9::setSoundVolume(vrpn_SoundID id, vrpn_float64 volume) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundVolume (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	vrpn_int32     myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundVolume(Invalid id)\n");
		printf("%s", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

	g_Sound[id].m_pSound->SetBufferVolume((long)volume);

	return;
}




void vrpn_Sound_Server_DX9::loadModelLocal(const char * filename) {

	char tempbuf[1024];

	sprintf(tempbuf,"loadModelLocal not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;

}




void vrpn_Sound_Server_DX9::loadModelRemote(){

	char tempbuf[1024];

	sprintf(tempbuf,"loadModelRemote not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}	




void vrpn_Sound_Server_DX9::loadPolyQuad(vrpn_QuadDef * quad) {

	char tempbuf[1024];

	sprintf(tempbuf,"loadPolyQuad not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}




void vrpn_Sound_Server_DX9::loadPolyTri(vrpn_TriDef * tri) {

	char tempbuf[1024];

	sprintf(tempbuf,"loadPolyTri not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}




void vrpn_Sound_Server_DX9::loadMaterial(vrpn_MaterialDef * material, vrpn_int32 id) {

	char tempbuf[1024];

	sprintf(tempbuf,"loadMaterial not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}




void vrpn_Sound_Server_DX9::setPolyQuadVertices(vrpn_float64 vertices[4][3], const vrpn_int32 id) {

	char tempbuf[1024];

	sprintf(tempbuf,"setPolyQuadVertices not implemented");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}




void vrpn_Sound_Server_DX9::setPolyTriVertices(vrpn_float64 vertices[3][3], const vrpn_int32 id) {

	char tempbuf[1024];

	sprintf(tempbuf,"setPolyTriVertices not implemented");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}




void vrpn_Sound_Server_DX9::setPolyOF(vrpn_float64 OF, vrpn_int32 tag) {

	char tempbuf[1024];

	sprintf(tempbuf,"setPolyOF not implemented");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}




void vrpn_Sound_Server_DX9::setPolyMaterial(const char * material, vrpn_int32 tag) {

	char tempbuf[1024];

	sprintf(tempbuf,"setPolyMaterial not implemented");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}




vrpn_float64  vrpn_Sound_Server_DX9::GetCurrentVolume(const vrpn_int32 id) {

	if(id<0 || id>g_numSounds){
		send_message("Error: GetCurrentVolume (Invalid id)",vrpn_TEXT_WARNING,0);
		return -1.0f;
	}

	HRESULT	hr;
	char tempbuf[1024];
	long val;
	vrpn_int32 myid = g_soundMap[id].m_iSoundNum;

	if (myid==-1) {
		sprintf(tempbuf,"Error: GetCurrentVolume(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return -1.0f;
	}

    // Get the 3D buffer from the secondary buffer
    hr = g_Sound[id].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	val = g_dsBufferParams.lConeOutsideVolume;

	return val;
}



// not supported in DirectX
vrpn_int32  vrpn_Sound_Server_DX9::GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId) {

	char tempbuf[1024];

	sprintf(tempbuf,"GetCurrentPlaybackRate not available in DirectX");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return -1;
}




void vrpn_Sound_Server_DX9::GetCurrentPosition(const vrpn_int32 id, float* X_val, float* Y_val, float* Z_val) {

	if(id<0 || id>g_numSounds){
		send_message("Error: GetCurrentPosition (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	char tempbuf[1024];
	vrpn_int32 myid = g_soundMap[id].m_iSoundNum;
	D3DVECTOR val;

	if (myid==-1) {

		sprintf(tempbuf,"Error: GetCurrentPosition(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return ;
	}

    // Get the 3D buffer from the secondary buffer
    g_Sound[id].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	hr = g_pDS3DBuffer->GetPosition( &val );

	*X_val = val.x;
	*Y_val = val.y;
	*Z_val = val.z;

	return;
}




void vrpn_Sound_Server_DX9::GetListenerPosition(float* X_val, float* Y_val, float* Z_val) {

	HRESULT hr;
	D3DVECTOR pos;

	hr = g_pDSListener->GetPosition( &pos );

	printf("DEBUG: %f %f %f \n\n",pos.x, pos.y, pos.z);

	*X_val = pos.x;
	*Y_val = pos.y;
	*Z_val = pos.z;

	return;
}




void vrpn_Sound_Server_DX9::GetCurrentDistances(const vrpn_int32 id, float* FMin, float* FMax) {

	if(id<0 || id>g_numSounds){
		send_message("Error: GetCurrentDistances (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	char tempbuf[1024];
	int myid = g_soundMap[id].m_iSoundNum;

	if (myid==-1) {

		sprintf(tempbuf,"Error: GetCurrentDistances(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return ;
	}

    // Get the 3D buffer from the secondary buffer
    g_Sound[myid].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	hr = g_pDS3DBuffer->GetMinDistance( FMin );
	hr = g_pDS3DBuffer->GetMaxDistance( FMax );

	return;
}




void vrpn_Sound_Server_DX9::GetListenerOrientation(double* y_val, double *p_val, double *r_val) {

	send_message("GetListenerOrientation not supported",vrpn_TEXT_WARNING,0);

	return;
}



// Get sound's orientation (sound cone orientation)
// In DirectX the sound cone orientation is a vector
void vrpn_Sound_Server_DX9::GetCurrentOrientation(const vrpn_int32 id,float *x, float *y, float *z) {

	if(id<0 || id>g_numSounds){
		send_message("Error: GetCurrentOrientation (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	HRESULT hr;
	char tempbuf[1024];
	int myid = g_soundMap[id].m_iSoundNum;
	D3DVECTOR ori;

	if (myid==-1) {

		sprintf(tempbuf,"Error: GetCurrentOrientation(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return ;
	}

    // Get the 3D buffer from the secondary buffer
    g_Sound[myid].m_pSound->Get3DBufferInterface( 0, &g_pDS3DBuffer );
    hr = g_pDS3DBuffer->GetAllParameters( &g_dsBufferParams );

	hr = g_pDS3DBuffer->GetConeOrientation( &ori );

	*x = ori.x;
	*y = ori.y;
	*z = ori.z;

	return;
}



void vrpn_Sound_Server_DX9::mainloop() {

	vrpn_Text_Sender::mainloop();
	vrpn_Sound::d_connection->mainloop();
	
	vrpn_SoundDef temp;
	temp.volume = 0.0f; // suppress VC6 initialization warning with this
	// check if we need to repeat any sounds
	for(int i=0; i<g_numSounds; i++) {
		if(g_Sound[i].repeat != 0) {
			if(!g_Sound[i].m_pSound->IsSoundPlaying()) {
				playSound(g_soundMap[i].m_iSoundNum, g_Sound[i].repeat, temp);
			}
		}
	}

	return;
}



bool vrpn_Sound_Server_DX9::noSounds(void) {return 0;}



void vrpn_Sound_Server_DX9::stopAllSounds() {

	for (int i(0); i<g_numSounds;i++)
		g_Sound[i].m_pSound->Stop();

	return;
}



void vrpn_Sound_Server_DX9::shutDown() {

	for (int i=0; i<MAX_NUM_SOUNDS; i++) {
		g_Sound[i].m_pSound = NULL;
		g_soundMap[i].m_iSoundNum=-1;
	}

	g_numSounds = 0;
}



void vrpn_Sound_Server_DX9::GetSoundList() {

	char tempbuf[1024];

	printf("\nCurrent sounds loaded***********\n");

	for(int i=0; i<g_numSounds; i++){

		sprintf(tempbuf,"Sound# %d: %s",i,g_soundMap[i].m_DX9SoundName);
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
	}

	printf("********************************\n");

	return;
}



// Create a secondary buffer
HRESULT vrpn_Sound_Server_DX9::CreateSecondaryBuffer( DX9Sound** ppSound, 
													  LPTSTR strWaveFileName, 
													  DWORD dwCreationFlags, 
													  GUID guid3DAlgorithm,
													  DWORD dwNumBuffers )
{
    HRESULT hr;
    HRESULT hrRet = S_OK;
    LPDIRECTSOUNDBUFFER*	apDSBuffer     = NULL;
    DWORD					dwDSBufferSize = NULL;
    DX9WaveFile*			pWaveFile      = NULL;

    if( m_pDS == NULL )
        return CO_E_NOTINITIALIZED;
    if( strWaveFileName == NULL || ppSound == NULL || dwNumBuffers < 1 )
        return E_INVALIDARG;

    apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
    pWaveFile = new DX9WaveFile();

    pWaveFile->Open( strWaveFileName, NULL, 1 );

    // Make the DirectSound buffer the same size as the wav file
    dwDSBufferSize = pWaveFile->GetSize();

    // Create the direct sound buffer, and only request the flags needed
    // since each requires some overhead and limits if the buffer can 
    // be hardware accelerated
    DSBUFFERDESC dsbd;
    ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
    dsbd.dwSize          = sizeof(DSBUFFERDESC);
    dsbd.dwFlags         = dwCreationFlags;
    dsbd.dwBufferBytes   = dwDSBufferSize;
    dsbd.guid3DAlgorithm = guid3DAlgorithm;
    dsbd.lpwfxFormat     = pWaveFile->m_pwfx;
    
    // DirectSound is only guaranteed to play PCM data.  Other
    // formats may or may not work depending the sound card driver.
    hr = m_pDS->CreateSoundBuffer( &dsbd, &apDSBuffer[0], NULL );

    // Create the sound
    *ppSound = new DX9Sound( apDSBuffer, dwDSBufferSize, dwNumBuffers, pWaveFile, dwCreationFlags );
    
    SAFE_DELETE( apDSBuffer );

    return hrRet;
}

HRESULT Get3DListenerInterface(LPDIRECTSOUND3DLISTENER *ppDSListener) {

	HRESULT hr;

	hr = pDSBPrimary->QueryInterface( IID_IDirectSound3DListener, 
												  (VOID**)ppDSListener );

	return hr;

}



//********************************************************
//
// DX9 Sound Class
//
//********************************************************
DX9Sound::DX9Sound( LPDIRECTSOUNDBUFFER* apDSBuffer, DWORD dwDSBufferSize, 
                DWORD dwNumBuffers, DX9WaveFile* pWaveFile, DWORD dwCreationFlags )
{
	m_fvolume = 0;

    m_apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];

    for(DWORD i=0; i<dwNumBuffers; i++ )
        m_apDSBuffer[i] = apDSBuffer[i];

    m_dwDSBufferSize = dwDSBufferSize;
    m_dwNumBuffers   = dwNumBuffers;
    m_pWaveFile      = pWaveFile;
    m_dwCreationFlags = dwCreationFlags;
    
    FillBufferWithSound( m_apDSBuffer[0]);
}



DX9Sound::~DX9Sound()
{
    for( DWORD i=0; i<m_dwNumBuffers; i++ )
    {
        SAFE_RELEASE( m_apDSBuffer[i] ); 
    }

    SAFE_DELETE_ARRAY( m_apDSBuffer ); 
    SAFE_DELETE( m_pWaveFile );
}




HRESULT DX9Sound::FillBufferWithSound( LPDIRECTSOUNDBUFFER pDSB )
{
    HRESULT hr; 
    VOID*   pDSLockedBuffer      = NULL; // Pointer to locked buffer memory
    DWORD   dwDSLockedBufferSize = 0;    // Size of the locked DirectSound buffer
    DWORD   dwWavDataRead        = 0;    // Amount of data read from the wav file 

    // Make sure we have focus, and we didn't just switch in from
    // an app which had a DirectSound device
    RestoreBuffer( pDSB, NULL );

    // Lock the buffer down
    hr = pDSB->Lock( 0, m_dwDSBufferSize, 
                                 &pDSLockedBuffer, &dwDSLockedBufferSize, 
                                 NULL, NULL, 0L );

    // Reset the wave file to the beginning 
    m_pWaveFile->ResetFile();

    m_pWaveFile->Read( (BYTE*) pDSLockedBuffer,
                                        dwDSLockedBufferSize, 
                                        &dwWavDataRead );           
    
    if( dwWavDataRead == 0 )
    {
        // Wav is blank, so just fill with silence
        FillMemory( (BYTE*) pDSLockedBuffer, 
                    dwDSLockedBufferSize, 
                    (BYTE)(m_pWaveFile->m_pwfx->wBitsPerSample == 8 ? 128 : 0 ) );
    }
    else if( dwWavDataRead < dwDSLockedBufferSize )
    {
        // If the wav file was smaller than the DirectSound buffer, 
        // we need to fill the remainder of the buffer with data 

        // Don't repeat the wav file, just fill in silence 
        FillMemory( (BYTE*) pDSLockedBuffer + dwWavDataRead, 
                    dwDSLockedBufferSize - dwWavDataRead, 
                    (BYTE)(m_pWaveFile->m_pwfx->wBitsPerSample == 8 ? 128 : 0 ) );
    }

    // Unlock the buffer, we don't need it anymore.
    pDSB->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0 );

    return S_OK;
}




HRESULT DX9Sound::RestoreBuffer( LPDIRECTSOUNDBUFFER pDSB, BOOL* pbWasRestored )
{
    HRESULT hr;

    if( pDSB == NULL )
        return CO_E_NOTINITIALIZED;
    if( pbWasRestored )
        *pbWasRestored = FALSE;

    DWORD dwStatus;
    pDSB->GetStatus( &dwStatus );

    if( dwStatus & DSBSTATUS_BUFFERLOST )
    {
        // Since the app could have just been activated, then
        // DirectSound may not be giving us control yet, so 
        // the restoring the buffer may fail.  
        // If it does, sleep until DirectSound gives us control.
        do 
        {
            hr = pDSB->Restore();
            if( hr == DSERR_BUFFERLOST )
                Sleep( 10 );
        }
        while( ( hr = pDSB->Restore() ) == DSERR_BUFFERLOST );

        if( pbWasRestored != NULL )
            *pbWasRestored = TRUE;

        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}




LPDIRECTSOUNDBUFFER DX9Sound::GetFreeBuffer()
{

    for( DWORD i=0; i<m_dwNumBuffers; i++ )
    {
        if( m_apDSBuffer[i] )
        {  
            DWORD dwStatus = 0;
            m_apDSBuffer[i]->GetStatus( &dwStatus );
            if ( ( dwStatus & DSBSTATUS_PLAYING ) == 0 )
                break;
        }
    }

    if( i != m_dwNumBuffers )
        return m_apDSBuffer[ i ];
    else
        return m_apDSBuffer[ rand() % m_dwNumBuffers ];
}



LPDIRECTSOUNDBUFFER DX9Sound::GetBuffer( DWORD dwIndex )
{
    if( dwIndex >= m_dwNumBuffers )
        return NULL;

    return m_apDSBuffer[dwIndex];
}




HRESULT DX9Sound::Get3DBufferInterface( DWORD dwIndex, LPDIRECTSOUND3DBUFFER* ppDS3DBuffer )
{
    if( dwIndex >= m_dwNumBuffers )
        return E_INVALIDARG;

    *ppDS3DBuffer = NULL;

    return m_apDSBuffer[dwIndex]->QueryInterface( IID_IDirectSound3DBuffer, 
                                                  (VOID**)ppDS3DBuffer );
}


HRESULT DX9Sound::Play( DWORD dwPriority, DWORD dwFlags, LONG lVolume )
{
    HRESULT hr;
    BOOL    bRestored;

    LPDIRECTSOUNDBUFFER pDSB = GetFreeBuffer();

    if( pDSB == NULL )
        return DXTRACE_ERR( TEXT("GetFreeBuffer"), E_FAIL );

    // Restore the buffer if it was lost
    if( FAILED( hr = RestoreBuffer( pDSB, &bRestored ) ) )
        return DXTRACE_ERR( TEXT("RestoreBuffer"), hr );

    if( bRestored )
    {
        // The buffer was restored, so we need to fill it with new data
        if( FAILED( hr = FillBufferWithSound( pDSB ) ) )
            return DXTRACE_ERR( TEXT("FillBufferWithSound"), hr );
    }

    if( m_dwCreationFlags & DSBCAPS_CTRLVOLUME )
    {
        pDSB->SetVolume( lVolume );
    }
    
    return pDSB->Play( 0, dwPriority, dwFlags );
}




HRESULT DX9Sound::Stop()
{
    HRESULT hr = 0;

    for( DWORD i=0; i<m_dwNumBuffers; i++ )
        hr |= m_apDSBuffer[i]->Stop();

    return hr;
}




HRESULT DX9Sound::Reset() 
{
    HRESULT hr = 0;

    for( DWORD i=0; i<m_dwNumBuffers; i++ )
        hr |= m_apDSBuffer[i]->SetCurrentPosition( 0 );

    return hr;
}




BOOL DX9Sound::IsSoundPlaying() 
{
    BOOL bIsPlaying = FALSE;

    for( DWORD i=0; i<m_dwNumBuffers; i++ )
    {
        if( m_apDSBuffer[i] )
        {  
            DWORD dwStatus = 0;
            m_apDSBuffer[i]->GetStatus( &dwStatus );
            bIsPlaying |= ( ( dwStatus & DSBSTATUS_PLAYING ) != 0 );
        }
    }

    return bIsPlaying;
}




void DX9Sound::SetBufferVolume(long volume) 
{
    LPDIRECTSOUNDBUFFER pDSB = GetFreeBuffer();

    if( m_dwCreationFlags & DSBCAPS_CTRLVOLUME )
    {
        pDSB->SetVolume( volume );
    }

	pDSB->Play( 0, 0, DSBPLAY_LOOPING );

	return;
}

long DX9Sound::GetBufferVolume()
{
	return m_fvolume;
}


//********************************************************
//
// DX9 Wavefile Class
//
//********************************************************
DX9WaveFile::DX9WaveFile()
{
    m_pwfx    = NULL;
    m_hmmio   = NULL;
    m_pResourceBuffer = NULL;
    m_dwSize  = 0;
    m_bIsReadingFromMemory = FALSE;
}




DX9WaveFile::~DX9WaveFile()
{
    Close();
}




HRESULT DX9WaveFile::Open( LPTSTR strFileName, WAVEFORMATEX* pwfx, DWORD dwFlags )
{
    MMCKINFO        mmciFormat;           // chunk info. for general use.
    PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       


    m_dwFlags = dwFlags;
    m_bIsReadingFromMemory = FALSE;

    if( strFileName == NULL )
        return E_INVALIDARG;
    SAFE_DELETE_ARRAY( m_pwfx );

	//ICK! API STINKS!
    m_hmmio = mmioOpen( strFileName, NULL, MMIO_ALLOCBUF | MMIO_READ );
	m_ckRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
	mmioDescend( m_hmmio, &m_ckRiff, NULL, 0 );
    mmciFormat.ckid = mmioFOURCC('f', 'm', 't', ' ');
    mmioDescend( m_hmmio, &mmciFormat, &m_ckRiff, MMIO_FINDCHUNK );
	mmioRead( m_hmmio, (HPSTR) &pcmWaveFormat, sizeof(pcmWaveFormat));
    m_pwfx = (WAVEFORMATEX*)new CHAR[ sizeof(WAVEFORMATEX) ];
    memcpy( m_pwfx, &pcmWaveFormat, sizeof(pcmWaveFormat) );
    m_pwfx->cbSize = 0;

	ResetFile();

    // After the reset, the size of the wav file is m_ck.cksize so store it now
    m_dwSize = m_ck.cksize;

    return S_OK;
}



DWORD DX9WaveFile::GetSize()
{
    return m_dwSize;
}



HRESULT DX9WaveFile::ResetFile()
{
    // Reset the pointer to the beginning of the file
    mmioSeek( m_hmmio, m_ckRiff.dwDataOffset + sizeof(FOURCC), SEEK_SET );
    m_ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
    mmioDescend( m_hmmio, &m_ck, &m_ckRiff, MMIO_FINDCHUNK );

    return S_OK;
}




HRESULT DX9WaveFile::Read( BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead )
{

    MMIOINFO mmioinfoIn; // current status of m_hmmio

    if( pdwSizeRead != NULL )
        *pdwSizeRead = 0;

    if( 0 != mmioGetInfo( m_hmmio, &mmioinfoIn, 0 ) )
        return DXTRACE_ERR( TEXT("mmioGetInfo"), E_FAIL );
            
    UINT cbDataIn = dwSizeToRead;
    if( cbDataIn > m_ck.cksize ) 
        cbDataIn = m_ck.cksize;       

    m_ck.cksize -= cbDataIn;

    for( DWORD cT = 0; cT < cbDataIn; cT++ )
    {
        // Copy the bytes from the io to the buffer.
        if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
        {
            if( 0 != mmioAdvance( m_hmmio, &mmioinfoIn, MMIO_READ ) )
                return DXTRACE_ERR( TEXT("mmioAdvance"), E_FAIL );

            if( mmioinfoIn.pchNext == mmioinfoIn.pchEndRead )
                return DXTRACE_ERR( TEXT("mmioinfoIn.pchNext"), E_FAIL );
        }

        // Actual copy.
        *((BYTE*)pBuffer+cT) = *((BYTE*)mmioinfoIn.pchNext);
        mmioinfoIn.pchNext++;
    }

    if( 0 != mmioSetInfo( m_hmmio, &mmioinfoIn, 0 ) )
        return DXTRACE_ERR( TEXT("mmioSetInfo"), E_FAIL );

    if( pdwSizeRead != NULL )
        *pdwSizeRead = cbDataIn;

    return S_OK;
}




HRESULT DX9WaveFile::Close()
{
    mmioClose( m_hmmio, 0 );
    m_hmmio = NULL;
    SAFE_DELETE_ARRAY( m_pResourceBuffer );

    return S_OK;
}








HWND GetConsoleHwnd(void) {

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



void main(int argc, char **argv) {

	HWND					hWin;
	vrpn_Sound_Server_DX9*	soundServer = NULL;
	vrpn_Tracker_Remote*	tracker_connection;
	char                    tracker_device[512];
	char                    tracker_name[512];
	vrpn_Connection*		connection;
	vrpn_Connection*		trackerCon;
	int                     got_report;

	char*	config_file_name = "vrpn.cfg";
	FILE*	config_file;
	char*	client_name   = NULL;
	int	    client_port   = 4501;
	int	    bail_on_error = 1;
	int	    verbose       = 0;
	int	    auto_quit     = 0;
	int	    realparams    = 0;
	int 	loop          = 0;
	int	    port          = vrpn_DEFAULT_LISTEN_PORT_NO;

	int		USE_TRACKER	  = 0;

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
		char*	pch;
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
			strncpy(scrap, line, sizeof(line) - 1);
			// Figure out the device from the name and handle appropriately

			// WARNING: SUBSTRINGS WILL MATCH THE EARLIER STRING, SO 
			// ADD AN EMPTY SPACE TO THE END OF STATIC STRINGS!!!!

			// #define isit(s) !strncmp(line,s,strlen(s))

#define isit(s) !strcmp(pch=strtok(scrap," \t"),s)
#define next() pch += strlen(pch) + 1

#ifdef _WIN32

			if(isit("vrpn_Sound_Server"))
			{
				next();

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

				printf("\nBegin initializing DX9 Sound Server...\n");	
				
				soundServer = NULL;
				soundServer = new vrpn_Sound_Server_DX9(s2, connection,hWin);
				
				if (soundServer == NULL) 
					printf("Can't create sound server\n");
				
				printf("DX9 Sound Server successfully initialized.\n\n");

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
	else
		if(verbose) printf("Not using tracker\n");

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

	// ********************************************************************
	//
	//					MAIN LOOP
	//
	// ********************************************************************

	float fPrevTime = 0.0f;
	float fFrameTime;
	float fTime;
	int counter = 0;
	int stopNow = 0;
	int numconnections = 0;

	printf("Entering main loop\n\n");

	while (!(_kbhit() && ((_getch()=='q') || (_getch()=='Q')))) {

		//soundServer->GetLastError(buf);
		soundServer->mainloop();
		connection->mainloop();

		if(counter % 100 == 0) 
			vrpn_SleepMsecs(1);

		counter++;

		// record time since last frame 
		if (counter==NUM_SPIN) {
			
			counter = 0;
			fTime = (float)timeGetTime();
			
			fFrameTime = (fTime - fPrevTime) * 0.001f;
			printf("Running at %4.2f Hz\r", (float) NUM_SPIN/fFrameTime); 
			fPrevTime = fTime;
		}

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

		if (numconnections==0 && connection->connected())
			numconnections++;

		if (((numconnections!=0) && (!connection->connected())) || !connection->doing_okay())  {
			soundServer->shutDown();
			numconnections=0;
			break;
		}

	}

	printf("DirectX Sound Server shutting down.\n");
}

#endif
