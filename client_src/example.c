/*****************************************************************************
 *
    example.c - more complicated example routine for vlib
    
    Syntax:
	% source ~hmd/bin/v_setenv
    	% example.sh <archive file name(s)>
	
	(Good archive files can be found in ~pxpl5/data and ~hmd/data.)

    Overview:

	This is a more complicated example vlib application than simple.c
	for showing how to read in archive files, use sounds, do flying
	and grabbing.
	
	The intent of this application is to give the most useful 
	capabilities of vixen, without all of the junk that comes along
	with a full Unix tool (like tons of options and modes).

    Interface:
    
	Fly with the left button and grab with the right button.  You
	can only grab an object if your hand is inside its bounding box,
	which will be drawn around it when your hand is inside of it.
	You can grab and fly at the same time.
	
	The tip of the middle finger is the 'hotspot' for grabbing small
	objects.

    Notes:

	- Remember that the room and the tracker fly with you, so the 
	    only thing you can fly toward or away from are the file objects.
	
	- Grabbing and flying both work for as long
	    as you have the button pressed.

	- for info on environment variables, etc, for running this, 
	    see the vlib man page:
	    
	    % setenv MANPATH ${MANPATH}:/usr/proj/hmd/man
	    % man vlib
	
	- to make your own version:
	
	    <remember: use pxpl4 and jason ONLY for *running* your programs.>
	      
	    % cd ~
	    % mkdir vw
	    % cd vw
	    % cp ~hmd/examples/vlib/{example.c,example.h,example.sh,\
	    	interaction.c,pobjects.c,readstructs.c,makefile} .
	    % mkdir sparc_sunos vax_ultrix
	    % make example

	- an identifier starting with v_ or V_ is from vlib;  t_ or T_ means
	    trackerlib; q_ or Q_ is from quatlib.
	    
	- source code for vlib is in /usr/proj/hmd/src/vlib


    Revision History:

    Author	    	Date	  Comments
    ------	    	--------  ----------------------------
    Rich Holloway   	05/19/92  Fixed to work w/ unposted structs
    Rich Holloway   	01/28/92  Made this more vixen-like, but without
    	    	    	    	    all of the overhead.  Now takes filename
				    argument(s);  no more animation.
    Rich Holloway	06/26/91  Added keyboard interface
    Rich Holloway	06/18/91  Added calls to adlib for buttons
    Rich Holloway	06/10/91  Simplified for v3.0
    Rich Holloway	04/16/91  Initial version


   Developed at the University of North Carolina at Chapel Hill, supported
   by the following grants/contracts:
   
     DARPA #DAEA18-90-C-0044
     ONR #N00014-86-K-0680
     NIH #5-R24-RR-02170
   
 *
 *****************************************************************************/

#include <stdlib.h>
#include "example.h"
#include "pc_station.fake.h"

/* globals for doing highlighting box	*/
v_xform_type	    HighlightXform;
v_dlist_ptr_type    HighlightScalePtr;
v_boolean   	    HighlighterActive = V_FALSE;

PC_per_station	*PC;		// Used to connect to the tracker and buttons

/*****************************************************************************
 *
   Move the user's head to the position reported by the VRPN head tracker.
 *
 *****************************************************************************/

void	tracker_callback(struct timeval t, int sensor, float *pos, float *quat)
{
	t_report_type	report;

	t = t; // Keep the compiler happy

	report.xyzQuat.xyz[0] = pos[0];
	report.xyzQuat.xyz[1] = pos[1];
	report.xyzQuat.xyz[2] = pos[2];

	report.xyzQuat.quat[0] = quat[0];
	report.xyzQuat.quat[1] = quat[1];
	report.xyzQuat.quat[2] = quat[2];
	report.xyzQuat.quat[3] = quat[3];

	/* update head or hand from tracker xform */
	switch (sensor) {
	  case 0:
		v_update_head_xform(0, &report);
		break;
	  case 1:
		v_update_hand_xform(0, &report);
		break;
	  default:
		fprintf(stderr,"Unknown tracker update (%d)\n",sensor);
	};
}

/*****************************************************************************
 *
   init - initialize everything
 *
 *****************************************************************************/

void init(v_index *displayIndexPtr)
{
	t_index         trackerIndex;	// Used to open NULL tracker
	t_name_type	trackerName; strcpy(trackerName, "nano_null");

	// Prepare the device (connect)
	PC = new PC_per_station("PC_station_0");

	/* initialize graphics  */
	if ( v_open(PG_POLYMAP) != V_OK )
	    exit(V_ERROR);
	fprintf(stderr, "Done.\n");

	/* 
	 * initialize display, a/d device & tracker
	 */

	/* open display */
	if ( (*displayIndexPtr = v_open_display(V_ENV_DISPLAY, 0)) ==
	     V_NULL_DISPLAY )
	    exit(V_ERROR);

	// Open head and hand tracker
	PC->set_tracker_callback(tracker_callback);

	// Open the button device and set up its callbacks
	PC->set_button_callback(button_callback);

	// Open the NULL tracker to use to fool vlib into orienting things
	// correctly.
	if ( (trackerIndex = t_open(&trackerName, 0)) == T_NULL_TRACKER) {
		exit(V_ERROR);
	}
	t_enable(trackerIndex, 0);
	t_enable(trackerIndex, 1);

	printf("display = '%s', tracker = VRPN, device = VRPN\n", 
	    v_display_name(*displayIndexPtr));

	/* set up user and object trees	*/
	v_create_world(1, V_NUM_USER_ELEMENTS, 0, 
		    NUM_OBJECTS, NUM_ELEMENTS, 2*sizeof(q_vec_type),
		     displayIndexPtr, &trackerIndex, NULL /* no AD device */);

	snd_OpenSound("sdi_sound0");
	snd_OpenFile(SOUND_FILE_NAME);

	/* kill any sounds leftover from previous application	*/
	snd_KillSound(snd_ChAll);

}	/* init */



