#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef __STDC__
#ifndef	__hpux
#include <getopt.h>
#endif
#endif /* __STDC__ */

#ifdef	linux
#include <sys/soundcard.h>

struct sbi_instrument instr;
#endif
static int sb;
static unsigned char sbbuf[404];
static int sbptr = 0;
static int instrf;
FILE *fp;

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
	if (sbptr> 400) sbflush();

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

void noteon (int channel, int pitch, int vol)
{
#ifdef	linux
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
#ifdef	linux
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
#ifdef	linux
	int jiffies;
	jiffies = (delay << 8) | TMR_WAIT_REL;
//	jiffies = (delay << 8) | SEQ_WAIT;
	sbwrite((char*)&jiffies);
#endif
}

#ifdef __STDC__
void usage(char *s) 
#else /* __STDC__ */
volatile void usage(char *s) 
#endif /* __STDC__ */
{
	fprintf(stderr,
		"usage: %s [-h] [-o octave] [-t time] [-i instr]\n",s);
	fprintf(stderr,
		"       [-v volume] [-c channel] [sbifile]\n");
	fprintf(stderr,
		" Default values are: -o 3 -t 50 -i 0 -v 64 -c 0\n");
	exit(9);
}
init(int chan)
{
#ifdef	linux
	char buf[100];
	buf[0] = SEQ_FMPGMCHANGE;
	buf[1] = chan;		/* program voice #channel to */
	buf[2] = chan; 		/* either play default instrument 0 */
  		/*buf[2] = nr_instr; or play requested instrument */
	sbwrite(buf);
#endif
}

struct entity
{
	int channel;
	int octave;
	int notevalue;
	int time;
	struct entity * next;
};

// There are two phrases, A and B -- this tells how many there are
#define s_num 2

// Lists for each phrase of up to 10 channels each.
// These lists hold notes to be played; the start and the end of each phrase.
struct entity *head[s_num][10], *tail[s_num][10];

int num_channel;	// Number of channels, read from the file (up to 10)
int notenew[10];	// Whether a certain channel needs to be turned on
			// at the next basic timing unit
int total_duration;	// Total duration of the currently-playing phrase
int unit = 25;		// One basic timing unit

void opendevice()
{
#ifdef	linux
	int i;
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
        if ((sb=open("/dev/sequencer", O_WRONLY|O_NONBLOCK, 0))==-1)
        {
                perror("/dev/sequencer");
                exit(-1);
        }
    	for (i = 0; i< 10; i++)
		notenew[i] = 1;
#endif
}

// Starts and/or stops all the notes for a given basic unit of time.  The
// phrase number to use is in s_id, and scaled_data is the client-side data
// value (which is from 0-1) scaled to the range 1-4, to tell how many
// channels should play.

setup1(int s_id, int scaled_data)
{
	int volume = 60, i, j, vol, vol1;
	static struct entity *p[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
	static int ptime[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	// Set the lists for each channel to the head of the phrase that we
	// were asked to play (s_id).
	for ( i = 0; i < num_channel; i++) {
	    if (p[i] == NULL) {
		p[i] = head[s_id][i];
		notenew[i] = 1; 
		ptime[i] = p[i]->time;
	    }
	}

	// For each channel that has a list of notes associated with it,
	// see if we need to turn the note on or off 
    for ( i = 0; i < num_channel; i++) {
	if  (notenew[i]) {
		switch (p[i]->octave) {
			case 1: vol1 = (int) volume * 2;break;
			case 2: vol1 = (int) volume * 1.4; break;
			case 3: vol1 = (int) volume * 1.15; break;
			default: vol1 = volume; break;
		}
		switch (p[i]->channel) {
			case 1: vol = (int) vol1 * 0.7; break;
			default: vol = vol1; break;
		}
		if (i >= scaled_data) vol = 0;
		vol = vol + (int) (scaled_data * 30/4);
		noteon(p[i]->channel, p[i]->notevalue + 12 * p[i]->octave, vol);
//		p[i]->time--;
		ptime[i]--;
		notenew[i] = 0;
	} else if (ptime[i] > 0) {
		//p[i]->time--;
		ptime[i]--;
	} else if (ptime[i] <= 0) {
	    noteoff(p[i]->channel, p[i]->notevalue + 12 * p[i]->octave, vol);
	    p[i] = p[i]->next;
	    if (p[i] != NULL) {
		ptime[i] = p[i]->time;
		switch (p[i]->octave) {
			case 1: vol1 = (int) volume * 2;break;
			case 2: vol1= (int) volume * 1.4; break;
			case 3: vol1 = (int) volume * 1.15; break;
			default: vol1 = volume; break;
		}
		switch (p[i]->channel) {
			case 1: vol = (int) vol1 * 0.7; break;
			default: vol = vol1; break;
		}
		if (i >= scaled_data) vol = 0;
		vol = vol + (int) (scaled_data * 30* 1.0/4);
		noteon(p[i]->channel, p[i]->notevalue + 12 * p[i]->octave, vol);
	//	p[i]->time--;
		ptime[i]--;
	    }
	}
    }
	wait(unit);
	sbflush();
}
	
setup(int s_id, int channel, int octave, char note, int duration)
{
	struct entity *p, *q, *tmp;
	int notevalue;
	q = tail[s_id][channel]; 
	tmp = (struct entity *) calloc(1, sizeof(struct entity));
	tmp->channel = channel;
	tmp->octave = octave;
	switch (note) {
	case 'A' : notevalue = 0; break;
	case 'B' : notevalue = 2; break;
	case 'C' : notevalue = 3; break;
	case 'D' : notevalue = 5; break;
	case 'E' : notevalue = 7; break;
	case 'F' : notevalue = 8; break;
	case 'G' : notevalue = 10; break;
	default: break;
	}
	tmp->notevalue = notevalue;
	tmp->time = duration;
	if (q == NULL) { head[s_id][channel] = tmp; tail[s_id][channel] = tmp; }
	else {
	q->next = tmp;
	tmp->next = NULL;
	tail[s_id][channel] = tmp;
	}
}
void randomplay(int j, int dateflow)
{
	int i;
    char name[100];
    int id, count;
    char c2;
    int c1, c3;
	opendevice();
   	for ( i = 0; i< 10; i++)
	{
		head[j][i] = NULL;
		tail[j][i] = NULL;
	}
	switch (j) {
	case 0:
		fp = fopen("/net/nano/nano3/sounds/garth_a.txt", "r");
		break;
	case 1:
		fp = fopen("/net/nano/nano3/sounds/garth_b.txt", "r");
		break;
	}
	if (fp == NULL) {
		perror("Cannot open tune file");
		return;
	}
        fscanf(fp, "%s %d", name, &total_duration);
        fscanf(fp, "%s %d", name, &unit);
        fscanf(fp, "%s %d", name, &num_channel);
        for ( i = 0; i < num_channel; i++) {
        count = 0;
        fscanf(fp, "%s %d %s %s %s\n", name, &id, name, name, name);
        while (count < total_duration) {
        fscanf(fp, "\t%d\t%c\t%d", &c1, &c2, &c3);
        setup(j, id, c1, c2, c3);
        count += c3;
        }
        fscanf(fp, "%s", name);
        }
        fclose(fp);
}
