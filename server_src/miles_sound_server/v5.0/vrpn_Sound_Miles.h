
#ifndef VRPN_SOUND_MILES


#ifdef _WIN32

#include "c:\miles\mss.h"
#include "vrpn_Sound.h"
#include "quat.h"

extern void ChangeSoundIdBox(vrpn_int32 action, vrpn_int32 newId);

class vrpn_Sound_Server_Miles : public vrpn_Sound_Server
{
public:
	vrpn_Sound_Server_Miles(const char * name, vrpn_Connection * c);
						/*Name is the sender name. c is the connection. 
						ThreeDProvider is the 3D audio provider.  Creative, huh.
						It provides 3D audio on a level underneath the Miles code and can 
						be swapped out when a new provider comes along.  This will just 
						necessitate including the provider file in the application directory
						and #defining an alias for it's name string so that it can be passed
						to the constructor.  
						The providerRoomSetting parameter changes the type of environment 
						the 3D audio provider will simulate.  There are a number of 
						possibilities, including (all prefaced with "EAX_ENVIRONMENT_")
						ROOM, BATHROOM, LIVINGROOM, CARPETEDHALLWAY, and HALLWAY.  The 
						complete list is on page 214 of the MSS 5.0 manual. */
	~vrpn_Sound_Server_Miles();

	void		setProvider(int index, int providerRoomSetting = EAX_ENVIRONMENT_GENERIC);  
					    //Set the 3D sound provider
	void		playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef);
						/*vrpn_SoundID's are used to access sounds in memory.  One is set
						by calling loadSound.  
						repeat is the number of times to play the sound.  Use 0 for the sound
						to loop until stopped.  
						soundDef has pose, velocity, and volume information
						*/
	void		stopSound(vrpn_SoundID id);
						//immediately stops playing the sound referred to by id
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
	
	void        changeE_f_S_m(qogl_matrix_type newmatrix); // sets the eye_from_sensor matrix

	void		addProvider(HPROVIDER p);
	void		mainloop(const struct timeval * timeout=NULL);
	bool		noSounds(void);
	void		stopAllSounds();
	void		shutDown();
	vrpn_int32  GetCurrentVolume(const vrpn_int32 CurrentSoundId);
	char      * GetLastError();
	vrpn_int32  GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId);
	void        GetCurrentPosition(const vrpn_int32 CurrentSoundId, F32* X_val, F32* Y_val, F32* Z_val);
	void        GetListenerPosition(F32* X_val, F32* Y_val, F32* Z_val);
	void        GetCurrentDistances(const vrpn_int32 CurrentSoundId, F32* FMin, F32* FMax, F32* BMin, F32* BMax);
	void        GetListenerOrientation(F32* X_val, F32 *Y_val, F32 *Z_val);
	void        GetCurrentOrientation(const vrpn_int32 CurrentSoundId,F32 *X_val, F32 *Y_val, F32 *Z_val);
	
	vrpn_int32    LastSoundId;		//ID of last played sound	

	qogl_matrix_type eye_f_sensor_m; // this is the matrix that goes from the
							         // sensor to the eyeball space (so for example
	                                 // it could be eyeball_f_hiballsensor [ where 
	                                 // hiballsensor would be the hiball mounted 
									 // on the HMD   

private:
			
	H3DSAMPLE*	samples;			/*audio handles for Miles, used as an array
									accessed with getSample and addSample, 
									which are passed soundID's*/
	HDIGDRIVER	DIG;				//digital driver	 	
	HPROVIDER	provider;			//Current 3D audio provider
	HPROVIDER	*providers;			//3D audio provider
	H3DPOBJECT	listener;			//listener
	
	vrpn_int32	  H_Max;			//size of array samples
	vrpn_int32	  H_Cur;			//index in samples
	vrpn_int32	  P_Max;			//size of array samples
	vrpn_int32	  P_Cur;			//index in samples

	void		addSample(H3DSAMPLE sample, vrpn_SoundID id);
									//adds a sample to the location in samples corresponding to id
	H3DSAMPLE	getSample(vrpn_SoundID id);
									//gets the handle stored in samples[id]
	void		chooseProvider(char * ThreeDProvider);
									/*chooses a 3D audio provider.  
									See the comment below the constructor for more info.*/
	void		unloadAllSounds();	//clears out samples
	void		unloadHandle(vrpn_SoundID id);
									//unloads a handle corresponding to id from samples
	bool		InitWindow(HINSTANCE  hInstance);   //Initializes the dialog window
};
#endif //_WIN32
#define VRPN_SOUND_MILES

#endif