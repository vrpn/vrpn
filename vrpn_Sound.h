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
#define	vrpn_SND_SETINSTR 5 
#define SHUTDOWN -4 

// Base class for a sound server.  Definition
// of remote sound class for the user is at the end.

#define MAXNOTELEN 1024

static char instrlib[393][40] = {"../sbi/accordn.sbi", "../sbi/piano.sbi",
"../sbi/acguit1.sbi", "../sbi/alien.sbi", "../sbi/alien2.sbi", "../sbi/altovio2.sbi",
"../sbi/alotviol.sbi", "../sbi/art1.sbi", "../sbi/art2.sbi", "../sbi/art3.sbi", 
"../sbi/art4.sbi", "../sbi/art5.sbi", "../sbi/b1.sbi", "../sbi/b2.sbi", 
"../sbi/b3.sbi", "../sbi/b4.sbi", "../sbi/b5.sbi", "../sbi/b6.sbi", "../sbi/b7.sbi",
"../sbi/b8.sbi", "../sbi/b9.sbi", "../sbi/b10.sbi", "../sbi/b11.sbi", 
"../sbi/b11.sbi", "../sbi/b12.sbi", "../sbi/b13.sbi", "../sbi/bagpipe1.sbi", 
"../sbi/bagpipe2.sbi", "../sbi/bamba.sbi", "../sbi/banjo.sbi", "../sbi/banjo1.sbi", 
"../sbi/bankmng.sbi", "../sbi/bass.sbi", "../sbi/bass1.sbi", "../sbi/bass2.sbi",
"../sbi/bass4.sbi", "../sbi/bassharp.sbi", "../sbi/basson.sbi", 
"../sbi/bassoon1.sbi", "../sbi/basssyn1.sbi", "../sbi/basstrlg.sbi", 
"../sbi/bbass.sbi", "../sbi/bcymbal.sbi", "../sbi/bdrum.sbi", "../sbi/bdrum1.sbi",
"../sbi/bdrum1a.sbi", "../sbi/bdrum2.sbi", "../sbi/bdrum3.sbi",
"../sbi/belguit2.sbi", "../sbi/bellong.sbi","../sbi/bells.sbi","../sbi/belpiano.sbi",
"../sbi/belshort.sbi", "../sbi/bguit.sbi", "../sbi/bncebass.sbi", 
"../sbi/bpiano.sbi", "../sbi/brass1.sbi", "../sbi/brass2.sbi", "../sbi/brass4.sbi",
"../sbi/brush1.sbi", "../sbi/brush2.sbi", "../sbi/brushes.sbi", "../sbi/brushes.sbi",
"../sbi/bstacc.sbi", "../sbi/bsynth.sbi", "../sbi/btgo.sbi", "../sbi/castanet.sbi",
"../sbi/cbanjo.sbi", "../sbi/cbassoon.sbi", "../sbi/celesta.sbi", 
"../sbi/celguit.sbi", "../sbi/cello.sbi", "../sbi/cello2.sbi", "../sbi/celpiano.sbi",
"../sbi/chirp.sbi", "../sbi/chorn.sbi", "../sbi/clapping.sbi", "../sbi/clar1.sbi", 
"../sbi/clar1a.sbi", "../sbi/clar2.sbi", "../sbi/clarinet.sbi", 
"../sbi/clavecin.sbi", "../sbi/claves.sbi", "../sbi/click.sbi", "../sbi/comput1.sbi",
"../sbi/contra2.sbi", "../sbi/contrab.sbi", "../sbi/cromorne.sbi", 
"../sbi/cstacc.sbi", "../sbi/csynth.sbi", "../sbi/cviolin.sbi", "../sbi/cymb101.sbi",
"../sbi/cymb10m.sbi", "../sbi/cymb10s.sbi", "../sbi/cymb10ss.sbi", 
"../sbi/cymbal.sbi", "../sbi/cymbal1.sbi", "../sbi/cymbal10.sbi", 
"../sbi/cymbal2.sbi", "../sbi/cymcrash.sbi", "../sbi/cymride.sbi", 
"../sbi/dblreed2.sbi", "../sbi/demo.sbi", "../sbi/echimes.sbi", 
"../sbi/echobass.sbi", "../sbi/eg1.sbi", "../sbi/eg2.sbi", "../sbi/eg3.sbi", 
"../sbi/elclav1.sbi", "../sbi/elclav2.sbi", "../sbi/eldrum1.sbi", 
"../sbi/eldrum2.sbi", "../sbi/elecfl.sbi", "../sbi/elecvibe.sbi", 
"../sbi/elguit.sbi", "../sbi/elguit1.sbi", "../sbi/elguit10.sbi", 
"../sbi/elguit11.sbi", "../sbi/elguit1a.sbi", "../sbi/elguit1b.sbi", 
"../sbi/elguit1c.sbi", "../sbi/elguit2.sbi", "../sbi/elguit3.sbi", 
"../sbi/elguit4.sbi", "../sbi/elguit5.sbi", "../sbi/elguit6.sbi", 
"../sbi/elguit7.sbi", "../sbi/elguit8.sbi", "../sbi/elguit9.sbi", 
"../sbi/elguitar.sbi", "../sbi/elorgan1.sbi", "../sbi/elpiano1.sbi", 
"../sbi/elpiano2.sbi", "../sbi/englhrn1.sbi", "../sbi/englhrn2.sbi",
"../sbi/epiano1a.sbi", "../sbi/epiano1b.sbi", "../sbi/fantapan.sbi",
"../sbi/flapbas.sbi", "../sbi/flute.sbi", "../sbi/flute1.sbi", "../sbi/flute2.sbi",
"../sbi/flute2.sbi", "../sbi/frhorn1.sbi", "../sbi/fstrp1.sbi", "../sbi/fstrp2.sbi",
"../sbi/funky.sbi", "../sbi/funnybas.sbi", "../sbi/fuz13l.sbi", "../sbi/fuz13m.sbi",
"../sbi/fuz13s.sbi", "../sbi/fuzgui12.sbi", "../sbi/fuzguit1.sbi", 
"../sbi/fuzguit2.sbi", "../sbi/gong.sbi", "../sbi/guit-sus.sbi", 
"../sbi/guitar1.sbi", "../sbi/guitar2.sbi", "../sbi/guitar3.sbi", 
"../sbi/guitar4.sbi", "../sbi/gun1.sbi", "../sbi/handbell.sbi", "../sbi/harmon1.sbi",
"../sbi/harmonca.sbi", "../sbi/harp.sbi", "../sbi/harp1.sbi", "../sbi/harp2.sbi",
"../sbi/harp3.sbi", "../sbi/harpe1.sbi", "../sbi/harpsi1.sbi", "../sbi/harpsi2.sbi",
"../sbi/harpsi3.sbi", "../sbi/harpsi4.sbi", "../sbi/harpsi5.sbi", 
"../sbi/hartbear.sbi", "../sbi/helico.sbi", "../sbi/helico1.sbi", 
"../sbi/helico2.sbi", "../sbi/helico3.sbi", "../sbi/helicptr.sbi", 
"../sbi/helilow.sbi", "../sbi/hihat1.sbi", "../sbi/hihat2.sbi", "../sbi/hihat3.sbi",
"../sbi/horn1.sbi", "../sbi/horn2.sbi", "../sbi/hvymetl1.sbi", "../sbi/hvymetl2.sbi",
"../sbi/hvymetl2.sbi", "../sbi/hvymetl3.sbi", "../sbi/javaican.sbi",
"../sbi/jazgit10.sbi", "../sbi/jazzbas2.sbi", "../sbi/jazzbass.sbi", 
"../sbi/jazzguit.sbi", "../sbi/jet1.sbi", "../sbi/jet11a.sbi", "../sbi/jet11b.sbi",
"../sbi/jewsharp.sbi", "../sbi/jorgan.sbi", "../sbi/kettle1.sbi", 
"../sbi/keybrd1.sbi", "../sbi/keybrd2.sbi", "../sbi/keybrd3.sbi", 
"../sbi/keybrd9.sbi", "../sbi/koto1.sbi", "../sbi/kuorsau2.sbi", 
"../sbi/kuorsaus.sbi", "../sbi/laser.sbi", "../sbi/laser1.sbi", "../sbi/laser2.sbi",
"../sbi/lngcymbl.sbi", "../sbi/lngsnare.sbi", "../sbi/logdrum1.sbi", 
"../sbi/low.sbi", "../sbi/lullbass.sbi", "../sbi/luola.sbi", "../sbi/marimba.sbi",
"../sbi/marimba1.sbi", "../sbi/marimaba2.sbi", "../sbi/mars.sbi", 
"../sbi/mdrnphon.sbi", "../sbi/meri.sbi", "../sbi/mgun1.sbi", "../sbi/mgun2.sbi",
"../sbi/mgun3.sbi", "../sbi/mltrdrum.sbi", "../sbi/moogsynt.sbi", "../sbi/moon.sbi",
"../sbi/motor1.sbi", "../sbi/noise1.sbi", "../sbi/notbig.sbi", "../sbi/oboe.sbi", 
"../sbi/oboe1.sbi", "../sbi/oboev.sbi", "../sbi/oldpia1.sbi", "../sbi/oldsnar.sbi",
"../sbi/organ1.sbi", "../sbi/organ13.sbi", "../sbi/organ2.sbi", "../sbi/organ2a.sbi",
"../sbi/organ3.sbi", "../sbi/organ3a.sbi", "../sbi/organ3b.sbi", "../sbi/organ4.sbi",
"../sbi/orgnperc.sbi", "../sbi/phone1.sbi", "../sbi/phone2.sbi", "../sbi/phone3.sbi",
"../sbi/pian1a.sbi", "../sbi/pian1b.sbi", "../sbi/pian1c.sbi", "../sbi/aalto.sbi",
"../sbi/piano1.sbi", "../sbi/piano1a.sbi", "../sbi/piano2.sbi", "../sbi/piano3.sbi", 
"../sbi/piano4.sbi", "../sbi/pianobel.sbi", "../sbi/pianof.sbi", 
"../sbi/piccolo.sbi", "../sbi/pipes.sbi", "../sbi/plane1.sbi", "../sbi/plane2.sbi",
"../sbi/plane3.sbi", "../sbi/popbass1.sbi", "../sbi/popbass2.sbi", 
"../sbi/popbass3.sbi", "../sbi/rimshot.sbi", "../sbi/rksnare1.sbi",
"../sbi/rlog.sbi", "../sbi/sax1.sbi", "../sbi/sax2.sbi", "../sbi/saxafone.sbi",
"../sbi/saxunder.sbi", "../sbi/sbsynth.sbi", "../sbi/scratch.sbi", 
"../sbi/scratch4.sbi", "../sbi/sdrum2.sbi", "../sbi/seawave.sbi",
"../sbi/sftbrss1.sbi", "../sbi/sftfl2.sbi", "../sbi/shock1.sbi", 
"../sbi/shock2.sbi", "../sbi/shock3.sbi", "../sbi/shock4.sbi", "../sbi/shot1.sbi",
"../sbi/shot2.sbi", "../sbi/shppizz.sbi", "../sbi/shrtvibe.sbi", "../sbi/siren1.sbi",
"../sbi/siren2a.sbi", "../sbi/siren2b.sbi", "../sbi/sitar.sbi", "../sbi/sitar2.sbi",
"../sbi/snakefl.sbi","../sbi/snare.sbi", "../sbi/snare1.sbi", "../sbi/snare10.sbi",
"../sbi/snrsust.sbi", "../sbi/sfotpizz.sbi", "../sbi/softsax.sbi", 
"../sbi/solovln.sbi", "../sbi/steelg.sbi", "../sbi/steelgt1.sbi", 
"../sbi/steelgt2.sbi", "../sbi/steelgt6.sbi", "../sbi/strings1.sbi", 
"../sbi/strnlon2.sbi", "../sbi/strnlong.sbi", "../sbi/surf.sbi", 
"../sbi/swinth1.sbi", "../sbi/swinth3.sbi", "../sbi/swinth4.sbi", "../sbi/syn1.sbi",
"../sbi/syn12l.sbi", "../sbi/syn12m.sbi", "../sbi/syn12s.sbi", "../sbi/syn13.sbi",
"../sbi/syn2.sbi", "../sbi/syn3.sbi", "../sbi/syn4.sbi", "../sbi/syn5.sbi", 
"../sbi/syn6.sbi", "../sbi/syn7.sbi", "../sbi/syn9.sbi", "../sbi/synbal1.sbi",
"../sbi/synbal2.sbi", "../sbi/synbass.sbi", "../sbi/synbass1.sbi", 
"../sbi/synbass2.sbi", "../sbi/synbass3.sbi", "../sbi/synbass4.sbi", 
"../sbi/synbass5.sbi", "../sbi/synbass6.sbi", "../sbi/synbass8.sbi",
"../sbi/synsnr1.sbi", "../sbi/synsnr2.sbi", "../sbi/synth1.sbi", "../sbi/synth2.sbi",
"../sbi/tensax.sbi", "../sbi/tensax2.sbi", "../sbi/tensax3.sbi", 
"../sbi/tensax4.sbi", "../sbi/test.sbi", "../sbi/test1.sbi", "../sbi/tincan1.sbi",
"../sbi/tom.sbi", "../sbi/tom1.sbi", "../sbi/tom10.sbi", "../sbi/tom11.sbi", 
"../sbi/tom2.sbi", "../sbi/tom3.sbi", "../sbi/train1.sbi", "../sbi/train2.sbi", 
"../sbi/trainbel.sbi", "../sbi/trangle.sbi", "../sbi/tromb1.sbi", 
"../sbi/tromb2.sbi", "../sbi/tromb3.sbi", "../sbi/trombone.sbi", "../sbi/trombv.sbi",
"../sbi/trpcup1.sbi", "../sbi/trumpet.sbi", "../sbi/trumpet1.sbi",
"../sbi/trumpet2.sbi", "../sbi/trumpet3.sbi", "../sbi/trumpet4.sbi", 
"../sbi/tuba1.sbi", "../sbi/tuba2.sbi", "../sbi/tuba3.sbi", "../sbi/ufo1.sbi", 
"../sbi/ufo2.sbi", "../sbi/vibra1.sbi", "../sbi/vibra2.sbi", "../sbi/vibra3.sbi", 
"../sbi/violin.sbi", "../sbi/violin1.sbi", "../sbi/violin2.sbi", 
"../sbi/violin3.sbi", "../sbi/vlnpizz.sbi", "../sbi/vlnpizz.sbi", 
"../sbi/vlnpizz1.sbi", "../sbi/wave.sbi", "../sbi/wave1.sbi", "../sbi/wave2.sbi",
"../sbi/wood1.sbi",  "../sbi/xylo1.sbi", "../sbi/xylo2.sbi", "../sbi/xylo3.sbi", 
"../sbi/xylofone.sbi", "../sbi/ykabass1.sbi"};

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
    int pack_command(int sound_type, int play_mode, int ear_mode, 
			char *notebuff);	
    int pack_command(int set_stop);
    int pack_command(int command, int *instrname, int num);
    set_instr(void);

  protected:
	int     childpid, pipe1[2], pipe2[2]; 

  	int     status; // playing or idle which will be used by child process
                    // to dertermine stopping the playing sound
	int	sound_type; // most-recent sound type 
	int  	play_mode; // vrpn_SND_SINGLE, LOOPED or STOP 
	int 	ear_mode; // mono or stereo
        int   volume;

        // for sample sound
        char    samplename[100]; // sample sound filename

        // for midi music
	int 	instr_ID[128];
	int 	num_instr;
	char    notebuff[MAXNOTELEN]; //music note buffer
   	int  	notelen; // len of notebuff

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
	play_sampled_sound(char *sound, int volume = 60,
		int mode = vrpn_SND_SINGLE, int ear = vrpn_SND_BOTH);
	play_stop();

	encode(char *msgbuf, char *sound, int volume, int mode, int ear);
	encode(char *msgbuf, char *notes, int mode, int ear);
	encode(char *msgbuf, int set_stop);
	encode(char *msgbuf, int num, int *instrids);
	
	set_midi_instr(int num, int *instrname);
	// Play a segment of midi notes immediately in both ears at standard
        // volume
	play_midi_music(char *notes, 
                int mode = vrpn_SND_SINGLE, int ear = vrpn_SND_BOTH);

	//XXX preload_sampled_sound(char *sound);

  protected:
	
};

#define	VRPN_SOUND_H
#endif
