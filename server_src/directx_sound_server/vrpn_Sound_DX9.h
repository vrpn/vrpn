#ifndef VRPN_SOUND_DX9_H
#define VRPN_SOUND_DX9_H

#ifdef _WIN32

// Define PI if not already defined
#ifndef PI
#define PI 3.1415926535897932384626433832795f
#endif

#include <objbase.h>
#include <stdlib.h>
#include <cguid.h>
#include <windows.h>
#include <initguid.h>
#include "vrpn_Sound.h"
#include "quat.h"
#include "string.h"
#include <conio.h>
#include <dxerr8.h>
#include <dsound.h>
#include <mmsystem.h>
#include <mmreg.h>

class vrpn_Sound_Server_DX9;
class DX9Sound;
class DX9WaveFile;

typedef struct{
	char m_DX9SoundName[80];
	int	 m_iSoundNum;
} SoundMap;

typedef struct{
	DX9Sound* m_pSound;
	int		repeat;
} Sound;

// Macros
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#define DEG2RAD(x) (x * (PI / 180.0f)) // Converts degrees to radians
#define RAD2DEG(x) (x * (180.0f / PI)) // Converts radians to degrees


class vrpn_Sound_Server_DX9 : public vrpn_Sound_Server {

public:
	vrpn_Sound_Server_DX9(const char * name, vrpn_Connection * c, HWND hWin);

	~vrpn_Sound_Server_DX9();

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
	void         GetListenerOrientation(double* y_val, double *p_val, double *r_val);
	void         GetCurrentOrientation(const vrpn_int32 CurrentSoundId,float *X_val, float *Y_val, float *Z_val);
	
	vrpn_int32  LastSoundId;		//ID of last played sound	

private:

	// the idea of a map is to have a way to go from the unique id being passed in
	// by the user to an internal identifier...

	HRESULT        lasterror;
	vrpn_int32     maxSounds;
	vrpn_int32     numSounds;
	SoundMap	   g_soundMap[MAX_NUMBER_SOUNDS];
	vrpn_int32     numconnections; // number of times server has been connected to..

    HRESULT		CreateSecondaryBuffer( DX9Sound** ppSound, LPTSTR strWaveFileName, DWORD dwCreationFlags = 0, GUID guid3DAlgorithm = GUID_NULL, DWORD dwNumBuffers = 1 );
};


class DX9Sound
{
protected:
    LPDIRECTSOUNDBUFFER* m_apDSBuffer;
    DWORD                m_dwDSBufferSize;
    DX9WaveFile*         m_pWaveFile;
    DWORD                m_dwNumBuffers;
    DWORD                m_dwCreationFlags;
	long				 m_fvolume;  // 0 max to -10000 min

    HRESULT RestoreBuffer( LPDIRECTSOUNDBUFFER pDSB, BOOL* pbWasRestored );

public:
    DX9Sound( LPDIRECTSOUNDBUFFER* apDSBuffer, DWORD dwDSBufferSize, DWORD dwNumBuffers, DX9WaveFile* pWaveFile, DWORD dwCreationFlags );
    virtual ~DX9Sound();

    HRESULT Get3DBufferInterface( DWORD dwIndex, LPDIRECTSOUND3DBUFFER* ppDS3DBuffer );
    HRESULT FillBufferWithSound( LPDIRECTSOUNDBUFFER pDSB );
    LPDIRECTSOUNDBUFFER GetFreeBuffer();
    LPDIRECTSOUNDBUFFER GetBuffer( DWORD dwIndex );

    HRESULT Play( DWORD dwPriority = 0, DWORD dwFlags = 0, LONG lVolume = 0);
    HRESULT Stop();
    HRESULT Reset();
    BOOL    IsSoundPlaying();
	void	SetBufferVolume(long volume = 0);
	long	GetBufferVolume();
};



class DX9WaveFile
{
public:
    WAVEFORMATEX* m_pwfx;        // Pointer to WAVEFORMATEX structure
    HMMIO         m_hmmio;       // MM I/O handle for the WAVE
    MMCKINFO      m_ck;          // Multimedia RIFF chunk
    MMCKINFO      m_ckRiff;      // Use in opening a WAVE file
    DWORD         m_dwSize;      // The size of the wave file
    MMIOINFO      m_mmioinfoOut;
    DWORD         m_dwFlags;
    BOOL          m_bIsReadingFromMemory;
    BYTE*         m_pbData;
    BYTE*         m_pbDataCur;
    ULONG         m_ulDataSize;
    CHAR*         m_pResourceBuffer;

public:
    DX9WaveFile();
    ~DX9WaveFile();
    HRESULT Open( LPTSTR strFileName, WAVEFORMATEX* pwfx, DWORD dwFlags );
    HRESULT Close();
    HRESULT Read( BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead );
    DWORD   GetSize();
    HRESULT ResetFile();
    WAVEFORMATEX* GetFormat() { return m_pwfx; };
};


#endif //_WIN32
#endif