#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef __STDC__
#ifndef	hpux
#ifndef	_WIN32
#include <getopt.h>
#endif
#endif
#endif

#ifdef linux
#include <termios.h>
#define  FDEFER 0x00040
#include <sys/soundcard.h>
#endif

#ifdef	_WIN32
#include <io.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#ifndef	_WIN32
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#endif
#include <fcntl.h>
#include <ctype.h>
#include "vrpn_Sound.h"

#define BUF_SIZE 4096

int audio_fd; // File descriptor for audio device /dev/audio
char audio_buffer[512]; // audio buffer
#ifdef linux
struct sbi_instrument instr; // MIDI
static int sb, sbptr = 0;
#endif
unsigned char sbbuf[404];

#ifdef	linux
static char command_buffer[1024];
static int my_sound_type;
static int my_volume;
static int my_play_mode;
static int my_ear_mode;
char my_samplename[100];
static int my_status;
int childpid, pipe1[2], pipe2[2];
#endif

#ifdef	linux

static void sbflush()
{
        if (!sbptr) return;

        if (write(sb, sbbuf, sbptr) == -1)
        {
                perror("write sb");
                exit(-1);
        }

        sbptr=0;
}

static void sbwrite(char *msg)
{
        if (sbptr>400) sbflush();

        memcpy(&sbbuf[sbptr], msg, 4);
        sbptr +=4;
}

static void midich(char c)
{
        char buf[4];
        buf[0] = 5;
        buf[1] = c;
        sbwrite(buf);
}

static void noteon(int chan,int pitch,int vol)
{
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
}

static void noteoff(int chan,int pitch,int vol)
{
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
}

static void wait(int delay)
{
        int jiffies;
        jiffies = (delay << 8) | SEQ_WAIT;
        sbwrite((char*)&jiffies);
}
#endif

int vrpn_Linux_Sound::mapping(char *name, int address)
{
	int i, fd, n, t;
	for (i=0; i< filenum; i++)
	   if (strcmp(name, filetable[i]) == 0) 
	  	 { channel_max[address] = filemax[i];
		   return i;
		 }
	if (filenum < MAXFNUM) 
	{
		strcpy(filetable[filenum], name);
		filetable[filenum][strlen(name)] = '\0';
		fd = open(name, O_RDONLY, 0);
		printf("fd = %d\n", fd);
		t = 0;
		fileinmem[filenum] = (char *) calloc(327680, sizeof(char));
		do {
			n = read(fd, &fileinmem[filenum][t*512], 512);
			t++;
		} while ( n>0 && t < 640);
		filemax[i] = t;
		channel_max[address] = t;
		close(fd);
	}	
	filenum++;
	return filenum - 1;
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
	while ( n > 0 && (m = read(pipe1[0], &cbuff[t], n)))
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
			play_mode[cbuff[5]] = cbuff[1];
			//	ear_mode = cbuff[2];
			volume[cbuff[5]] = cbuff[3];
			//cbuff[5] is used for channel #
			channel_on[cbuff[5]] = 1;
			strncpy(samplename, &cbuff[6], (int) cbuff[4]-1);
	    	samplename[(int) cbuff[4]-1] = '\0';
			channel_index[cbuff[5]] = 100;
			channel_file[cbuff[5]] = 
			    mapping(samplename, cbuff[5]);
			break;
		case vrpn_SND_STOP:
			channel_on[cbuff[1]] = 0;	
			break;
		case vrpn_SND_PRELOAD:
			strncpy(samplename, &cbuff[2], (int) cbuff[1]);
	    	samplename[(int) cbuff[1]] = '\0';
			printf("%s\n", samplename);
			mapping(samplename, 0);
			break;
		case SHUTDOWN:
			return 1;	
	}
#endif
	return 0;
}

#ifdef linux
static void sndinit(int chan)
{
	char buf[100];
        buf[0] = SEQ_FMPGMCHANGE;
        buf[1] = chan;       /* program voice channel to */
        buf[2] = chan;   /* either play default instrument 0 */
                      /*buf[2] = num_instr;or play requested instrument */
        sbwrite(buf);
}
#endif

