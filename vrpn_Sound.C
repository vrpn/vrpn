#include <time.h>
#include <math.h>
#ifdef __STDC__
#include <stdlib.h>
#else
#include <getopt.h>
#endif
#include <stdio.h>
#include <string.h>
#ifdef linux
#include <termios.h>
#define  FDEFER 0x00040
#include <sys/soundcard.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <netinet/in.h>
#include "vrpn_Sound.h"

#define BUF_SIZE 4096

int audio_fd; // File descriptor for audio device /dev/audio
unsigned char audio_buffer[BUF_SIZE]; // audio buffer
#ifdef linux
struct sbi_instrument instr; // MIDI
#endif
static int sb, sbptr = 0;
unsigned char sbbuf[404];
static int tag = 0;
static char command_buffer[1024];

void sbflush()
{
        if (!sbptr) return;

        if (write(sb, sbbuf, sbptr) == -1)
        {
                perror("write sb");
                exit(-1);
        }

        sbptr=0;
}

void sbwrite(char *msg)
{
        if (sbptr>400) sbflush();

        memcpy(&sbbuf[sbptr], msg, 4);
        sbptr +=4;
}

void midich(char c)
{
        char buf[4];
        buf[0] = 5;
        buf[1] = c;
        sbwrite(buf);
}

void noteon(int chan,int pitch,int vol)
{
#ifdef linux
        char buf[4];
#ifdef DEBUG
        printf("Note on, chan=%d pitch=%d vol=%d\n",chan+1,pitch,vol);
#endif

        if (chan >= 0)
        {
                buf[0] = SEQ_FMNOTEON;
                buf[1] = chan;
                buf[2] = pitch;
                buf[3] = vol;
                sbwrite(buf);
        } else {
                midich(0x90+chan);
                midich(pitch);
                midich(vol);
        }
#endif
}

void noteoff(int chan,int pitch,int vol)
{
#ifdef linux
        char buf[4];
#ifdef DEBUG
        printf("Note off, chan=%d pitch=%d vol=%d\n",chan+1,pitch,vol);
#endif

        if (chan >= 0)
        {
                buf[0] = SEQ_FMNOTEOFF;
                buf[1] = chan;
                buf[2] = pitch;
                buf[3] = vol;
                sbwrite(buf);
        } else {
                midich(0x80+chan);
                midich(pitch);
                midich(vol);
        }
#endif
}

void wait(int delay)
{
#ifdef linux
        int jiffies;
        jiffies = (delay << 8) | SEQ_WAIT;
        sbwrite((char*)&jiffies);
#endif
}

vrpn_Linux_Sound::set_instr(void)
{
#ifdef linux
        char buf1[100];
        int dev, nrdevs;
        struct synth_info info;

        int l, i, op, finstrf;
        if ((sb=open("/dev/sequencer", O_WRONLY, 0))==-1)
        {
                perror("/dev/sequencer");
                exit(-1);
        }

        if (ioctl(sb, SNDCTL_SEQ_NRSYNTHS, &nrdevs) == -1)
        {
                perror("/dev/sequencer");
                exit(-1);
        }

        dev = -1;

        for (i=0;i<nrdevs && dev==-1;i++)
        {
                info.device = i;
                if (ioctl(sb, SNDCTL_SYNTH_INFO, &info)==-1)
                {
                        perror("info: /dev/sequencer");
                        exit(-1);
                }

                if (info.synth_type == SYNTH_TYPE_FM) dev = i;
        }

        if (dev == -1)
        {
                fprintf(stderr, "FM synthesizer not detected\n");
                exit(-1);
        }
        for (op=0;op < num_instr;op++) {
                if ((finstrf=open(instrlib[instr_ID[op]], O_RDONLY, 0))==-1) {
                        perror(instrlib[op]);
                } else if ((l=read(finstrf, buf1, 100))==-1) {
                        perror(instrlib[op]);
                } else if (buf1[0]!='S' || buf1[1]!='B' || buf1[2]!='I') {
                        fprintf(stderr,"Not SBI file\n");
                } else if (l<51) {
                        fprintf(stderr,"Short file\n");
                } else {
                        instr.channel = op;
                        instr.key = FM_PATCH;
                        instr.device = dev;

                        for (i=0;i<16;i++) {
                                instr.operators[i]=buf1[i+0x24];
                        }

                		if (write(sb, &instr, sizeof(instr))==-1) 
						perror("/dev/sequencer");
				 }
                 close(finstrf);
        }
		close(sb);
#endif
}

