//	vrpn_Sound_ASM.cpp (AuSIM)
//
//	July 2003- Matt McCallus

/*  TODO:
	find out about sound parameters and listener parameters
	error checking on cre functions.

*/

#include "vrpn_Sound_ASM.h"

#define NUM_SPIN 10000
#define MAX_NUM_SOUNDS 16
const int CHAR_BUF_SIZE = 1024;


// Some globals for mapping sounds
int						g_numSounds				= 0;
SoundMap				g_soundMap[MAX_NUM_SOUNDS];	
Sound					g_Sound[MAX_NUM_SOUNDS];		// wrapper for ASMSound obj
Listener*				g_pListener;

		

vrpn_Sound_Server_ASM::vrpn_Sound_Server_ASM(const char      * name, 
											 vrpn_Connection * c ) 
											 : vrpn_Sound_Server(name, c){

	// initialize AuSIM
	if(cre_init(Atrn_ASM1, 0, MAX_NUM_SOUNDS, _CONSOLE_|_VERBOSE_) < Ok) {
		
		printf("AudioInit().\n");
		send_message("Error initializing AuSIM server.. sound server did not start.",vrpn_TEXT_ERROR,0);
		exit(0);
	}

	// set up the sound map.
	g_numSounds = 0;

	for(int i=0; i<MAX_NUM_SOUNDS; i++){
		g_soundMap[i].m_iSoundNum = -1;
		for(int j=0; j<80; j++)
			g_soundMap[i].m_ASMSoundName[j] = '\0';
	}
}



vrpn_Sound_Server_ASM::~vrpn_Sound_Server_ASM() {

	// TODO: figure out what to release/delete
}


