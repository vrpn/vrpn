/*To Do list:
	1. Figure out how to get up vector info from quaternion pose to set listener up vector.  
		Setting the Sound up vectors would be nice, but isn't as important as sounds are 
		symmetric around their orientation vector.
	2. Sample level environment preferences (ie, EAX_ENVIRONMENT_BATHROOM)
		could be added in loadSound as a passed-in parameter

 */


#ifdef _WIN32

#include "vrpn_Sound_Miles.h"
#include <math.h>
#include <iostream.h>

/********************************************************************************************
 Begin vrpn_Sound_Server_Miles
 *******************************************************************************************/
vrpn_Sound_Server_Miles::vrpn_Sound_Server_Miles(const char * name, vrpn_Connection * c)
				  : vrpn_Sound_Server(name, c)
{
	AIL_startup();							//initialize the Miles SDK stuff

	// load a digital driver
	if (!AIL_quick_startup(1,0,44100,16,2)) {
     MessageBox(0,"Couldn't open a digital output device.","Error",MB_OK);
    }
    AIL_quick_handles(&DIG,0,0);

	// No sound played yet
	LastSoundId = -1;

	provider = 0;
	//Set variables to control size and growing of Audio handle array
	H_Max = vrpn_Sound_START;
	H_Cur = 0;

	//Allocate space for Audio Handle array
	samples = new H3DSAMPLE[H_Max];

	//Set variables to control size and growing of providers array
	P_Max = vrpn_Sound_START;
	P_Cur = 0;

	//Allocate space for Audio Handle array
	providers = new HPROVIDER[P_Max];
}

vrpn_Sound_Server_Miles::~vrpn_Sound_Server_Miles()		
{
	shutDown();
	delete [] samples;
}

void vrpn_Sound_Server_Miles::shutDown()
{
	unloadAllSounds();
	AIL_close_3D_provider(provider);

	/*These two calls must be made before trying to exit or the 
	program will hang when you try to quit.*/
	AIL_waveOutClose(DIG);
	AIL_shutdown();
}

void vrpn_Sound_Server_Miles::addProvider(HPROVIDER p)
{
	if (P_Cur >= P_Max)
	{
		int i;
		HPROVIDER *oldproviders;

		oldproviders = providers;

		P_Max *= 2;
		providers = new HPROVIDER[P_Max];

		for(i = 0; i < P_Cur; i++)
			providers[i] = oldproviders[i];

		delete [] oldproviders;
	}

	providers[P_Cur++] = p;
}

bool vrpn_Sound_Server_Miles::noSounds()
{
	int i;

	for(i = 0; i < H_Cur; i++)
		if (samples[i] != NULL)
			return false;
		
	return true;
}

void vrpn_Sound_Server_Miles::setProvider(int index, int providerRoomSetting)
{
  DWORD result;

  unloadAllSounds();
  //If a provider is already open, then close it
  AIL_close_3D_provider(provider);

  //load the new provider
  result = AIL_open_3D_provider(providers[index]);
  if (result != M3D_NOERR)
     cerr << "Error Opening 3D Provider" << endl;     

  provider = providers[index];
  //open a listener.  Position defaults to the origin, looking down the -Z axis with (
  listener = AIL_open_3D_listener(provider);
  AIL_set_3D_position(&listener, 0, 0, 0);
  AIL_set_3D_orientation(&listener, 0, 0, -1, 0, 1, 0);
  AIL_set_3D_velocity(&listener, 0, 0, 0, 0);

  //set a room style
  AIL_set_3D_provider_preference(provider, "EAX environment selection", &providerRoomSetting);
}

void vrpn_Sound_Server_Miles::unloadHandle(vrpn_SoundID id)
//unloads a handle corresponding to id from samples
{
	vrpn_int32 SIndex;

	SIndex = map_CIndex_To_SIndex(id);
	if (SIndex != vrpn_Sound_FAIL)
	{
		samples[SIndex] = NULL;
		set_CIndex_To_SIndex(id, vrpn_Sound_FAIL);
	}	
}

void vrpn_Sound_Server_Miles::chooseProvider(char *ThreeDProvider)
/*chooses a 3D audio provider.  
See the comment below the constructor in the .h for more info.*/

