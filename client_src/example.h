

/*****************************************************************************
 *
    example.h - All #defines and typedefs
    
    For revision history, see example.c.
 *
 *****************************************************************************/

#include <v.h>
#include <sound.h>
#include <string.h>

/* Should be in v.h */
extern "C" int v_get_extent(q_vec_type extentVectors[2], NameType structName);
extern "C" int v_free(void *ptr);


/* indices into tracker report list */
#define HEAD	0
#define HAND	1

/* index for null object    */
#define NULL_OBJECT 	    (-1)

/* for filename arguments	*/
#define MAX_FILES            50
#define MAX_FILENAME_LENGTH  200

/* max number of objects for this application	*/
#define NUM_OBJECTS 	1000

/* number of user-defined transformations per object	*/
#define NUM_ELEMENTS	20

/* rough guess at update rate; used for flying speed	*/
#define	UPDATE_RATE 	20

#define FILE_OBJECTS_NAME   "file_objects"

/* on sound-server Mac	*/
#define SOUND_FILE_NAME	    "vlib_example_sounds"

#ifdef sun
#   define HAND_ARCHIVE_FILE_NAME 	"/usr/proj/hmd/data/hand"
#else
#   define HAND_ARCHIVE_FILE_NAME 	"/usr/proj/hmd/data/pxpl4/hand"
#endif

/* sounds in SOUND_FILE_NAME on sound Mac   */
#define AAOO	    	    	"aaoo"
#define HUM	    	    	"hum"
#define BOTTLE_ROCKET	    	"rocket"
#define JET 		    	"jet"
#define FLOOP	    	    	"floop"
#define BELL	    	    	"bell"
#define GONG	    	    	"gong"

/* sounds used in the program	*/
#define START_SOUND    	    BELL
#define END_SOUND  	    GONG
#define GRAB_SOUND  	    FLOOP
#define HIGHLIGHT_SOUND	    HUM
#define FLY_SOUND   	    JET




/*****************************************************************************
 *
    globals
 *
 *****************************************************************************/


/* for doing highlighting box	*/
extern v_xform_type	HighlightXform;
extern v_dlist_ptr_type	HighlightScalePtr;
extern v_boolean    	HighlighterActive;


/*****************************************************************************
 *
    routines
 *
 *****************************************************************************/

extern	void button_callback(struct timeval t, int button_num, int pressed);
extern	void	interaction(void);
extern	void	playSound(char soundString[], char mode);

extern	void	createObjects(char fileNameList[MAX_FILES][MAX_FILENAME_LENGTH],
				int     numFiles);
extern	int	createFileObjects(
			char fileNameList[MAX_FILES][MAX_FILENAME_LENGTH],
			int numFiles);