void vrpn_Sound_Server_ASM::playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef) {

	if(id<0 || id>g_numSounds){
		send_message("Error: playSound (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	// get sound id from the sound map
	int myid = g_soundMap[id].m_iSoundNum;

	g_Sound[myid].repeat = repeat;

	if (myid != -1) {
		
		if(g_Sound[id].m_pSound != NULL){

			if (repeat == 0)	// loop continuously
				g_Sound[myid].m_pSound->Play(myid, 0 , g_Sound[myid].m_pSound->m_fVolume );
			else {				// decrement the repeat count
				g_Sound[myid].m_pSound->Play(myid, g_Sound[myid].repeat, g_Sound[myid].m_pSound->m_fVolume );
				g_Sound[myid].repeat--;
				
			}
    			
			send_message("Playing sound\n",vrpn_TEXT_NORMAL,0);
		}
		else send_message("Error: playSound (Sound not loaded)",vrpn_TEXT_WARNING,0);

	}
	else send_message("Error: playSound (Sound not loaded)",vrpn_TEXT_WARNING,0);
}



void vrpn_Sound_Server_ASM::stopSound(vrpn_SoundID id) {

	if(id<0 || id>g_numSounds){
		send_message("Error: stopSound (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	int myid = g_soundMap[id].m_iSoundNum;

	if ( myid != -1 && g_Sound[myid].m_pSound != NULL ) {
		g_Sound[myid].m_pSound->Stop(myid);
		g_Sound[myid].m_pSound->Reset(myid);
		send_message("Stopping sound",vrpn_TEXT_NORMAL,0);
	} 
	else{
		send_message("Invalid sound id",vrpn_TEXT_ERROR,0);
		printf("Invalid sound id\n");
	}
}



void vrpn_Sound_Server_ASM::loadSoundLocal(char* filename, vrpn_SoundID id, vrpn_SoundDef soundDef) {

	char tempbuf[1024];	

    // Free any previous sound, and make a new one
	// SAFE_DELETE( g_Sound[id].m_pSound );

    wavFt* waveFile;

	if( !(waveFile = cre_open_wave(filename, NULL))) {

		printf("Error opening waveFile: %s\n",waveFile);
		send_message("Error opening waveFile",vrpn_TEXT_ERROR,0);
		return;
	}

	g_Sound[id].m_pSound = new ASMSound(waveFile);

	// Load the sound id and name(path) into the sound map
	g_soundMap[g_numSounds].m_iSoundNum = (int)id;

	char* nameptr = g_soundMap[g_numSounds].m_ASMSoundName;
	strcpy(nameptr, filename);
	int myid = g_soundMap[g_numSounds].m_iSoundNum;

	g_numSounds++;
	
	// Set up sound parameters
	float loc[6];

	loc[0] = (float)soundDef.pose.position[0];
	loc[1] = (float)soundDef.pose.position[1];
	loc[2] = (float)soundDef.pose.position[2];

	// convert quat to euler
	q_vec_type tempeuler;
	q_type tempquat;

	tempquat[0] = soundDef.pose.orientation[0];
	tempquat[1] = soundDef.pose.orientation[1];
	tempquat[2] = soundDef.pose.orientation[2];
	tempquat[3] = soundDef.pose.orientation[3];

	// 0: yaw
	// 1: pitch
	// 2: roll
	q_to_euler(tempeuler, tempquat);
	
	loc[3] = -(float)tempeuler[0];
	loc[4] = -(float)tempeuler[1];
	loc[5] = -(float)tempeuler[2];

	cre_locate_source( (int)id, loc );
	cre_update_audio();
	
	sprintf(tempbuf,"Loading sound #%d: %s                    \n",id,filename);
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);

	GetSoundList();

	return;

}



// not supported
void vrpn_Sound_Server_ASM::loadSoundRemote(char* filename, vrpn_SoundID id, vrpn_SoundDef soundDef) {

	send_message("loadSoundRemote not supported",vrpn_TEXT_WARNING,0);
}




void vrpn_Sound_Server_ASM::unloadSound(vrpn_SoundID id) {

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
		
		if(g_Sound[i].m_pSound->IsSoundPlaying(i)){
			g_Sound[i].m_pSound->Stop(i);
		}
		char* tempptr1 = g_soundMap[i].m_ASMSoundName;
		strcpy( tempptr1, &g_soundMap[i+1].m_ASMSoundName[0] );
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



void vrpn_Sound_Server_ASM::setListenerPose(vrpn_PoseDef pose) {

	// Set up listener parameters
	float loc[6];

	loc[0] = (float)pose.position[0];
	loc[1] = (float)pose.position[1];
	loc[2] = (float)pose.position[2];

	// convert quat to euler
	q_vec_type tempeuler;
	q_type tempquat;

	tempquat[0] = pose.orientation[0];
	tempquat[1] = pose.orientation[1];
	tempquat[2] = pose.orientation[2];
	tempquat[3] = pose.orientation[3];

	// 0: yaw
	// 1: pitch
	// 2: roll
	q_to_euler(tempeuler, tempquat);

	
	loc[3] = -(float)tempeuler[0];
	loc[4] = -(float)tempeuler[1];
	loc[5] = -(float)tempeuler[2];

	cre_locate_head( 0, loc );
	cre_update_audio();

	return;
}




void vrpn_Sound_Server_ASM::setListenerVelocity(vrpn_float64 velocity[4]) {

	send_message("setListenerVelocity not supported",vrpn_TEXT_WARNING,0);

	return;
}




void vrpn_Sound_Server_ASM::changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef) {

	if(id<0 || id>g_numSounds){
		send_message("Error: changeSoundStatus (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	vrpn_int32     myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {

		sprintf(tempbuf,"Error: changeSoundStatus(Invalid id)          ");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

	// Set sound parameters
	float loc[6];

	loc[0] = (float)soundDef.pose.position[0];
	loc[1] = (float)soundDef.pose.position[1];
	loc[2] = (float)soundDef.pose.position[2];

	// convert quat to euler
	q_vec_type tempeuler;
	q_type tempquat;

	tempquat[0] = soundDef.pose.orientation[0];
	tempquat[1] = soundDef.pose.orientation[1];
	tempquat[2] = soundDef.pose.orientation[2];
	tempquat[3] = soundDef.pose.orientation[3];

	// 0: yaw
	// 1: pitch
	// 2: roll
	q_to_euler(tempeuler, tempquat);
	
	loc[3] = -(float)tempeuler[0];
	loc[4] = -(float)tempeuler[1];
	loc[5] = -(float)tempeuler[2];

	cre_locate_source( (int)id, loc );
	cre_update_audio();

	return;
}




void vrpn_Sound_Server_ASM::setSoundPose(vrpn_SoundID id, vrpn_PoseDef pose) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundPose (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	int myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundPose(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

	// Set sound parameters
	float loc[6];

	loc[0] = (float)pose.position[0];
	loc[1] = (float)pose.position[1];
	loc[2] = (float)pose.position[2];

	// convert quat to euler
	q_vec_type tempeuler;
	q_type tempquat;

	tempquat[0] = pose.orientation[0];
	tempquat[1] = pose.orientation[1];
	tempquat[2] = pose.orientation[2];
	tempquat[3] = pose.orientation[3];

	// 0: yaw
	// 1: pitch
	// 2: roll
	q_to_euler(tempeuler, tempquat);
	
	loc[3] = -(float)tempeuler[0];
	loc[4] = -(float)tempeuler[1];
	loc[5] = -(float)tempeuler[2];

	cre_locate_source( (int)id, loc );
	cre_update_audio();

	return;
}




void vrpn_Sound_Server_ASM::setSoundVelocity(vrpn_SoundID id, vrpn_float64 *velocity) {

	send_message("setSoundVelocity not supported",vrpn_TEXT_WARNING,0);

	return;

}



// AuSIM only sets minimum distance...
void vrpn_Sound_Server_ASM::setSoundDistInfo(vrpn_SoundID id, vrpn_float64 *distinfo) {

	// distinfo[0] is rear min 
	// distinfo[1] is rear max
	// distinfo[2] is front min (we use this)
	// distinfo[3] is front max

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundDistInfo (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	if(distinfo[2]<0){
		send_message("Error: setSoundDistInfo (Invalid distinfo[2])",vrpn_TEXT_WARNING,0);
		return;
	}

	int myid = (int)g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundDistInfo(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}
	

	void* pData = &distinfo[2];

	cre_define_source(myid, AtrnGAINdist, 1, pData );

	return;

}


// This differs from a DirectX implementation...
// Instead of taking in 3 floats specifying an orientation vector(coneinfo[0...2]),
// we are taking in 3 floats representing euler angles in radians.
void vrpn_Sound_Server_ASM::setSoundConeInfo(vrpn_SoundID id, vrpn_float64 *coneinfo) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundConeInfo (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	int myid = (int)g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundConeInfo(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

	float loc[6];

	// Set up sound parameters

	// we just get old position from sound struct.
    loc[0] = g_Sound[id].m_pSound->m_fPos[0];
	loc[1] = g_Sound[id].m_pSound->m_fPos[1];
	loc[2] = g_Sound[id].m_pSound->m_fPos[2];

	// set the orientation of the sound cone (euler).
    loc[3] = (float)coneinfo[0]; // yaw
	loc[4] = (float)coneinfo[1]; // pitch
	loc[5] = (float)coneinfo[2]; // roll

	// write pos and ori.
	cre_locate_source(myid, loc);

	/*	
		AuSIM doc for AtrnRADfields, their sound cone implementation.

		AtrnRADfields:
			Allows the user to control the directivity of the sound.
		data[] will contain two parameters (both in radians) describing a
		field of radiation and a field of intensity. These fields are cones
		centered on the source’s boresight direction (principal direction of
		aural emission) in which ~90% (for the field of radiation) or ~45%
		(for the field of intensity) of the sound energy is dissipated.
	*/

	//	NOTE: notice that coneinfo[5] (the outside cone gain) is not used.
	
	float angles[2];

	angles[0] = (float)coneinfo[3];
	angles[1] = (float)coneinfo[4];

	cre_define_source(myid, AtrnRADfields, 2, angles);

	return;
}



void vrpn_Sound_Server_ASM::setSoundDoplerFactor(vrpn_SoundID id, vrpn_float64 doplerfactor) {

	// Still check for sane user anyway.
	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundDoplerFactor (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	int myid = (int)g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundDoplerFactor(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}
	
	void* pData = &doplerfactor;

	cre_define_source(myid, AtrnDPLRfactor, 1, pData);

	return;
}



void vrpn_Sound_Server_ASM::setSoundEqValue(vrpn_SoundID id, vrpn_float64 eqvalue) {

	send_message("setSoundEqValue not supported",vrpn_TEXT_WARNING,0);

	return;
}




void vrpn_Sound_Server_ASM::setSoundPitch(vrpn_SoundID id, vrpn_float64 pitch) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundConeInfo (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	if(pitch<0.5 || pitch>2.0){
		send_message("Error: setSoundConeInfo (Invalid pitch)",vrpn_TEXT_WARNING,0);
		return;
	}

	int myid = (int)g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundConeInfo(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

	void* pData = &pitch;

	cre_ctrl_wave(myid, g_Sound[id].m_pSound->m_pWav, WaveCTRL_PTCH, pData);

	return;
}




void vrpn_Sound_Server_ASM::setSoundVolume(vrpn_SoundID id, vrpn_float64 volume) {

	if(id<0 || id>g_numSounds){
		send_message("Error: setSoundVolume (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	if(volume > 0){
		send_message("Error: setSoundVolume (Invalid volume)",vrpn_TEXT_WARNING,0);
		return;
	}

	int myid = g_soundMap[id].m_iSoundNum;
	char tempbuf[1024];

	if (myid==-1) {
		sprintf(tempbuf,"Error: setSoundVolume(Invalid id)\n");
		printf("%s", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return;
	}

	cre_amplfy_source(myid, (float)volume);
	
	return;
}




void vrpn_Sound_Server_ASM::loadModelLocal(const char * filename) {

	char tempbuf[1024];

	sprintf(tempbuf,"loadModelLocal not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;

}




void vrpn_Sound_Server_ASM::loadModelRemote(){

	char tempbuf[1024];

	sprintf(tempbuf,"loadModelRemote not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}	




void vrpn_Sound_Server_ASM::loadPolyQuad(vrpn_QuadDef * quad) {

	char tempbuf[1024];

	sprintf(tempbuf,"loadPolyQuad not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;}




void vrpn_Sound_Server_ASM::loadPolyTri(vrpn_TriDef * tri) {

	char tempbuf[1024];

	sprintf(tempbuf,"loadPolyTri not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;}




void vrpn_Sound_Server_ASM::loadMaterial(vrpn_MaterialDef * material, vrpn_int32 id) {

	char tempbuf[1024];

	sprintf(tempbuf,"loadMaterial not implemented\n");
	printf("%s", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;}




void vrpn_Sound_Server_ASM::setPolyQuadVertices(vrpn_float64 vertices[4][3], const vrpn_int32 id) {

	char tempbuf[1024];

	sprintf(tempbuf,"setPolyQuadVertices not implemented");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;}




void vrpn_Sound_Server_ASM::setPolyTriVertices(vrpn_float64 vertices[3][3], const vrpn_int32 id) {

	char tempbuf[1024];

	sprintf(tempbuf,"setPolyTriVertices not implemented");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;}




void vrpn_Sound_Server_ASM::setPolyOF(vrpn_float64 OF, vrpn_int32 tag) {

	char tempbuf[1024];

	sprintf(tempbuf,"setPolyOF not implemented");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;
}




void vrpn_Sound_Server_ASM::setPolyMaterial(const char * material, vrpn_int32 tag) {

	char tempbuf[1024];

	sprintf(tempbuf,"setPolyMaterial not implemented");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return;}







vrpn_float64  vrpn_Sound_Server_ASM::GetCurrentVolume(const vrpn_int32 id) {

	if(id<0 || id>g_numSounds){
		send_message("Error: GetCurrentVolume (Invalid id)",vrpn_TEXT_WARNING,0);
		return -1.0f;
	}

	char tempbuf[1024];

	int myid = g_soundMap[id].m_iSoundNum;

	if (myid==-1) {
		sprintf(tempbuf,"Error: GetCurrentVolume(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return -1.0f;
	}
	
	vrpn_float64 val = g_Sound[id].m_pSound->m_fVolume;

    return val;
}


vrpn_int32  vrpn_Sound_Server_ASM::GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId) {

	char tempbuf[1024];

	sprintf(tempbuf,"GetCurrentPlaybackRate not available.");
	printf("%s\n", tempbuf);
	send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

	return -1;
}



// This returns the position for a sound
void vrpn_Sound_Server_ASM::GetCurrentPosition(const vrpn_int32 id, float* X_val, float* Y_val, float* Z_val) {

	if(id<0 || id>g_numSounds){
		send_message("Error: GetCurrentPosition (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	char tempbuf[1024];
	int myid = g_soundMap[id].m_iSoundNum;

	if (myid==-1) {
		sprintf(tempbuf,"Error: GetCurrentPosition(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return ;
	}	

	*X_val = g_Sound[id].m_pSound->m_fPos[0];
	*Y_val = g_Sound[id].m_pSound->m_fPos[1];
	*Z_val = g_Sound[id].m_pSound->m_fPos[2];

	return;
}




void vrpn_Sound_Server_ASM::GetListenerPosition(float* X_val, float* Y_val, float* Z_val) {


	char tempbuf[1024];

	if(g_pListener == NULL){
		sprintf(tempbuf,"Error: GetListenerPosition(NULL Listener)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return ;
	}	

	*X_val = g_pListener->m_fPos[0];
	*Y_val = g_pListener->m_fPos[1];
	*Z_val = g_pListener->m_fPos[2];

	return;
}



void vrpn_Sound_Server_ASM::GetCurrentDistances(const vrpn_int32 id, float* FMin, float* FMax) {

	if(id<0 || id>g_numSounds){
		send_message("Error: GetCurrentDistances (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	char tempbuf[1024];
	int myid = g_soundMap[id].m_iSoundNum;

	if (myid==-1) {

		sprintf(tempbuf,"Error: GetCurrentDistances(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return ;
	}

    *FMin = g_Sound[myid].m_pSound->m_fMinDist;
	*FMax = 0; // not supported in AuSIM

	return;
}




void vrpn_Sound_Server_ASM::GetListenerOrientation(float* y_val, float *p_val, float *r_val) {

	char tempbuf[1024];

	if(g_pListener == NULL){
		sprintf(tempbuf,"Error: GetListenerOrientation(NULL Listener)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return ;
	}	

	*y_val = g_pListener->m_fOri[0];
	*p_val = g_pListener->m_fOri[1];
	*r_val = g_pListener->m_fOri[2];

	return;
}




void vrpn_Sound_Server_ASM::GetCurrentOrientation(const vrpn_int32 id,float *yaw, float *pitch, float *roll) {

	if(id<0 || id>g_numSounds){
		send_message("Error: GetCurrentOrientation (Invalid id)",vrpn_TEXT_WARNING,0);
		return;
	}

	char tempbuf[1024];
	int myid = g_soundMap[id].m_iSoundNum;

	if (myid==-1) {
		sprintf(tempbuf,"Error: GetCurrentOrientation(Invalid id)");
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_ERROR,0);

		return ;
	}

	*yaw   = g_Sound[myid].m_pSound->m_fOri[0];
	*pitch = g_Sound[myid].m_pSound->m_fOri[1];
	*roll  = g_Sound[myid].m_pSound->m_fOri[2];

	return;
}



void vrpn_Sound_Server_ASM::mainloop() {

	vrpn_Text_Sender::mainloop();
	vrpn_Sound::d_connection->mainloop();
	
	vrpn_SoundDef temp;
	temp.volume = 0.0f; // suppress VC6 initialization warning with this
	// check if we need to repeat any sounds
	for(int i=0; i<g_numSounds; i++){
		if(g_Sound[i].repeat != 0) {
			if(!g_Sound[i].m_pSound->IsSoundPlaying(i)){
				playSound(g_soundMap[i].m_iSoundNum, g_Sound[i].repeat, temp);
			}
		}
	}
}



bool vrpn_Sound_Server_ASM::noSounds(void) {return 0;}



void vrpn_Sound_Server_ASM::stopAllSounds() {

	for (int i(0); i<g_numSounds;i++)
		g_Sound[i].m_pSound->Stop(i);

	return;
}




// re-initialize all sounds and materials etc
void vrpn_Sound_Server_ASM::shutDown() {

	// reinitialize playlist
	for (int i=0; i<MAX_NUM_SOUNDS; i++) {

		// Stop all playing sounds
		if(g_Sound[i].m_pSound != NULL){
			if(g_Sound[i].m_pSound->IsSoundPlaying(i))
				g_Sound[i].m_pSound->Stop(i);
		}
		
		g_Sound[i].m_pSound = NULL;
		g_soundMap[i].m_iSoundNum=-1;
	}

	g_numSounds = 0;
}



void vrpn_Sound_Server_ASM::GetSoundList() {

	char tempbuf[1024];

	printf("\nCurrent sounds loaded***********\n");

	for(int i=0; i<g_numSounds; i++){

		sprintf(tempbuf,"Sound# %d: %s",i,g_soundMap[i].m_ASMSoundName);
		printf("%s\n", tempbuf);
		send_message((const char *) tempbuf,vrpn_TEXT_NORMAL,0);
	}

	printf("********************************\n");

	return;
}



//********************************************************
//
// ASM Sound Class
//
//********************************************************
ASMSound::ASMSound( wavFt* pWave )
{
	m_fVolume = 0;
	m_pWav = pWave;

}



ASMSound::~ASMSound()
{
    SAFE_DELETE( m_pWav );
}



void ASMSound::Play(int id, int repeat, float volume )
{
	if(repeat == 0)
		cre_ctrl_wave(id, m_pWav, WaveCTRL_LOOP, NULL);
	else
		cre_ctrl_wave(id, m_pWav, WaveCTRL_PLAY, NULL);
	
	return;
}




void ASMSound::Stop(int id)
{
	cre_ctrl_wave(id, m_pWav, WaveCTRL_STOP, NULL);

	return;
}




void ASMSound::Reset(int id)
{
    cre_ctrl_wave(id, m_pWav, WaveCTRL_RWND, NULL);

	return;
}




BOOL ASMSound::IsSoundPlaying(int id)
{
    BOOL bIsPlaying = false;

	int val = cre_ctrl_wave(id, m_pWav, WaveCTRL_STAT, NULL);

	if(val != 0)
		bIsPlaying = true;

    return bIsPlaying;
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
	vrpn_Sound_Server_ASM*	soundServer = NULL;
//	vrpn_Tracker_Remote*	tracker_connection;
	char                    tracker_device[512];
	char                    tracker_name[512];
	vrpn_Connection*		connection;
//	vrpn_Connection*		trackerCon;
//	int                     got_report;

	char*	config_file_name = "vrpn.cfg";
	FILE*	config_file;
	char*	client_name   = NULL;
	int	    client_port   = 4502;
	int	    bail_on_error = 1;
	int	    verbose       = 0;
	int	    auto_quit     = 0;
	int	    realparams    = 0;
	int 	loop          = 0;
	int	    port          = vrpn_DEFAULT_LISTEN_PORT_NO;

//#define USING_TRACKER
	int USE_TRACKER = 0;

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

			//	  #define isit(s) !strncmp(line,s,strlen(s))

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

				printf("\nBegin initializing ASM Sound Server...\n");
				

				soundServer = NULL;
				soundServer = new vrpn_Sound_Server_ASM(s2, connection);
				
				if (soundServer == NULL) 
					printf("Can't create sound server\n");
				
				printf("ASM Sound Server successfully initialized.\n\n");

			}
#endif
		}
	}

	fclose(config_file);

	// Open remote tracker if we are to use one

#ifdef USING_TRACKER

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
#else
		if(verbose) printf("Not using tracker\n");
#endif

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
	// **          *  *               *   *                        *   * **
	// **     *           MAIN LOOP             *             *          **
	// ** *                 *   *                    *   *               **
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
			//printf("Running at %4.2f Hz\r", (float) NUM_SPIN/fFrameTime); 
			fPrevTime = fTime;
		}

		soundServer->mainloop();

		// ensure we get a new report!
#ifdef USING_TRACKER
		if (USE_TRACKER) {
			tracker_connection->mainloop();
			got_report = 0;
			if (trackerCon->doing_okay())
				while (!got_report) 
					tracker_connection->mainloop(); 
		}
#endif

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

	printf("Sound Server shutting down.\n");
}

