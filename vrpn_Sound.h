#ifndef	VRPN_SOUND_H

#include "vrpn_Connection.h"
#include "vrpn_Shared.h"

#if defined(__CYGWIN__)
#include "vrpn_cygwin_hack.h"
#endif

typedef vrpn_int32 vrpn_SoundID;

class vrpn_Sound
{
public:

	typedef struct _vrpn_PoseDef
	{
	  vrpn_float64 position[3];
	  vrpn_float64 orientation[4];
	} vrpn_PoseDef;

	typedef struct _vrpn_SoundDef
	{
	  vrpn_PoseDef pose;
	  vrpn_float64 velocity[3];
	  vrpn_int32 volume;
	} vrpn_SoundDef;

	typedef struct _vrpn_ListenerDef
	{
	  vrpn_PoseDef pose;
	  vrpn_float64 velocity[3];
	} vrpn_ListenerDef;

protected:
#define vrpn_Sound_START 10

	vrpn_ListenerDef Listener;				 // A listeners information
	vrpn_Connection *connection;		     // Used to send messages
	vrpn_int32 my_id;						 // ID of this tracker to connection
	vrpn_int32 load_sound;					 // ID of message to load a sound
	vrpn_int32 unload_sound;				 // ID of message to unload a sound
	vrpn_int32 play_sound;					 // ID of message to play a sound
	vrpn_int32 stop_sound;					 // ID of message to stop a sound
	vrpn_int32 change_listener_status;		 // ID of message to change the listener's status
	struct timeval timestamp;				 // Current timestamp
	

	/*All encodes and decodes functions are for the purpose of setting up
	  messages to be sent over the network properly (ie to put them in one
	  char buffer and to put them in proper network order and for getting
	  the messages back into a usable format once they have been received*/

	/*Note encodeSound allocates space dynamically for buf, it is your
	  responsibility to free it up*/
	vrpn_int32 encodeSound(char *filename, vrpn_SoundID id, char **buf);
	/*Note decodeSound allocates space dynamically for filename, it is your
	  responsibility to free it up*/
	vrpn_int32 decodeSound(char *buf, char **filename, vrpn_SoundID *id, int payload);
	vrpn_int32 encodeSoundID(vrpn_SoundID id, char* buf);
	vrpn_int32 decodeSoundID(char* buf, vrpn_SoundID *id);
	vrpn_int32 encodeSoundDef(vrpn_SoundDef sound, vrpn_SoundID id, vrpn_int32 repeat, char* buf);
	vrpn_int32 decodeSoundDef(char* buf, vrpn_SoundDef *sound, vrpn_SoundID *id, vrpn_int32 *repeat);
	vrpn_int32 encodeListener(vrpn_ListenerDef Listener, char* buf);
	vrpn_int32 decodeListener(char* buf, vrpn_ListenerDef *Listener);

	/* Since there can be multiple sounds, we need to keep track of
	   all the defintion structures for each file.  Since there is no
	   set limit to how many sounds can be loaded at once, these functions
	   are here for the purpose of controlling the array of those soundDefs.
	   All dynamic allocation is handled by them.*/
	vrpn_SoundID addSoundDef(vrpn_SoundDef sound);
	inline vrpn_SoundDef getSoundDef(vrpn_SoundID id)
	{
		return soundDefs[id];
	};
	inline void unloadSoundDefs() 
	{
		Defs_CurNum = 0;
	};

public:
	vrpn_Sound(const char * name, vrpn_Connection * c);
	~vrpn_Sound();
	virtual void mainloop(const struct timeval * timeout=NULL) = 0;

private:
	vrpn_SoundDef *soundDefs;
	vrpn_int32 Defs_MaxNum;
	vrpn_int32 Defs_CurNum;
};

class vrpn_Sound_Client : public vrpn_Sound
{
public:
	vrpn_Sound_Client(const char * name, vrpn_Connection * c);
	~vrpn_Sound_Client();

	//This command starts a sound playing, the repeat value indicates how
	//many times to play it.  Continuously if repeat is set to 0
	vrpn_int32 playSound(vrpn_SoundID id, vrpn_int32 repeat);
	vrpn_int32 stopSound(vrpn_SoundID id);
	//Loads a sound into memory on the server side, returns the ID value to be
	//used to refer to the sound from now on.  Pass in the path and filename
	vrpn_SoundID loadSound(char* sound);
	vrpn_int32 unloadSound(vrpn_SoundID id);

	//All the functions with change and sound in them, can change either an
	//already playing sound or one yet to be played
	vrpn_int32 changeSoundVolume(vrpn_SoundID id, vrpn_int32 volume);
	vrpn_int32 changeSoundPose(vrpn_SoundID id, vrpn_float32 position[3], vrpn_float32 orientation[4]);
	vrpn_int32 changeSoundVelocity(vrpn_SoundID id, vrpn_float32 velocity[3]);

	//The position, orientation and velocity of the listener can change how it sounds
	vrpn_int32 changeListenerPose(vrpn_float32 position[3], vrpn_float32 orientation[4]);
	vrpn_int32 changeListenerVelocity(vrpn_float32 velocity[3]);

	void mainloop(const struct timeval * timeout=NULL);

protected:
	void initSoundDef(vrpn_SoundDef* soundDef);
};

#ifndef VRPN_CLIENT_ONLY
class vrpn_Sound_Server : public vrpn_Sound
{
public:
	vrpn_Sound_Server(const char * name, vrpn_Connection * c);
	~vrpn_Sound_Server();

	virtual void playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef) = 0;
	virtual void loadSound(char* filename, vrpn_SoundID id) = 0;
	virtual void stopSound(vrpn_SoundID id) = 0;
	virtual void unloadSound(vrpn_SoundID id) = 0;
	
protected:
#define vrpn_Sound_FAIL -1								//To be set when in in the Mapping
											//when a sound fails to be loaded

	inline vrpn_int32 map_CIndex_To_SIndex(vrpn_int32 CIndex)
	{
		return CSMap[CIndex];
	};
	void set_CIndex_To_SIndex(vrpn_int32 CIndex, vrpn_int32 SIndex);

	inline void unloadCSMap() 
	{
		for(int i = 0; i < CSMap_MaxNum; i++) 
			CSMap[i] = vrpn_Sound_FAIL;
	};

private:
	vrpn_int32 *CSMap;
	vrpn_int32 CSMap_MaxNum;

	static int handle_playSound(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_stopSound(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_loadSound(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_unloadSound(void *userdata, vrpn_HANDLERPARAM p);
};
#endif //#ifndef VRPN_CLIENT_ONLY

#define VRPN_SOUND_H
#endif