// This function is for child process. It will read from the pipe and see if there 
// is any command from the parent to be processed. When it recieves the message,
// it will decode it and set the corresponding variables
int vrpn_Linux_Sound::checkpipe(void)
{
#ifdef linux
	// read command from pipe1
	char cbuff[256], rest[256], tmp;
	int m, n, t;
        // read the command len first
	fcntl(pipe1[0], F_SETFL, O_NONBLOCK);
	if ( read(pipe1[0], &tmp, 1) == -1) 
		return 0;
        // try to read the n number of char
	fcntl(pipe1[0], F_SETFL, FDEFER);
	n = (int) tmp;
	t = 0;
	while ( n > 0 && (m = read(pipe1[0], &cbuff[t], n+2)))
	{
		if ( m < 0) 
		{
			fprintf(stderr, "pipe read error");
			exit(0);
		}
		n = n - m;
		t = t + m;
	}
	if (t==0) return 0;
    cbuff[t] = '\0';

	// here we assume that the command message have the fixed format
	// 0: command 
        // 1: play_mode                           / number of instrments
        // 2: ear_mode                            / instrment No. 2~
		// 3: volume
        // 4: filename length / notebuf length
        // 5~: filename / content of notebuf
	switch (cbuff[0]) {
		case vrpn_SND_SAMPLE:
			play_mode = cbuff[1];
			ear_mode = cbuff[2];
			volume = cbuff[3];
			//cbuff[5] is used for channel #
			strncpy(samplename, &cbuff[6], (int) cbuff[4]-1);
	    	samplename[(int) cbuff[4]-1] = '\0';
			status = vrpn_SND_SAMPLE;
			{
				int bar, baz;
				char devname[30] = "/dev/mixer";
				if ((baz = open(devname,O_RDWR)) < 0){
					perror(devname);
					exit(1);
				}
				bar = volume;
				if (ioctl(baz, MIXER_WRITE(0), &bar) == -1)
					fprintf(stderr, "can't set volume\n");
				close(baz);
			}
			break;
		case vrpn_SND_MIDI:
			play_mode = cbuff[1];
			ear_mode = cbuff[2];
			strncpy(notebuff, &cbuff[4], (int) cbuff[3]);
			notebuff[(int) cbuff[3]] = '\0';
			status = vrpn_SND_MIDI;
			break;
		case vrpn_SND_STOP:
			status = vrpn_SND_STOP;
			break;
		case vrpn_SND_WAVEFORM:
			break;
		case vrpn_SND_SETINSTR:
			status = vrpn_SND_SETINSTR;
			num_instr = (int) cbuff[1];
			for (int i=0; i < num_instr; i++)
			{
				instr_ID[i] = (int) cbuff[i+2];
			}
			set_instr();
			status = vrpn_SND_STOP;
			break;
		case SHUTDOWN:
			return 1;	
	}
#endif
	return 0;
}
sndinit(int chan)
{
#ifdef linux
		char buf[100];
        buf[0] = SEQ_FMPGMCHANGE;
        buf[1] = chan;       /* program voice channel to */
        buf[2] = chan;   /* either play default instrument 0 */
                      /*buf[2] = num_instr;or play requested instrument */
        sbwrite(buf);
#endif
}
vrpn_Linux_Sound::soundplay()
{
#ifdef linux
	// the local variables for sampled sound
    int fd, n, speed, stereo;
    int format = AFMT_MU_LAW;
    // the local variables for MIDI
    char buf[100];
    int dev, nrdevs;
    struct synth_info info;
    int l, i, j, op;
    int instr_num;
    int channel;
	int num_channel, octave = 3, time = 50, volume = 90;
	static int audio_status = 0;

	switch (status) {
	case vrpn_SND_SAMPLE:
		if (play_mode == vrpn_SND_SINGLE)
			status = vrpn_SND_STOP;
		else if (play_mode == vrpn_SND_LOOPED)
			status = vrpn_SND_SAMPLE;
        // play the sample sound on /dev/audio
		if (audio_status == 0) {
		audio_status = 1;
		if ((audio_fd = open("/dev/audio", O_WRONLY, 0)) == -1)
		{
			perror("/dev/audio");
			fprintf(stderr, "can't open device /dev/audio\n");
			exit(0);
		} 
		if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format) == -1)
			fprintf(stderr, "can't set audio format\n");
		format = AFMT_QUERY;
		if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &format)==-1)
        {
            fprintf(stderr, "can't test the format\n");
        }
        stereo = ear_mode;
		if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo)==-1)
            fprintf(stderr, "can't set the stereo mode\n");
        speed = 8192;
        if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed)==-1)
            fprintf(stderr, "can't set the speed\n");
		}
        fd = open(samplename, O_RDONLY, 0);
       // n = read(fd,  audio_buffer, BUF_SIZE);
        n = read(fd,  audio_buffer, 512);
        write(audio_fd, audio_buffer, n);
        while (n>0)
        {
            n = read(fd, audio_buffer, BUF_SIZE);
            write(audio_fd, audio_buffer, n);
        }
        close(fd);
		break;
	case vrpn_SND_MIDI:
		// set up the volume
			{
				int bar, baz;
				char devname[30] = "/dev/mixer";
				if ((baz = open(devname,O_RDWR)) < 0){
					perror(devname);
					exit(1);
				}
				bar = 90;
				if (ioctl(baz, MIXER_WRITE(0), &bar) == -1)
					fprintf(stderr, "can't set volume\n");
				close(baz);
			}
	 	if (play_mode == vrpn_SND_SINGLE)
            status = vrpn_SND_STOP;
        else if (play_mode == vrpn_SND_LOOPED)
            status = vrpn_SND_MIDI;
        // play the music note on /dev/sequencer
	if ((sb= open("/dev/sequencer", O_WRONLY, 0))==-1)
	{
		perror("/dev/sequencer");
	}
	num_channel = (int) notebuff[0];
    sndinit(0);	
	// play the music according to the notes in the buffer 
		for (i = 0; i < num_channel; i++)
		    for (j = 0; j < notebuff[notebuff[i+1] + 3]; j++)
			{
				noteon(notebuff[notebuff[i+1]] -1, notebuff[notebuff[i+1]+6+
				 j*3]+12 * notebuff[notebuff[i+1]+4+3*j],
				 notebuff[notebuff[i+1]+5+3*j]);
				 wait(notebuff[notebuff[i+1] + 1] + 
				      (j+1) * time * notebuff[notebuff[i+1]+2]);
				noteoff(notebuff[notebuff[i+1]]-1, notebuff[notebuff[i+1]+6+
				 j*3]+12 * notebuff[notebuff[i+1]+4+3*j],
				 notebuff[notebuff[i+1]+5+3*j]);
			}
		sbflush();
		close(sb);
		break;
	case vrpn_SND_STOP:
	    if (audio_status == 1)
        close(audio_fd);
		audio_status = 0;
		break;
	case vrpn_SND_WAVEFORM:
		break;
	default: 
		break;
	}	