/*****************************************************************************
 *
   handleTermInput - handle all keyboard events
 
    input:
    	- file descriptor for terminal
	- pointer to 'done' flag
    
    output:
    	- 'done' flag is updated if 'q' is typed at keyboard
 
    notes:
    	- doesn't check the keyboard at every pass, since this isn't really
	    necessary.
 *
 *****************************************************************************/

/* check keyboard every INPUT_CHECK_INTERVAL passes */
#define INPUT_CHECK_INTERVAL	    20

void handleTermInput(int ttyFD, v_boolean *donePtr)
{

    static int	numTimesCalled = 0;
    
    /* buffer must be at least V_MAX_COMMAND_LENGTH bytes long	*/
    char    	buffer[V_MAX_COMMAND_LENGTH];

numTimesCalled++;

/* is it time to really read the keyboard?  */
if ( numTimesCalled <= INPUT_CHECK_INTERVAL )
    return;
else
    numTimesCalled = 0;

/* nonblocking read: is there anything to read at the keyboard?	*/
if ( v_read_raw_term(ttyFD, buffer) < 1 )
    /* no   */
    return;

/* yes-  see what was typed and handle it   */
switch ( buffer[0] )
    {
    case 'q':
    case '\03':	/* ^C	*/
	*donePtr = V_TRUE;
	fprintf(stderr, "\nShutting down...\n");
    	break;
    
    default:
    	fprintf(stderr, "\nunrecognized command: '%s'.\n", buffer);
    	break;
    }

}	/* handleTermInput */


/*****************************************************************************
 *
   doArgs - handle command line arguments
 *
 *****************************************************************************/

void doArgs(int argc, char *argv[],
	   char fileNameList[MAX_FILES][MAX_FILENAME_LENGTH], int *numFilesPtr)
{

    int	    	i;


/* parse command line	*/
for ( i = 1; i < argc; i++ ) 
    {
    /* command line flags	*/
    if ( argv[i][0] == '-' ) 
	{
	/* add your options here    */
	switch ( argv[i][1] )
	    {
	    case 'b':
		/* -b flag does nothing right now   */
		break;
	    default:
		/* bogus flag value	*/
		v_exit("usage:", "example filename1 [filename2 ...]\n");
	    }
	}
    else
    	{
	/* else, fileName argument	*/
	strcpy(fileNameList[*numFilesPtr], argv[i]);
	fprintf(stderr, "file %d: '%s'\n", 
	    	    	    *numFilesPtr, fileNameList[*numFilesPtr]);
    	(*numFilesPtr)++;
	}
    }

/* make sure we got at least one fileName   */
if ( *numFilesPtr == 0 )
    v_exit("usage:", "example filename1 [filename2 ...]\n");

}	/* doArgs */


/*****************************************************************************
 *
   shutDown - close up all devices, etc
 *
 *****************************************************************************/

void shutDown(int ttyFD)
{

/* shut down trackers and a/d devices    */
//XXX

/* close display, etc	*/
v_close();
v_close_raw_term(ttyFD);

/* make sure all sounds are shut off	*/
snd_KillSound(snd_ChAll);

/* play one last sound at quitting time;  this can NOT be snd_LOOPED!	*/
playSound(END_SOUND, snd_DEFAULT);

}	/* shutDown */



int main(int argc, char *argv[])
{

	int	    	    ttyFD;
	v_boolean	    done = V_FALSE;
	char    	    fileNameList[MAX_FILES][MAX_FILENAME_LENGTH];
	int	    	    numFiles = 0;
	v_index 	    displayIndex;

	/* handle filename args	*/
	doArgs(argc, argv, fileNameList, &numFiles);

	/* initialize graphics, tracker, etc	 */
	init(&displayIndex);

	createObjects(fileNameList, numFiles);

	/* open raw terminal with echo   */
	if ( ttyFD = v_open_raw_term(V_TRUE) < 0 )
	    v_exit("example", "error opening terminal.\n");

	playSound(START_SOUND, snd_DEFAULT);
	fprintf(stderr, "\07\nCommand loop (type 'q' to quit): ");

/* 
 * main interactive loop
 */
while ( ! done )
    {

	// Let the PC/station do its callbacks
	PC->loop();

    /* routine for handling mouse button input   */
    interaction();

    /* draw new image for each eye  */
    v_update_displays(&displayIndex);
    
    /* check for keyboard input	*/
    handleTermInput(ttyFD, &done);
    }

shutDown(ttyFD);
    
}   /* main */