//a better way really should be implemented to choose a provider
{	
   char* name;  
   HPROENUM next = HPROENUM_FIRST;
   while (AIL_enumerate_3D_providers(&next, &provider, &name))
      {	    
		if(strcmp(name, ThreeDProvider) == 0)
			break;
      }
}

void vrpn_Sound_Server_Miles::unloadAllSounds()
//clears out samples
{
	for(vrpn_int32 i = 0; i < H_Cur; i++)
	{
		if (samples[i] == NULL)
			continue;
		S32 status = AIL_3D_sample_status(samples[i]);
		if(status == SMP_DONE || status == SMP_STOPPED)
			//if handle is loaded and not playing, unload
		{
			AIL_release_3D_sample_handle(samples[i]);
		}
		if(status == SMP_PLAYING)
			//if handle is playing, stop then unload
		{
			AIL_stop_3D_sample(samples[i]);
			AIL_release_3D_sample_handle(samples[i]);
		}

	}
	H_Cur = 0;
}

void vrpn_Sound_Server_Miles::addSample(H3DSAMPLE sample, vrpn_SoundID id)
//adds sample to the location in samples corresponding to id
{
	if (H_Cur >= H_Max)
	{
		int i;
		H3DSAMPLE *oldsamples;

		oldsamples = samples;

		H_Max *= 2;
		samples = new H3DSAMPLE[H_Max];

		for(i = 0; i < H_Cur; i++)
			samples[i] = oldsamples[i];

		delete [] oldsamples;
	}

	if (samples != NULL)
	{
		samples[H_Cur] = sample;
		set_CIndex_To_SIndex(id, H_Cur);
		H_Cur++;		
	}
	else
		set_CIndex_To_SIndex(id, vrpn_Sound_FAIL);

}

H3DSAMPLE vrpn_Sound_Server_Miles::getSample(vrpn_SoundID id)
//gets the handle stored in samples[id]
{
	vrpn_int32 SIndex;

	SIndex = map_CIndex_To_SIndex(id);

	if (SIndex == vrpn_Sound_FAIL)
		return NULL;
	else
		return samples[SIndex];
}

void vrpn_Sound_Server_Miles::loadSound(char* filename, vrpn_SoundID id)
/*loads a .wav file into memory.  gives back id to reference the sound
currently (7/28/99) this does not reuse space previously vacated in 
samples by unloading sounds*/
{
    fprintf(stdout,"Loading sound: %s\n", filename);
	//load into handle
	unsigned long *s;
	long type;
	H3DSAMPLE handle;
	void *sample_address;
	void *d;
	AILSOUNDINFO info;

	LastSoundId = id;

	//load the .wav file into memory
	s = (unsigned long *)AIL_file_read(filename, FILE_READ_WITH_SIZE);

	if (s==0)
       return;

	type=AIL_file_type(s+1,s[0]);

	switch (type) {
     case AILFILETYPE_PCM_WAV:
       sample_address = s+1;
	   break;

     case AILFILETYPE_ADPCM_WAV:
       AIL_WAV_info(s+1,&info);
       AIL_decompress_ADPCM(&info,&d,0);
       AIL_mem_free_lock(s);
	   sample_address = d;
       break;

     default:
       AIL_mem_free_lock(s);
       return;
   }

	//initialize handle
    handle = AIL_allocate_3D_sample_handle(provider);
	//tell handle where the .wav file is in memory
	AIL_set_3D_sample_file(handle, sample_address);
	//set defaults
	AIL_set_3D_sample_volume(handle, 100);	//default volume is 100 on 0...127 inclusive
	AIL_set_3D_position(handle,0,0,0);
	AIL_set_3D_orientation(handle, 0,0,-1,0,1,0);
	AIL_set_3D_velocity(handle,0,0,-1,0);
	AIL_set_3D_sample_distances(handle, 200, 20, 200, 20);

	//load handle into samples
	addSample(handle, id);

	/*Sample level environment preferences (ie, EAX_ENVIRONMENT_BATHROOM)
	could be added in here as a passed-in parameter*/
}