#endif
}

vrpn_Linux_Sound::initchild()
{
	status = vrpn_SND_STOP;
	volume = 60;
	// the following sets the default midi instr
	num_instr = 0;
}

vrpn_Linux_Sound::decode(char *msgbuf)
{
	tag = 0;
	sound_type = (int) command_buffer[0];
	if (sound_type == vrpn_SND_SAMPLE) {
	volume = (int) command_buffer[1];
	play_mode = (int) command_buffer[2];
	ear_mode = (int) command_buffer[3];
	strncpy(samplename, &command_buffer[4], strlen(command_buffer) - 4);
	samplename[strlen(command_buffer) - 4] = '\0';}
	else if (sound_type == vrpn_SND_MIDI) {
	play_mode = (int) command_buffer[1];
	ear_mode = (int) command_buffer[2];
	strncpy(notebuff, &command_buffer[3], strlen(command_buffer) - 3);
	notebuff[strlen(command_buffer) - 3] = '\0';
	} 
	else if (sound_type == vrpn_SND_SETINSTR) 
	{
		num_instr = command_buffer[1];
		for (int i=0; i< num_instr; i++)
			instr_ID[i] = (int) command_buffer[i+2];
	} else if (sound_type != vrpn_SND_STOP)
			fprintf(stderr, "Unknown command from user. \n");
	status = sound_type;
}
int sound_handler(void *userdata, vrpn_HANDLERPARAM p)
{
#ifdef linux
	// in this function, it will get the command from the Remote
	int i;
	unsigned long *longbuf = (unsigned long*)(void *)(p.buffer);
	for (i=0; i< p.payload_len/4; i++)
	{
		command_buffer[i] = (char) ntohl(longbuf[i]);
	}
	command_buffer[p.payload_len/4] = '\0';
	tag = 1;
#endif
	return 0;
}

