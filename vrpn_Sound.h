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
#define SHUTDOWN -4 

// Base class for a sound server.  Definition
// of remote sound class for the user is at the end.
#define CHANNEL_NUM 5 
#define MAXFNUM 20

class vrpn_Sound {
  public:
	vrpn_Sound(vrpn_Connection *c = NULL): connection(c) {};

	// Called once through each main loop iteration to handle
	// updates.
	virtual void mainloop(void) = 0;	// Hear about play requests

  protected:
	vrpn_Connection *connection;
	long my_id;			// ID of this soundserver to connection
	long playsample_id;		// ID of "play sample" message
};

// Sound server running under Linux.

class vrpn_Linux_Sound: public vrpn_Sound {
  public:
	// Open the sound devices connected to the local machine, talk to the
	// outside world through the connection.
	vrpn_Linux_Sound(char *name, vrpn_Connection *connection);
	decode(char *msgbuf);
	int checkpipe();
	virtual void mainloop(void);
	// just for test, will be moved to protected part after done
    int pack_command(int sound_type, int play_mode, int ear_mode, 
			int volume, char *samplename);	
    int pack_command(int set_stop, int channel);
    int pack_command(int set_load, char* sound);
	int mapping(char *name, int address);

  protected:
	int     childpid, pipe1[2], pipe2[2]; 

  	int     status; // playing or idle which will be used by child process
                    // to dertermine stopping the playing sound
	int	sound_type; // most-recent sound type 
	int 	ear_mode; // mono or stereo

        // for sample sound
	int   	fd[CHANNEL_NUM]; 
	char    samplename[100];
	// 
	char filetable[MAXFNUM][100];
	signed char *fileinmem[MAXFNUM];
	int  filemax[MAXFNUM];
	int  filenum;
	int channel_on[CHANNEL_NUM];
	int channel_file[CHANNEL_NUM];
    int     volume[CHANNEL_NUM];
	int  	play_mode[CHANNEL_NUM]; // vrpn_SND_SINGLE, LOOPED or STOP 
	int channel_index[CHANNEL_NUM];
	int channel_max[CHANNEL_NUM]; //num of chunks chunk size is 512 here
	
    soundplay(void);
    int initchild();
};

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

	int encode(char *msgbuf,
	       const char *sound,
		   const int volume,
		   const int mode,
		   const int ear,
		   const int channel);
	int encode(char *msgbuf, int set_stop, int channel);
	int encode(char *msgbuf, int set_load, const char *sound);
	
	void preload_sampled_sound(const char *sound);

  protected:
	
};

#define	VRPN_SOUND_H
#endif
