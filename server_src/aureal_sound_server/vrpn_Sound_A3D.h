#ifndef VRPN_SOUND_A3D_H
#define VRPN_SOUND_A3D_H

#ifdef _WIN32

#define INITGUID


#include <objbase.h>
#include <stdlib.h>
#include <cguid.h>

#include <windows.h>

#include <initguid.h>
#include "ia3dutil.h"
#include "ia3dapi.h"

#include "vrpn_Sound.h"
#include "quat.h"

#include "string.h"
#include <conio.h>


class vrpn_Sound_Server_A3D : public vrpn_Sound_Server
{
public:
	vrpn_Sound_Server_A3D(const char * name, vrpn_Connection * c, HWND hWin);
					
	~vrpn_Sound_Server_A3D();

	void		setProvider(int index, int providerRoomSetting=0);  
					    //Set the 3D sound provider
	void		playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef);
						/*vrpn_SoundID's are used to access sounds in memory.  One is set
						by calling loadSound.  
						repeat is the number of times to play the sound.  Use 0 for the sound
						to loop until stopped.  
						soundDef has pose, velocity, and volume information
						*/
	void		stopSound(vrpn_SoundID id);
						//immediately stops playing the sound refered to by id
	void		loadSound(char* filename, vrpn_SoundID id);
						/*loads a .wav file into memory.  
						gives back id to reference the sound
						currently (7/28/99) this does not reuse space previously 
						vacated in samples by unloading sounds*/
	void		unloadSound(vrpn_SoundID id);
						/*unloads id from samples.  
						Sets the corresponding cell in samples to FAIL. */
	void		changeListenerStatus(vrpn_ListenerDef listenerDef);
						/*sets the listener's pose and velocity to what is passed in*/
	void		changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef);
						/*sets the sounds's pose and velocity to what is passed in.  
						If the sound is playing changes take place immediately.  
						If not nothing happens. */
	void        initModel(vrpn_ModelDef modelDef);
	void        DrawModel(char * filename);
	
	void        changeE_f_S_m(qogl_matrix_type newmatrix); // sets the eye_from_sensor matrix

	void		mainloop(const struct timeval * timeout=NULL);
	bool		noSounds(void);
	void		stopAllSounds();
	void		shutDown();
	vrpn_int32  GetCurrentVolume(const vrpn_int32 CurrentSoundId);
	char      * GetLastError();
	vrpn_int32  GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId);
	void        GetCurrentPosition(const vrpn_int32 CurrentSoundId, float* X_val, float* Y_val, float* Z_val);
	void        GetListenerPosition(float* X_val, float* Y_val, float* Z_val);
	void        GetCurrentDistances(const vrpn_int32 CurrentSoundId, float* FMin, float* FMax);
	void        GetListenerOrientation(float* X_val, float *Y_val, float *Z_val);
	void        GetCurrentOrientation(const vrpn_int32 CurrentSoundId,float *X_val, float *Y_val, float *Z_val);
	
	vrpn_int32    LastSoundId;		//ID of last played sound	

	qogl_matrix_type eye_f_sensor_m; // this is the matrix that goes from the
							         // sensor to the eyeball space (so for example
	                                 // it could be eyeball_f_hiballsensor [ where 
	                                 // hiballsensor would be the hiball mounted 
									 // on the HMD   
private:

	IA3d4        * a3droot;
	IA3dListener * a3dlis;
	IA3dSource  ** a3dsamples;
	IA3dGeom     * a3dgeom;
	HRESULT        lasterror;

	int            maxSounds;
	int            numSounds;
};

#endif //_WIN32
#endif