void vrpn_Linux_Sound::soundplay()
{
#ifdef linux
	// the local variables for sampled sound
    int i, j;

	checkpipe();
    // play the sample sound on /dev/audio
	for (i=0; i < 512; i++)
		audio_buffer[i] = 0;
	for (i=1; i < CHANNEL_NUM; i++)
		if (channel_on[i]) {
			for ( j = 0; j < 512; j++)
				audio_buffer[j] += ((char) ( 
				fileinmem[channel_file[i]][channel_index[i]*512 + j] 
				* 1.0 * volume[i] /100)>>1);
			channel_index[i] = 
			(channel_index[i] + 1 > channel_max[i]) ? 100 : channel_index[i]+1;
			if ((play_mode[i] == vrpn_SND_SINGLE) && channel_index[i] ==100)
				 	channel_on[i] = 0;
		}
    write(audio_fd, audio_buffer, 512); 
#endif
}

void vrpn_Linux_Sound::initchild()
{
#ifdef	linux
	int i, stereo, speed;
	int format = AFMT_S16_LE;

	filenum = 0;
	for (i=0; i<CHANNEL_NUM; i++)
	{
		channel_on[i] = 0;
		channel_max[i] = 0;
	}
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
    speed = 8192*2;
    if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &speed)==-1)
        fprintf(stderr, "can't set the speed\n");
    {
		int bar, baz;
		char devname[30] = "/dev/mixer";
		if ((baz = open(devname,O_RDWR)) < 0){
			perror(devname);
			exit(1);
		}
		bar = 100;
		if (ioctl(baz, MIXER_WRITE(0), &bar) == -1)
			fprintf(stderr, "can't set volume\n");
		close(baz);
	}
#endif
}

#ifdef	linux
static void decode()
{
	my_sound_type = (int) command_buffer[0];
	if (my_sound_type == vrpn_SND_SAMPLE) {
	my_volume = (int) command_buffer[1];
	my_play_mode = (int) command_buffer[2];
	my_ear_mode = (int) command_buffer[3];
	strncpy(my_samplename, 
	        &command_buffer[4], strlen(command_buffer) - 4);
	my_samplename[strlen(command_buffer) - 4] = '\0';}
	else if (my_sound_type == vrpn_SND_STOP)
			my_volume = (int) command_buffer[1];
	else if (my_sound_type == vrpn_SND_PRELOAD)
	{
	strncpy(my_samplename, 
	        &command_buffer[1], strlen(command_buffer) - 1);
	my_samplename[strlen(command_buffer) - 1] = '\0';}
	my_status = my_sound_type;
}

static int pack_command(int sound_type, int play_mode, int ear_mode,
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

static int pack_command(int set_stop, int channel)
{
	char box[1024];
	box[1] = set_stop;
	box[2] = channel;
	box[3] = '\0';
	box[0] = 3;
	write(pipe1[1], box, box[0]+1);
	return 1;
}

static int pack_command(int set_load, char *sound)
{
	char box[1024];
	box[1] = set_load;
	box[2] = (char) strlen(sound);
	strncpy(&box[3], sound, (int) box[2]);
	box[3+(int)box[2]] = '\0';
	box[0] = 3 + (int) box[2];
	write(pipe1[1], box, box[0]+1);
	return 1;
}

static int sound_handler(void *userdata, vrpn_HANDLERPARAM p)
{
	// in this function, it will get the command from the Remote
	int i;
	unsigned long *longbuf = (unsigned long*)(void *)(p.buffer);
	for (i=0; i< p.payload_len/4; i++)
	{
		command_buffer[i] = (char) ntohl(longbuf[i]);
	}
	command_buffer[p.payload_len/4] = '\0';
	decode();
	switch(my_status) {
		case vrpn_SND_SAMPLE: // for sampled sound using /dev/audio 
		 pack_command(my_sound_type, my_play_mode, my_ear_mode, 
		             my_volume, my_samplename);
		 break;
		case vrpn_SND_STOP: // stop the currently playing audio
		 pack_command(vrpn_SND_STOP, my_volume);
		 break;
		case vrpn_SND_PRELOAD:
		 pack_command(vrpn_SND_PRELOAD, my_samplename);
		 break;
		default:
		 break;
	} 
	return 0;
}
#endif

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
	play_mode[0] = vrpn_SND_SINGLE; //init as single mode
	ear_mode = vrpn_SND_BOTH; // init as stereo mode

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
		    soundplay();
		close(pipe1[0]);
		close(pipe2[1]);
		exit(0);
        }
	}
