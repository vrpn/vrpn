#ifndef	VRPN_SOUND_H
#include "vrpn_Connection.h"

static	int	vrpn_SND_SINGLE = 0;
static	int	vrpn_SND_LOOPED = 1;
static	int	vrpn_SND_STOP = 2;

static	int	vrpn_SND_LEFT = 1;
static	int	vrpn_SND_RIGHT = 2;
static	int	vrpn_SND_BOTH = 3;

// Base class for a sound server.  Definition
// of remote sound class for the user is at the end.

class vrpn_Sound {
  public:
	vrpn_Sound(): connection(NULL) {};

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

  protected:
	int	status;
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
	play_sampled_sound(char *sound, float volume = 1.0,
		int mode = vrpn_SND_SINGLE, int ear = vrpn_SND_BOTH);
	//XXX preload_sampled_sound(char *sound);

  protected:
};

#define	VRPN_SOUND_H
#endif