vrpn_Linux_Sound::vrpn_Linux_Sound(char *name, vrpn_Connection *c):vrpn_Sound(c)
{
#ifdef linux
    vrpn_MESSAGEHANDLER Myhandler;
	Myhandler = sound_handler;
	if (connection) {
	    my_id = connection->register_sender(name);
	    playsample_id = connection->register_message_type("play_sample");
            // install a handler for sound 
	    connection->register_handler(playsample_id, Myhandler, NULL, my_id);
	}
	sound_type = -1; // No music type has been set
	play_mode = vrpn_SND_SINGLE; //init as single mode
	ear_mode = vrpn_SND_BOTH; // init as stereo mode

        notelen = 0;

        // Create Child Process which communicate through pipe with the parent
	// process. It will listen on the pipe for the command and control the 
        // sound device and play the sound.

        {
	if (pipe(pipe1) < 0 || pipe(pipe2) < 0)
	{
		fprintf(stderr, "can't create pipes");	
		exit(1);
	}
	if ((childpid = fork()) < 0)
	{
		fprintf(stderr, "can't fork child process");
		exit(1);
	} else if (childpid > 0) {   //parent
		close(pipe1[0]);
		close(pipe2[1]);
	} else {
		close(pipe1[1]); // currently we only use the pipe1 
                                 // message only goes from parent to child
		close(pipe2[0]);
		initchild();
		while(1)
		    if (checkpipe()) 
			break;	
		    else soundplay();
		close(pipe1[0]);
		close(pipe2[1]);
		exit(0);
        }
	}
#endif
}


int vrpn_Linux_Sound::pack_command(int sound_type, int play_mode, int ear_mode,
					int volume,char *samplename1)
{
	char box[1024];
	// box[0] is the total bytes transfered, so starting from box[1]
	box[1] = sound_type;
	box[2] = play_mode;
	box[3] = ear_mode;
	box[4] = volume;
	box[5] = (char) strlen(samplename1);
	strncpy(&box[6], samplename1, (int) box[5]);
	box[6+(int)box[5]] = '\0';
	box[0] = 6 + (int) box[5];
	write(pipe1[1], box, box[0]+1);
	return 1;
}

int vrpn_Linux_Sound::pack_command(int sound_type, int play_mode, int ear_mode,
					char *notebuff)
{
	char box[1024];
	box[1] = sound_type;
	box[2] = play_mode;
	box[3] = ear_mode;
	box[4] = (char) strlen(notebuff);
	strncpy(&box[5], notebuff, (int) box[4]);
	box[0] = 5 + (int) box[4];
	for ( int i = 0; i< box[0]; i++)
	write(pipe1[1], box, box[0] + 1);
	return 1;
}

int vrpn_Linux_Sound::pack_command(int set_stop)
{
	char box[1024];
	box[1] = sound_type;
	box[2] = '\0';
	box[0] = 2;
	write(pipe1[1], box, box[0]+1);
	return 1;
}

int vrpn_Linux_Sound::pack_command(int command, int *instrids, int num)
{
	char box[1024];
	box[1] = sound_type;
	box[2] = num;
	for (int i = 0; i < num; i++)
		box[i+3] = instrids[i];
	box[3 + num] = '0';
	box[0] = 3 + num;
	write(pipe1[1], box, box[0]+1);
	return 1;
}

void vrpn_Linux_Sound::mainloop(void)
{
	char msgbuf[4096];

	if (tag) {
	decode(command_buffer);
	switch(status) {
		case vrpn_SND_SAMPLE: // for sampled sound using /dev/audio 
		 pack_command(sound_type, play_mode, ear_mode, volume, samplename);
		 break;
		case vrpn_SND_MIDI: // for midi sound using /dev/sequencer 
		 pack_command(sound_type, play_mode, ear_mode, notebuff); 
		 break;
		case vrpn_SND_STOP: // stop the currently playing audio
		 pack_command(vrpn_SND_STOP);
		 break;
		case vrpn_SND_WAVEFORM:
		 break;
		case vrpn_SND_SETINSTR:
		 pack_command(vrpn_SND_SETINSTR, instr_ID, num_instr);
		 break;
		default:
		 break;
	} }
	return;
}
// The following are the part the users are concerned with

vrpn_Sound_Remote::vrpn_Sound_Remote(char *name)
{

	//Establish the connection
	if ((connection = vrpn_get_connection_by_name(name)) == NULL) {
		return;
	}

	//Register the sound device and the needed sample id
	my_id = connection->register_sender(name);
	playsample_id = connection->register_message_type("play_sample");
}

