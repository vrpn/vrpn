#include "vrpn_Sound.h"
#include <string.h>

//vrpn_Sound constructor.
vrpn_Sound::vrpn_Sound(const char * name, vrpn_Connection * c)
{
	char * servicename;
  
	servicename = vrpn_copy_service_name(name);
	
	if (c == NULL)
	{
		fprintf(stderr, "vrpn_Sound: Connection passed is null\n");
		exit(1);
	}

	connection = c;
	//Register the sender
	my_id = connection->register_sender(servicename);

	//Register the types
	load_sound = connection->register_message_type("Sound Load");
	unload_sound = connection->register_message_type("Sound Unload");
	play_sound = connection->register_message_type("Sound Play");
	stop_sound = connection->register_message_type("Sound Stop");
	change_listener_status = connection->register_message_type("Listener");

	// Set the current time to zero, just to have something there
	timestamp.tv_sec = 0;
	timestamp.tv_usec = 0;

	/*Values used for determining whether to grow the array of sound definitions or not*/
	Defs_MaxNum = START;
	Defs_CurNum = 0;

	/*The array of sound definitions needs to be initialized with some starting space*/
	soundDefs = new vrpn_SoundDef[Defs_MaxNum];

	if (servicename)
       delete [] servicename;
}

vrpn_Sound::~vrpn_Sound()
{
	delete [] soundDefs;
}

//Used to send the filename to load and the index to be used to refer 
//to the sound by the client from now on
vrpn_int32 vrpn_Sound::encodeSound(char *filename, vrpn_SoundID id, char **buf)
{
	vrpn_int32 len = sizeof(vrpn_SoundID) + strlen(filename) + 1;
	vrpn_int32 ret = len;
	char *mptr;

	*buf = new char[strlen(filename) + sizeof(vrpn_SoundID) + 1];

	mptr = *buf;
	vrpn_buffer(&mptr, &len, id);
	vrpn_buffer(&mptr, &len, filename, strlen(filename)+1);

	return ret;
}

//Decodes the file and Client Index number
vrpn_int32 vrpn_Sound::decodeSound(char *buf, char **filename, vrpn_SoundID *id, int payload)
{
	const char *mptr = buf;

	*filename = new char[payload - sizeof(vrpn_SoundID)];
	
	vrpn_unbuffer(&mptr, id);
	vrpn_unbuffer(&mptr, *filename, payload - sizeof(vrpn_SoundID));
	
	return 0;
}

//Encodes the client sound ID 
vrpn_int32 vrpn_Sound::encodeSoundID(vrpn_SoundID id, char* buf)
{
	char* mptr = buf;
	vrpn_int32 len = sizeof(vrpn_SoundID);

	vrpn_buffer(&mptr,&len, id);

	return (sizeof(vrpn_SoundID));
}

//Decodes the client sound ID 
vrpn_int32 vrpn_Sound::decodeSoundID(char* buf, vrpn_SoundID *id)
{
	const char* mptr = buf;

	vrpn_unbuffer(&mptr, id);

	return 0;
}

//Sends all the information necessary to play a sound appropriately.
//IE, The sounds position, orientation, velocity, volume and repeat count
vrpn_int32 vrpn_Sound::encodeSoundDef(vrpn_SoundDef sound, vrpn_SoundID id, vrpn_int32 repeat, char* buf)
{
	char *mptr = buf;
	vrpn_int32 len = sizeof(vrpn_SoundDef) + sizeof(vrpn_SoundID) + sizeof(vrpn_int32);
	vrpn_int32 ret = len;
	int i;

	vrpn_buffer(&mptr, &len, repeat);
	vrpn_buffer(&mptr, &len, id);

	for(i = 0; i < 3; i++)
		vrpn_buffer(&mptr, &len, sound.pose.position[i]);

	for(i = 0; i < 4; i++)
		vrpn_buffer(&mptr, &len, sound.pose.orientation[i]);

	for(i = 0; i < 3; i++)
		vrpn_buffer(&mptr, &len, sound.velocity[i]);

	vrpn_buffer(&mptr, &len, sound.volume);

	return ret;
	
}

vrpn_int32 vrpn_Sound::decodeSoundDef(char* buf, vrpn_SoundDef *sound, vrpn_SoundID *id, vrpn_int32 *repeat)
{
	const char *mptr = buf;
	int i;

	vrpn_unbuffer(&mptr, repeat);
	vrpn_unbuffer(&mptr, id);

	for(i = 0; i < 3; i++)
		vrpn_unbuffer(&mptr, &(sound->pose.position[i]));

	for(i = 0; i < 4; i++)
		vrpn_unbuffer(&mptr, &(sound->pose.orientation[i]));

	for(i = 0; i < 3; i++)
		vrpn_unbuffer(&mptr, &(sound->velocity[i]));

	vrpn_unbuffer(&mptr, &(sound->volume));

	return 0;
}