#else
	name = name;	// Keep the compiler happy
#endif
}



void vrpn_Linux_Sound::mainloop(void)
{
}
// The following are the part the users are concerned with

#ifndef	_WIN32
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
#endif

void vrpn_Sound_Remote::mainloop(void)
{
	if (connection) { connection->mainloop(); }
}
// this function is used to change all the information to a msg
// the format used here conforms to the one defined in checkpipe()
int vrpn_Sound_Remote::encode(char *msgbuf, const char *sound, 
                              const int volume, 
				              const int mode, 
							  const int ear, 
							  const int channel)
{
	unsigned i;
	unsigned long *longbuf = (unsigned long*) (void*)(msgbuf);
	unsigned index = 0;
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

int vrpn_Sound_Remote::encode(char *msgbuf, int set_stop, int channel)
{
	unsigned long *longbuf = (unsigned long*) (void*)(msgbuf);
	int index = 0;
	longbuf[index++] = set_stop;
	longbuf[index++] = channel;
	longbuf[0] = htonl(longbuf[0]);
	longbuf[1] = htonl(longbuf[1]);
	return index*sizeof(unsigned long);
}
int vrpn_Sound_Remote::encode(char *msgbuf, int set_load, const char *sound)
{
	unsigned i;
	unsigned long *longbuf = (unsigned long*) (void*)(msgbuf);
	unsigned index = 0;
	longbuf[index++] = set_load;
	for ( i = 0; i< strlen(sound); i++)
		longbuf[index++] = sound[i];
	for ( i = 0; i< index; i++) 
		longbuf[i] = htonl(longbuf[i]);
	return index*sizeof(unsigned long);
}

void vrpn_Sound_Remote::play_sampled_sound(const char *sound, 
                                      const int volume, 
                                      const int mode, 
									  const int ear,
									  const int channel)
{
	struct timeval current_time;
	//pack the message and send it to the sound server through connection
	char msgbuf[1024];
	int len;

	gettimeofday(&current_time, NULL);
	
	if (connection) {
	    len = encode(msgbuf, sound, volume, mode, ear, channel);
		if (connection->pack_message(len, current_time, playsample_id, my_id, 
		  msgbuf, vrpn_CONNECTION_RELIABLE))
		  	fprintf(stderr, "can't write message\n");
	}
}

void vrpn_Sound_Remote::play_stop(const int channel)
{
	struct timeval current_time;
	char msgbuf[1024];
	int len;

	gettimeofday(&current_time, NULL);
	
	if (connection) {
	    len = encode(msgbuf, vrpn_SND_STOP, channel);
		if (connection->pack_message(len, current_time, playsample_id, my_id, 
		  msgbuf, vrpn_CONNECTION_RELIABLE))
		  	fprintf(stderr, "can't write message\n");
	}
}

void vrpn_Sound_Remote::preload_sampled_sound(const char *sound)
{
	struct timeval current_time;
	char msgbuf[1024];
	int len;

	gettimeofday(&current_time, NULL);
	
	if (connection) {
	    len = encode(msgbuf, vrpn_SND_PRELOAD, sound);
		if (connection->pack_message(len, current_time, playsample_id, my_id, 
		  msgbuf, vrpn_CONNECTION_RELIABLE))
		  	fprintf(stderr, "can't write message\n");
	}
}

