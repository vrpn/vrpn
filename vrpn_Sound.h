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
	  vrpn_float64 velocity[4];
	  vrpn_float64 max_front_dist;
	  vrpn_float64 min_front_dist;
	  vrpn_float64 max_back_dist;
	  vrpn_float64 min_back_dist;
	  vrpn_int32 volume;
	} vrpn_SoundDef;

	typedef struct _vrpn_ListenerDef
	{
	  vrpn_PoseDef pose;
	  vrpn_float64 velocity[4];
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
	vrpn_int32 change_sound_status;			 // ID of message to change the sound's status
	vrpn_int32 change_listener_status;		 // ID of message to change the listener's status
	struct timeval timestamp;				 // Current timestamp
	

	/*All encodes and decodes functions are for the purpose of setting up
	  messages to be sent over the network properly (ie to put them in one
	  char buffer and to put them in proper network order and for getting
	  the messages back into a usable format once they have been received*/

	/*Note encodeSound allocates space dynamically for buf, it is your
	  responsibility to free it up*/
	vrpn_int32 encodeSound(const char *filename, const vrpn_SoundID id, char **buf);
	/*Note decodeSound allocates space dynamically for filename, it is your
	  responsibility to free it up*/
	vrpn_int32 decodeSound(const char *buf, char **filename, vrpn_SoundID *id, const int payload);
	vrpn_int32 encodeSoundID(const vrpn_SoundID id, char* buf);
	vrpn_int32 decodeSoundID(const char* buf, vrpn_SoundID *id);
	vrpn_int32 encodeSoundDef(const vrpn_SoundDef sound, const vrpn_SoundID id, const vrpn_int32 repeat, char* buf);
	vrpn_int32 decodeSoundDef(const char* buf, vrpn_SoundDef *sound, vrpn_SoundID *id, vrpn_int32 *repeat);
	vrpn_int32 encodeListener(const vrpn_ListenerDef Listener, char* buf);
	vrpn_int32 decodeListener(const char* buf, vrpn_ListenerDef *Listener);

public:
	vrpn_Sound(const char * name, vrpn_Connection * c);
	~vrpn_Sound();
	virtual void mainloop(const struct timeval * timeout=NULL) = 0;
};

class vrpn_Sound_Client : public vrpn_Sound
{
public:
	vrpn_Sound_Client(const char * name, vrpn_Connection * c);
	~vrpn_Sound_Client();

	//This command starts a sound playing, the repeat value indicates how
	//many times to play it.  Continuously if repeat is set to 0
	vrpn_int32 playSound(const vrpn_SoundID id, vrpn_int32 repeat);
	vrpn_int32 stopSound(const vrpn_SoundID id);
	//Loads a sound into memory on the server side, returns the ID value to be
	//used to refer to the sound from now on.  Pass in the path and filename
	vrpn_SoundID loadSound(const char* sound);
	vrpn_int32 unloadSound(const vrpn_SoundID id);

	//All the functions with change and sound in them, can change either an
	//already playing sound or one yet to be played
	vrpn_int32 changeSoundVolume(const vrpn_SoundID id, const vrpn_int32 volume);
	vrpn_int32 changeSoundPose(const vrpn_SoundID id, vrpn_float64 position[3], vrpn_float64 orientation[4]);
	vrpn_int32 changeSoundVelocity(const vrpn_SoundID id, vrpn_float64 velocity[4]);
	
	// set min/max distances for a specific sound
	  // the max_front_dist is distance at which sound will be completely silent when sound in front of listener
	  // the min_front_dist is distance at which sound will be at max volume when sound in front of listener
	  // the max_back_dist is distance at which sound will be completely silent when sound behind listener
	  // the min_back_dist is distance at which sound will be completely silent when sound behind listener
	vrpn_int32 changeSoundDistances(const vrpn_SoundID id, const vrpn_float64 max_front_dist, const vrpn_float64 min_front_dist, const vrpn_float64 max_back_dist, const vrpn_float64 min_back_dist);

	//The position, orientation and velocity of the listener can change how it sounds
	vrpn_int32 changeListenerPose(const vrpn_float64 position[3], const vrpn_float64 orientation[4]);
	vrpn_int32 changeListenerVelocity(const vrpn_float64 velocity[4]);

	void mainloop(const struct timeval * timeout=NULL);

protected:
	void initSoundDef(vrpn_SoundDef* soundDef);
	/* Since there can be multiple sounds, we need to keep track of
	   all the defintion structures for each file.  Since there is no
	   set limit to how many sounds can be loaded at once, these functions
	   are here for the purpose of controlling the array of those soundDefs.
	   All dynamic allocation is handled by them.*/
	vrpn_SoundID addSoundDef(vrpn_SoundDef sound);
	inline vrpn_SoundDef getSoundDef(vrpn_SoundID id)
	{
		vrpn_SoundDef soundDef;
		initSoundDef(&soundDef);
		return (id < Defs_MaxNum) ? soundDefs[id] : soundDef;
	};
	inline void unloadSoundDefs() 
	{
		Defs_CurNum = 0;
	};
	inline void setDefVolume(vrpn_SoundID id, vrpn_int32 volume)
	{
		soundDefs[id].volume = volume;
	};
	inline void setDefPose(vrpn_SoundID id, vrpn_float64 position[3], vrpn_float64 orientation[4])
	{
		int i;
		for(i = 0; i < 3; i++) soundDefs[id].pose.position[i] = position[i];
		for(i = 0; i < 4; i++) soundDefs[id].pose.orientation[i] = orientation[i];
	};
	inline void setDefVelocity(vrpn_SoundID id, vrpn_float64 velocity[4])
	{
		int i;
		for(i = 0; i < 4; i++) soundDefs[id].velocity[i] = velocity[i];
	};
	inline void setDefDistances(vrpn_SoundID id, vrpn_float64 max_front_dist,vrpn_float64 min_front_dist,vrpn_float64 max_back_dist,vrpn_float64 min_back_dist) {
		soundDefs[id].max_front_dist = max_front_dist;
		soundDefs[id].min_front_dist = min_front_dist;
		soundDefs[id].max_back_dist  = max_back_dist;
		soundDefs[id].min_back_dist  = min_back_dist;
	};

private:
	vrpn_SoundDef *soundDefs;
	vrpn_int32 Defs_MaxNum;
	vrpn_int32 Defs_CurNum;
};

/*Note on the server design
  The server is designed in such a way that it expects a sub-class that is implemented
  that actually implements sound functionality to have certain functions that it can
  call to tell the child to play, load, whatever.   This parent server class, handles
  all of the callback functionality and decoding, allowing child classes to only have 
  to worry about sound functionality*/
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
	virtual void changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef) = 0;
	virtual void changeListenerStatus(vrpn_ListenerDef listener) = 0;
	
protected:
#define vrpn_Sound_FAIL -1								//To be set when in in the Mapping
											//when a sound fails to be loaded

	inline vrpn_int32 map_CIndex_To_SIndex(vrpn_int32 CIndex)
	{
		return (CIndex < CSMap_MaxNum) ? CSMap[CIndex] : vrpn_Sound_FAIL;
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
	static int handle_soundStatus(void *userdata, vrpn_HANDLERPARAM p);
	static int handle_listener(void *userdata, vrpn_HANDLERPARAM p);
};
#endif //#ifndef VRPN_CLIENT_ONLY

#define VRPN_SOUND_H
#endif