//Sends information about the listener. IE  position, orientation and velocity
vrpn_int32 vrpn_Sound::encodeListener(vrpn_ListenerDef Listener, char* buf)
{
	vrpn_float64 *dbuf = (vrpn_float64 *) buf;
	int index = 0;
	int i;

	for(i = 0; i < 3; i++)
		dbuf[index++] = htond(Listener.pose.position[i]);

	for(i = 0; i < 4; i++)
		dbuf[index++] = htond(Listener.pose.orientation[i]);

	for(i = 0; i < 3; i++)
		dbuf[index++] = htond(Listener.velocity[i]);

	return index*sizeof(vrpn_float64);
}

vrpn_int32 vrpn_Sound::decodeListener(char* buf, vrpn_ListenerDef *Listener)
{
	vrpn_float64 *dbuf = (vrpn_float64 *) buf;
	int index = 0;
	int i;

	for(i = 0; i < 3; i++)
		Listener->pose.position[i] = ntohd(dbuf[index++]);

	for(i = 0; i < 4; i++)
		Listener->pose.orientation[i] = ntohd(dbuf[index++]);

	for(i = 0; i < 3; i++)
		Listener->velocity[i] = ntohd(dbuf[index++]);

	return 0;
}
//Add a new sound def to the array.  If adding it would overflow
//the current allocated space, then double the available space and then add
vrpn_SoundID vrpn_Sound::addSoundDef(vrpn_SoundDef sound)
{
	int i;
	vrpn_SoundID returnVal;
	vrpn_SoundDef *oldsounds;

	if (Defs_CurNum >= Defs_MaxNum)
	{
		oldsounds = soundDefs;
		Defs_MaxNum *= 2;
		soundDefs = new vrpn_SoundDef[Defs_MaxNum];
		for(i = 0; i < Defs_CurNum; i++);
		    soundDefs[i] = oldsounds[i];
		delete [] oldsounds;
	}

	//Copy all information into the current free spot in the array
	for(i = 0; i < 3; i++)
		soundDefs[Defs_CurNum].pose.position[i] = sound.pose.position[i];

	for(i = 0; i < 4; i++)
		soundDefs[Defs_CurNum].pose.orientation[i] = sound.pose.orientation[i];

	for(i = 0; i < 3; i++)
		soundDefs[Defs_CurNum].velocity[i] = sound.velocity[i];

	soundDefs[Defs_CurNum].volume = sound.volume;

	returnVal = Defs_CurNum;
	Defs_CurNum++;

	return returnVal;
}

/********************************************************************************************
 Begin vrpn_Sound_Client
 *******************************************************************************************/
vrpn_Sound_Client::vrpn_Sound_Client(const char * name, vrpn_Connection * c) 
				  : vrpn_Sound(name, c)
{
}

vrpn_Sound_Client::~vrpn_Sound_Client()
{
}

/*Sounds when loaded default to certain settings.
  volume = 100%
  position = (0, 0, 0)
  orientation = (0, 0, 0, 0)
  velocity = (0, 0, 0)*/
void vrpn_Sound_Client::initSoundDef(vrpn_SoundDef* soundDef)
{
	int i;

	soundDef->volume = 100;
	
	for(i = 0; i < 3; i++)
		soundDef->pose.position[i] = 0;

	for(i = 0; i < 4; i++)
		soundDef->pose.orientation[i] = 0;

	for(i = 0; i < 3; i++)
		soundDef->velocity[i] = 0;
}

vrpn_int32 vrpn_Sound_Client::playSound(vrpn_SoundID id, vrpn_int32 repeat)
{
	char* buf = new char[sizeof(vrpn_SoundDef) + sizeof(vrpn_SoundID) + sizeof(vrpn_int32)];
	vrpn_int32 len;

	len = encodeSoundDef(getSoundDef(id), id, repeat, buf);

	gettimeofday(&timestamp, NULL);

	if (connection->pack_message(len, timestamp, play_sound, my_id, buf, vrpn_CONNECTION_RELIABLE))
      fprintf(stderr,"vrpn_Sound_Client: cannot write message play: tossing\n");

	connection->mainloop();
	delete [] buf;
	return 0;
}

vrpn_int32 vrpn_Sound_Client::stopSound(vrpn_SoundID id)
{
	char* buf = new char[sizeof(vrpn_SoundID)];
	vrpn_int32 len;

	len = encodeSoundID(id, buf);

	gettimeofday(&timestamp, NULL);

	if (connection->pack_message(len, timestamp, stop_sound, my_id, buf, vrpn_CONNECTION_RELIABLE))
      fprintf(stderr,"vrpn_Sound_Client: cannot write message play: tossing\n");

	connection->mainloop();
	delete [] buf;
	return 0;
}

