#ifdef _WIN32

#include "vrpn_Sound_Miles.h"

/********************************************************************************************
 Begin vrpn_Sound_Server_Miles
 *******************************************************************************************/
vrpn_Sound_Server_Miles::vrpn_Sound_Server_Miles(const char * name, vrpn_Connection * c) 
				  : vrpn_Sound_Server(name, c)
{
	AIL_quick_startup(1,0,44100,16,2);	//initialize the Miles SDK stuff

	//Set variables to control size and growing of Audio handle array
	H_Max = START;
	H_Cur = 0;

	//Allocate space for Audio Handle array
	handles = new HAUDIO[H_Max];
}

void vrpn_Sound_Server_Miles::unloadAllSounds()
{
	for(vrpn_int32 i = 0; i < H_Cur; i++)
	{
		if (handles[i] == NULL)
			continue;
		S32 status = AIL_quick_status(handles[i]);
		if(status == QSTAT_LOADED || status == QSTAT_PLAYING || status == QSTAT_DONE)
			//if handle is loaded, unload
		{
			AIL_quick_unload(handles[i]);
		}
	}
	H_Cur = 0;
}

vrpn_Sound_Server_Miles::~vrpn_Sound_Server_Miles()		
{
	unloadAllSounds();
	AIL_quick_shutdown();
	delete [] handles;
}

void vrpn_Sound_Server_Miles::addHandle(HAUDIO handle, vrpn_SoundID id)
{
	if (H_Cur >= H_Max)
	{
		int i;
		HAUDIO *oldhandles;

		oldhandles = handles;

		H_Max *= 2;
		handles = new HAUDIO[H_Max];

		for(i = 0; i < H_Cur; i++)
			handles[i] = oldhandles[i];

		delete [] oldhandles;
	}

	if (handle != NULL)
	{
		handles[H_Cur] = handle;
		set_CIndex_To_SIndex(id, H_Cur);
		H_Cur++;		
	}
	else
		set_CIndex_To_SIndex(id, FAIL);
}
HAUDIO vrpn_Sound_Server_Miles::getHandle(vrpn_SoundID id)
{
	vrpn_int32 SIndex;

	SIndex = map_CIndex_To_SIndex(id);

	if (SIndex == FAIL)
		return NULL;
	else
		return handles[SIndex];
}

void vrpn_Sound_Server_Miles::unloadHandle(vrpn_SoundID id)
{
	vrpn_int32 SIndex;

	SIndex = map_CIndex_To_SIndex(id);
	handles[SIndex] = NULL;
	set_CIndex_To_SIndex(id, FAIL);
}

void vrpn_Sound_Server_Miles::loadSound(char* filename, vrpn_SoundID id)
{
	addHandle(AIL_quick_load(filename), id);
}

void vrpn_Sound_Server_Miles::playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef)
//return 1 if not playing when called, plays
//return -1 if playing when called
//return 0 otherwise	
{
	//if not playing, play
	//if playing already modify parameters in p for pose, vel, etc but ignore loop count

	switch(AIL_quick_status(getHandle(id)))
	{
		case QSTAT_DONE:
		case QSTAT_LOADED:
		{
			AIL_quick_set_volume(getHandle(id), soundDef.volume, 0);
			AIL_quick_play(getHandle(id), repeat);
			break;
		}
		case QSTAT_PLAYING:
		{
			break;
		}
		default:
		{
			break;
		}
	}
}
	

void vrpn_Sound_Server_Miles::stopSound(vrpn_SoundID id)
{	
	AIL_quick_halt(getHandle(id));
}

void vrpn_Sound_Server_Miles::unloadSound(vrpn_SoundID id)
{
	AIL_quick_unload(getHandle(id));
	unloadHandle(id);
}

void vrpn_Sound_Server_Miles::mainloop(const timeval *timeout)
{
	if (!connection->connected())
	{
		unloadAllSounds();
		unloadCSMap();
		unloadSoundDefs();
	}
	connection->mainloop(timeout);
}
#endif