#ifndef VRPN_SOUND_A3D_H
#define VRPN_SOUND_A3D_H

#ifdef _WIN32
#include "vrpn_Shared.h"

#include <objbase.h>
#include <stdlib.h>
#include <cguid.h>

#include <initguid.h>
#include "ia3dutil.h"
#include "ia3dapi.h"

#include "vrpn_Sound.h"
#include "quat.h"

#include "string.h"
#include <conio.h>

class vrpn_Sound_Server_A3D : public vrpn_Sound_Server {
public:
	vrpn_Sound_Server_A3D(const char * name, vrpn_Connection * c, HWND hWin);
					
	~vrpn_Sound_Server_A3D();

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
	void		setSoundEqValue(vrpn_SoundID id, vrpn_float64 eqvalue);
	void		setSoundPitch(vrpn_SoundID id, vrpn_float64 pitch);
	void		setSoundVolume(vrpn_SoundID id, vrpn_float64 volume);
	void		loadModelLocal(const char * filename);
	void		loadModelRemote();	// not supported
	void		loadPolyQuad(vrpn_QuadDef * quad);
	void		loadPolyTri(vrpn_TriDef * tri);
	void		loadMaterial(vrpn_MaterialDef * material, vrpn_int32 id);
	void		setPolyQuadVertices(vrpn_float64 vertices[4][3], const vrpn_int32 id);
	void		setPolyTriVertices(vrpn_float64 vertices[3][3], const vrpn_int32 id);
	void		setPolyOF(vrpn_float64 OF, vrpn_int32 tag);
	void		setPolyMaterial(const char * material, vrpn_int32 tag);
	
	void		mainloop();
	bool		noSounds(void);
	void		stopAllSounds();
	void		shutDown();

	vrpn_float64 GetCurrentVolume(const vrpn_int32 CurrentSoundId);
	void         GetLastError(char * buf);
	vrpn_int32   GetCurrentPlaybackRate(const vrpn_int32 CurrentSoundId);
	void         GetCurrentPosition(const vrpn_int32 CurrentSoundId, float* X_val, float* Y_val, float* Z_val);
	void         GetListenerPosition(float* X_val, float* Y_val, float* Z_val);
	void         GetCurrentDistances(const vrpn_int32 CurrentSoundId, float* FMin, float* FMax);
	void         GetListenerOrientation(float* X_val, float *Y_val, float *Z_val);
	void         GetCurrentOrientation(const vrpn_int32 CurrentSoundId,float *X_val, float *Y_val, float *Z_val);
	
	vrpn_int32  LastSoundId;		//ID of last played sound	
private:
 
  // these are specific to A3D 3.0
	IA3d5        * a3droot;
	IA3dListener * a3dlis;
	IA3dSource2 ** a3dsamples;
	IA3dGeom2    * a3dgeom;
	HRESULT        lasterror;

  // the idea of a map is to have a way to go from the unique id being passed in
  //   by the user to an internal identifier...
	vrpn_int32     maxSounds;
	vrpn_int32     numSounds;
	vrpn_int32     soundMap[MAX_NUMBER_SOUNDS];

  vrpn_int32     maxMaterials;
  vrpn_int32     numMaterials;
  vrpn_int32     materialMap[MAX_NUMBER_MATERIALS];
  IA3dMaterial * materials[MAX_NUMBER_MATERIALS];
  char 		       mat_names[MAX_NUMBER_MATERIALS][MAX_MATERIAL_NAME_LENGTH];

  vrpn_int32     numconnections; // number of times server has been connected to..

};

#endif //_WIN32
#endif