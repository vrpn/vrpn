#ifndef	VRPN_SOUND_H
#include "vrpn_Connection.h"

#define	vrpn_SND_LOOPED 1
#define	vrpn_SND_SINGLE 2 

#define	vrpn_SND_LEFT 1
#define	vrpn_SND_RIGHT 2
#define	vrpn_SND_BOTH 3

#define	vrpn_SND_SAMPLE 1  //for sample sound broadcast on /dev/audio
#define	vrpn_SND_MIDI 2  // for midi music on /dev/sequencer
#define vrpn_SND_WAVEFORM 3 //?
#define	vrpn_SND_STOP 4 
#define	vrpn_SND_PRELOAD 5 
#define vrpn_SND_SHUTDOWN -4 

// Base class for a sound server.  Definition
// of remote sound class for the user is at the end.
#define vrpn_SND_CHANNEL_NUM 5 
#define vrpn_SND_MAXFNUM 20

class vrpn_Sound {
  public:
	vrpn_Sound(vrpn_Connection *c = NULL): connection(c) {};

	// Called once through each main loop iteration to handle
	// updates.
	virtual void mainloop(void) = 0;	// Hear about play requests

  protected:
	vrpn_Connection *connection;
	vrpn_int32 my_id;		// ID of this soundserver to connection
	vrpn_int32 playsample_id;	// ID of "play sample" message
};

#ifndef VRPN_CLIENT_ONLY

// Sound server running under Linux.

class vrpn_Linux_Sound: public vrpn_Sound {
  public:
	// Open the sound devices connected to the local machine, talk to the
	// outside world through the connection.
	vrpn_Linux_Sound(char *name, vrpn_Connection *connection);
	virtual void mainloop(void);
	int mapping(char *name, int address);
	int checkpipe();

  protected:
  	int     status; // playing or idle which will be used by child process
                    // to dertermine stopping the playing sound
	int	sound_type; // most-recent sound type 
	int 	ear_mode; // mono or stereo

        // for sample sound
	int   	fd[vrpn_SND_CHANNEL_NUM]; 
	char    samplename[100];
	// 
	char filetable[vrpn_SND_MAXFNUM][100];
	char *fileinmem[vrpn_SND_MAXFNUM];
	int  filemax[vrpn_SND_MAXFNUM];
	int  filenum;
	int channel_on[vrpn_SND_CHANNEL_NUM];
	int channel_file[vrpn_SND_CHANNEL_NUM];
	int volume[vrpn_SND_CHANNEL_NUM];
	int play_mode[vrpn_SND_CHANNEL_NUM]; // vrpn_SND_SINGLE, LOOPED or STOP 
	int channel_index[vrpn_SND_CHANNEL_NUM];
	int channel_max[vrpn_SND_CHANNEL_NUM]; //num of chunks chunk size is 512 here
	// for midi
	int midi_flag;
	int midi_data;
	
    void soundplay(void);
    void initchild();
};

#endif  // VRPN_CLIENT_ONLY

//----------------------------------------------------------
//************** Users deal with the following *************

// Open a sound server that is on the other end of a connection
// and send updates to it.  This is the type of sound server that user code will
// deal with.

class vrpn_Sound_Remote: public vrpn_Sound {
  public:
	// The name of the sound device to connect to
	vrpn_Sound_Remote(char *name);

	// This routine calls the mainloop of the connection it's on
	virtual void mainloop(void);

	// Play a sampled sound immediately in both ears at standard volume
	void play_sampled_sound(const char *sound, 
	                   const int volume = 60,
	               	   const int mode = vrpn_SND_SINGLE, 
					   const int ear = vrpn_SND_BOTH, 
					   const int channel = 1);
	void play_stop(const int channel = 1);
	void play_midi_sound(float data);

	vrpn_int32 encode(char *msgbuf,
	       const char *sound,
		   const int volume,
		   const int mode,
		   const int ear,
		   const int channel);
	vrpn_int32 encode(char *msgbuf, int set_mode, int info); // info = channel/data
//	vrpn_int32 encode(char *msgbuf, int set_midi, int data);
	vrpn_int32 encode(char *msgbuf, int set_load, const char *sound);
	
	void preload_sampled_sound(const char *sound);

  protected:
	
};

#define	VRPN_SOUND_H
#endif