void vrpn_Sound_Remote::mainloop(void)
{
	if (connection) { connection->mainloop(); }
}
// this function is used to change all the information to a msg
// the format used here conforms to the one defined in checkpipe()
int vrpn_Sound_Remote::encode(char *msgbuf, char *sound, int volume, 
				int mode, int ear, int channel)
{
	int i;
	unsigned long *longbuf = (unsigned long*) (void*)(msgbuf);
	int index = 0;
	longbuf[index++] = vrpn_SND_SAMPLE;
	longbuf[index++] = volume;
	longbuf[index++] = mode;
	longbuf[index++] = ear;
	longbuf[index++] = channel;
	for ( i = 0; i< strlen(sound); i++)
		longbuf[index++] = sound[i];
	for ( i = 0; i< index; i++) 
		longbuf[i] = htonl(longbuf[i]);
	return index*sizeof(unsigned long);
}
int vrpn_Sound_Remote::encode(char *msgbuf, char *notes, int mode, int ear)
{
	int i;
	unsigned long *longbuf = (unsigned long*) (void*)(msgbuf);
	int index = 0;
	longbuf[index++] = vrpn_SND_MIDI;
	longbuf[index++] = mode;
	longbuf[index++] = ear;
	for ( i = 0; i< strlen(notes); i++)
		longbuf[index++] = notes[i];
	for ( i = 0; i< index; i++) 
		longbuf[i] = htonl(longbuf[i]);
	return index*sizeof(unsigned long);
}

int vrpn_Sound_Remote::encode(char *msgbuf, int set_stop)
{
	int i;
	unsigned long *longbuf = (unsigned long*) (void*)(msgbuf);
	int index = 0;
	longbuf[index++] = vrpn_SND_STOP;
	longbuf[0] = htonl(longbuf[0]);
	return index*sizeof(unsigned long);
}
int vrpn_Sound_Remote::encode(char *msgbuf, int num, int *instrids)
{
	int i;
	unsigned long *longbuf = (unsigned long*) (void*)(msgbuf);
	int index = 0;
	longbuf[index++] = vrpn_SND_SETINSTR;
	longbuf[index++] = num;
	for (i=0; i<num;i++)
		longbuf[index++] = instrids[i];
	for ( i = 0; i< index; i++) 
		longbuf[i] = htonl(longbuf[i]);
	return index*sizeof(unsigned long);
}

vrpn_Sound_Remote::play_sampled_sound(char *sound, 
                                      const int volume, 
                                      const int mode, 
									  const int ear,
									  const int channel)
{
	struct timeval current_time;
	//pack the message and send it to the sound server through connection
	char msgbuf[1024];
	int i, len;

	gettimeofday(&current_time, NULL);
	
	if (connection) {
	    len = encode(msgbuf, sound, volume, mode, ear, channel);
		if (connection->pack_message(len, current_time, playsample_id, my_id, 
		  msgbuf, vrpn_CONNECTION_RELIABLE))
		  	fprintf(stderr, "can't write message\n");
	}
}
vrpn_Sound_Remote::play_stop(const int channel)
{
	struct timeval current_time;
	char msgbuf[1024];
	int len;

	gettimeofday(&current_time, NULL);
	
	if (connection) {
	    len = encode(msgbuf, vrpn_SND_STOP);
		if (connection->pack_message(len, current_time, playsample_id, my_id, 
		  msgbuf, vrpn_CONNECTION_RELIABLE))
		  	fprintf(stderr, "can't write message\n");
	}
}
vrpn_Sound_Remote::set_midi_instr(int num, int *instrname)
{
	struct timeval current_time;
	char msgbuf[1024];
	int len;

	gettimeofday(&current_time, NULL);
	
	if (connection) {
	    len = encode(msgbuf, num, instrname);
		if (connection->pack_message(len, current_time, playsample_id, my_id, 
		  msgbuf, vrpn_CONNECTION_RELIABLE))
		  	fprintf(stderr, "can't write message\n");
	}
}
vrpn_Sound_Remote::play_midi_music(char *notes, int mode, int ear)
{
	struct timeval current_time;
	char msgbuf[1024];
	int len;

	gettimeofday(&current_time, NULL);
	
	if (connection) {
	    len = encode(msgbuf, notes, mode, ear);
		if (connection->pack_message(len, current_time, playsample_id, my_id, 
		  msgbuf, vrpn_CONNECTION_RELIABLE))
		  	fprintf(stderr, "can't write message\n");
	}
}