/*Sounds when loaded default to certain settings.
  volume = 100%
  position = (0, 0, 0)
  orientation = (0, 0, 0, 0)
  velocity = (0, 0, 0)*/

vrpn_SoundID vrpn_Sound_Client::loadSound(char* sound)
{
	vrpn_int32 len, curIndex;
	char* buf;
	vrpn_SoundDef soundDef;

	initSoundDef(&soundDef);
	curIndex = addSoundDef(soundDef);

	len = encodeSound(sound, curIndex, &buf);
	
	gettimeofday(&timestamp, NULL);

	if (connection->pack_message(len, timestamp, load_sound, my_id, buf, vrpn_CONNECTION_RELIABLE))
      fprintf(stderr,"vrpn_Sound_Client: cannot write message load: tossing\n");

	connection->mainloop();
	delete [] buf;
	return curIndex;
}

vrpn_int32 vrpn_Sound_Client::unloadSound(vrpn_SoundID id)
{
	vrpn_int32 len;
	char *buf = new char[sizeof(vrpn_SoundID)];

	len = encodeSoundID(id, buf);

	gettimeofday(&timestamp, NULL);

	if (connection->pack_message(len, timestamp, unload_sound, my_id, buf, vrpn_CONNECTION_RELIABLE))
      fprintf(stderr,"vrpn_Sound_Client: cannot write message unload: tossing\n");

	connection->mainloop();
	delete [] buf;
	return 0;
}


void vrpn_Sound_Client::mainloop(const timeval *timeout)
{
	connection->mainloop(timeout);
}

/********************************************************************************************
 Begin vrpn_Sound_Server
 *******************************************************************************************/
#ifndef VRPN_CLIENT_ONLY
vrpn_Sound_Server::vrpn_Sound_Server(const char * name, vrpn_Connection * c) 
				  : vrpn_Sound(name, c)
{
	/*Values used for determining whether to grow the array of client sound it to 
	  server id map or not*/
	CSMap_MaxNum = START;

	/*The mapping array*/
	CSMap = new vrpn_int32[CSMap_MaxNum];

	for(int i = 0; i < CSMap_MaxNum; i++) 
		CSMap[i] = FAIL;

	/*Register the handlers*/
	connection->register_handler(load_sound, handle_loadSound, this, my_id);
	connection->register_handler(unload_sound, handle_unloadSound, this, my_id);
	connection->register_handler(play_sound, handle_playSound, this, my_id);
	connection->register_handler(stop_sound, handle_stopSound, this, my_id);
}

vrpn_Sound_Server::~vrpn_Sound_Server()
{
	delete [] CSMap;
}

void vrpn_Sound_Server::set_CIndex_To_SIndex(vrpn_int32 CIndex, vrpn_int32 SIndex)
{
	if (CIndex >= CSMap_MaxNum)
	{
		vrpn_int32 *oldcsmap;
		int i;

		oldcsmap = CSMap;
		CSMap_MaxNum *= 2;
		CSMap = new vrpn_int32[CSMap_MaxNum];

		for(i = 0; i < CIndex; i++)
			CSMap[i] = oldcsmap[i];

		delete [] oldcsmap;
	}

	CSMap[CIndex] = SIndex;
}

int vrpn_Sound_Server::handle_loadSound(void *userdata, vrpn_HANDLERPARAM p)
{
	vrpn_Sound_Server *me = (vrpn_Sound_Server*)userdata;
	vrpn_SoundID id;
	char * filename;

	me->decodeSound((char*)p.buffer,&filename, &id, p.payload_len);
	me->loadSound(filename, id);
	delete [] filename;
	return 0;
}

int vrpn_Sound_Server::handle_playSound(void *userdata, vrpn_HANDLERPARAM p)
{
	vrpn_Sound_Server *me = (vrpn_Sound_Server*)userdata;
	vrpn_int32 return_value = 0;
	vrpn_SoundID id;
	vrpn_SoundDef soundDef;
	vrpn_int32 repeat;

	me->decodeSoundDef((char*)p.buffer, &soundDef, &id, &repeat);
	me->playSound(id, repeat, soundDef);
	return 0;
}
	

int vrpn_Sound_Server::handle_stopSound(void *userdata, vrpn_HANDLERPARAM p)	
{
	vrpn_Sound_Server *me = (vrpn_Sound_Server*)userdata;
	vrpn_SoundID id;

	me->decodeSoundID((char *)p.buffer, &id);
	me->stopSound(id);
	return 0;
}

int vrpn_Sound_Server::handle_unloadSound(void *userdata, vrpn_HANDLERPARAM p)
{
	vrpn_Sound_Server *me = (vrpn_Sound_Server*)userdata;
	vrpn_SoundID id;

	me->decodeSoundID((char *)p.buffer, & id);
	me->unloadSound(id);
	return 0;
}

#endif //#ifndef VRPN_CLIENT_ONLY