void vrpn_Sound_Server_Miles::changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef)
{
	vrpn_float32 uX = 0, uY = 1, uZ = 0;
	/*(uX,uY,uZ) describes up vector
	note: orientation vector and up vector should be orthogonal*/
	
	/*For sounds, the up vector is trivial under Miles as they are simulated 
	as either conical or spherical projections.  Therefore it can just be 
	set to be perpenticular to the orientation vector.*/
	
	//get pose info from quaternion	in soundDef, put it into the above vectors
	AIL_set_3D_position(getSample(id),soundDef.pose.position[0] , soundDef.pose.position[1], soundDef.pose.position[2]);
	AIL_set_3D_orientation(getSample(id), soundDef.pose.orientation[0], soundDef.pose.orientation[1], soundDef.pose.orientation[2], uX, uY, uZ);

	AIL_set_3D_velocity(getSample(id), soundDef.velocity[0], soundDef.velocity[1], soundDef.velocity[2], soundDef.velocity[3]);

	/*set the volume to the new level.*/
	AIL_set_3D_sample_volume(getSample(id), soundDef.volume);
}

void vrpn_Sound_Server_Miles::playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef)
{
	// update last sound played record
	LastSoundId = id;

	changeSoundStatus(id, soundDef);	
	AIL_set_3D_sample_loop_count(getSample(id), repeat);

	//if not playing, play
	//if playing already modify parameters in p for pose, 
	//vel, etc but ignore loop count

	switch(AIL_3D_sample_status(getSample(id)))
	{
		case SMP_DONE:
		case SMP_STOPPED:
		{
			AIL_start_3D_sample(getSample(id));
			break;
		}
		case SMP_PLAYING:
		{
			//it's already playing.  
			//new repeat info is ignored.  new pose and velocity info was set above
			break;
		}
		default:
		{
			/*It shouldn't occur that one of the above cases isn't triggered.  If error
			messages back to the client are ever implemented, one should be keyed here*/
			break;
		}
	}
}
	

void vrpn_Sound_Server_Miles::stopSound(vrpn_SoundID id)
/*immediately stops playing the sound refered to by id
the sound can be restarted*/
{
	AIL_stop_3D_sample(getSample(id));
}

void vrpn_Sound_Server_Miles::stopAllSounds()
{
	int i;

	for(i = 0; i < H_Cur; i++)
		AIL_stop_3D_sample(getSample(i));
}

void vrpn_Sound_Server_Miles::unloadSound(vrpn_SoundID id)
/*unloads id from samples.  
Sets the corresponding cell in samples to FAIL. */
{
	AIL_release_3D_sample_handle(getSample(id));
	unloadHandle(id);
}

void vrpn_Sound_Server_Miles::changeListenerStatus(vrpn_ListenerDef listenerDef)
//change pose, etc for listener
{
	vrpn_float32 uX = 0, uY = 1, uZ = 0;

	AIL_set_3D_position(&listener, listenerDef.pose.position[0], listenerDef.pose.position[1], listenerDef.pose.position[2]);
	AIL_set_3D_orientation(&listener, listenerDef.pose.orientation[0], listenerDef.pose.orientation[1], listenerDef.pose.orientation[2], uX, uY, uZ);
	AIL_set_3D_velocity(&listener, listenerDef.velocity[0], listenerDef.velocity[1], listenerDef.velocity[2], listenerDef.velocity[3]);
}

void vrpn_Sound_Server_Miles::mainloop(const timeval *timeout)
{
	if (!connection->connected())
	{
		unloadAllSounds();
		unloadCSMap();
	}
	connection->mainloop(timeout);
}

vrpn_int32 vrpn_Sound_Server_Miles::GetCurrentVolume() {
	if (LastSoundId != -1) {
		AIL_set_3D_sample_volume(getSample(LastSoundId),100);

		return (vrpn_int32) AIL_3D_sample_volume(getSample(LastSoundId)); }
	else return -1;
 }

char * vrpn_Sound_Server_Miles::GetLastError() {
	return AIL_last_error();
}

vrpn_int32 vrpn_Sound_Server_Miles::GetCurrentPlaybackRate() {
	if (LastSoundId != -1)
		return (vrpn_int32) AIL_3D_sample_playback_rate(getSample(LastSoundId));
 	else return -1;
}

void vrpn_Sound_Server_Miles::GetCurrentPosition(F32* X, F32* Y, F32* Z) {
	if (LastSoundId != -1) 
		AIL_3D_position(getSample(LastSoundId),X,Y,Z);
    else {
		*X = -1;
		*Y = -1;
		*Z = -1;
	}
return;
}


#endif
