#ifndef VRPN_SOUND_MILES

#ifdef _WIN32

#include "vrpn_Sound.h"
#include "pc_win32/mss.h"


class vrpn_Sound_Server_Miles : public vrpn_Sound_Server
{
public:
	vrpn_Sound_Server_Miles(const char * name, vrpn_Connection * c);
	~vrpn_Sound_Server_Miles();

	void playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef);
	void stopSound(vrpn_SoundID id);
	void loadSound(char* filename, vrpn_SoundID id);
	void unloadSound(vrpn_SoundID id);

	void mainloop(const struct timeval * timeout=NULL);

private:
	HAUDIO *handles;
	vrpn_int32 H_Max;
	vrpn_int32 H_Cur;

	void addHandle(HAUDIO handle, vrpn_SoundID id);
	HAUDIO getHandle(vrpn_SoundID id);

	void unloadAllSounds();
	void unloadHandle(vrpn_SoundID id);
};
#endif //_WIN32
#define VRPN_SOUND_MILES
#endif