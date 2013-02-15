#ifndef VRPN_SOUND_ASM_H
#define VRPN_SOUND_ASM_H

#ifdef _WIN32
#include "vrpn_Shared.h"

// Define PI if not already defined
#ifndef PI
#define PI 3.1415926535897932384626433832795f
#endif

#include <objbase.h>
#include <stdlib.h>
#include <cguid.h>
#include "vrpn_Sound.h"
#include "quat.h"
#include "string.h"
#include <conio.h>
#include <mmsystem.h>
#include <mmreg.h>
#include "cre_tron.h"
#include "cre_wave.h"
#include <stdio.h>

class vrpn_Sound_Server_ASM;
class ASMSound;

typedef struct{
	char m_ASMSoundName[80];
	int	 m_iSoundNum;
} SoundMap;

typedef struct{
	ASMSound*	m_pSound;
	int			repeat;
	float		m_fPos[3];
	float		m_fOri[3];
	wavFt*		wavefile;
	float		m_fMinDist;
} Sound;

typedef struct{
	float		m_fPos[3];
	float		m_fOri[3];
} Listener;


// Macros
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#define DEG2RAD(x) (x * (PI / 180.0f)) // Converts degrees to radians
#define RAD2DEG(x) (x * (180.0f / PI)) // Converts radians to degrees

class vrpn_Sound_Server_ASM : public vrpn_Sound_Server {

public:
	vrpn_Sound_Server_ASM(const char * name, vrpn_Connection * c );

	~vrpn_Sound_Server_ASM();

	void		playSound(vrpn_SoundID id, vrpn_int32 repeat, vrpn_SoundDef soundDef);
	void		stopSound(vrpn_SoundID id);

	void		loadSoundLocal(char* filename, vrpn_SoundID id, vrpn_SoundDef soundDef);
	void		loadSoundRemote(char* file, vrpn_SoundID id, vrpn_SoundDef soundDef); // not supported
	void		unloadSound(vrpn_SoundID id);
	void		changeSoundStatus(vrpn_SoundID id, vrpn_SoundDef soundDef);
	
	void		setListenerPose(vrpn_PoseDef pose);
	void		setListenerVelocity(vrpn_float64 *velocity);

	void		setSoundPose(vrpn_SoundID id, vrpn_PoseDef pose);
	void		setSoundVelocity(vrpn_SoundID id, vrpn_float64 *velocity);
	void		setSoundDistInfo(vrpn_SoundID id, vrpn_float64 *distinfo);
	void		setSoundConeInfo(vrpn_SoundID id, vrpn_float64 *coneinfo); 
  
	void		setSoundDoplerFactor(vrpn_SoundID id, vrpn_float64 doplerfactor);
	void		setSoundEqValue(vrpn_SoundID id, vrpn_float64 eqvalue); // not supported
	void		setSoundPitch(vrpn_SoundID id, vrpn_float64 pitch);
	void		setSoundVolume(vrpn_SoundID id, vrpn_float64 volume);

	void		GetSoundList(void);


	// These are not supported
	void		loadModelLocal(const char * filename);
	void		loadModelRemote();	// not supported
	void		loadPolyQuad(vrpn_QuadDef * quad);
	void		loadPolyTri(vrpn_TriDef * tri);
	void		loadMaterial(vrpn_MaterialDef * material, vrpn_int32 id);
	void		setPolyQuadVertices(vrpn_float64 vertices[4][3], const vrpn_int32 id);
	void		setPolyTriVertices(vrpn_float64 vertices[3][3], const vrpn_int32 id);
	void		setPolyOF(vrpn_float64 OF, vrpn_int32 tag);
	void		setPolyMaterial(const char * material, vrpn_int32 tag);
	// end of unsupported functions


	void		mainloop();
	bool		noSounds(void);
	void		stopAllSounds();
	void		shutDown();

	vrpn_float64 GetCurrentVolume(const vrpn_int32 CurrentSoundId);
	vrpn_int32   GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId);
	void         GetCurrentPosition(const vrpn_int32 CurrentSoundId, float* X_val, float* Y_val, float* Z_val);
	void         GetListenerPosition(float* X_val, float* Y_val, float* Z_val);
	void         GetCurrentDistances(const vrpn_int32 CurrentSoundId, float* FMin, float* FMax);
	void         GetListenerOrientation(float* yaw, float *pitch, float *roll);
	void         GetCurrentOrientation(const vrpn_int32 CurrentSoundId,float *yaw, float *pitch, float *roll);
	
	vrpn_int32  LastSoundId;		//ID of last played sound	

private:

	// the idea of a map is to have a way to go from the unique id being passed in
	// by the user to an internal identifier...

	HRESULT        lasterror;
	vrpn_int32     maxSounds;
	vrpn_int32     numSounds;
	SoundMap	   g_soundMap[MAX_NUMBER_SOUNDS];
	vrpn_int32     numconnections; // number of times server has been connected to..
};


class ASMSound
{

public:
    wavFt*	m_pWav;
  	float	m_fVolume;		
	float		m_fPos[3];
	float		m_fOri[3];
	float	m_fMinDist;

	ASMSound( wavFt* pWav );
    virtual ~ASMSound();
    void Play(int id, int repeat, float fVolume = 0);
    void Stop(int id);
    void Reset(int id);
    BOOL    IsSoundPlaying(int id);
};

#endif //_WIN32
#endif