/*****************************************************************************
 *
    pobjects.c -  object definition code
    
    
    Overview:
    	This file defines all PPHIGS objects.
	Most of the calls in this file are therefore to PPHIGS, with
	occasional calls to vlib for setting up xforms (which are passed
	to PPHIGS via v_create_xform() and v_replace_xform() calls).
    
    Notes:
    	This is meant to be a simple template-  your application may need
	a more complicated set of data structures for keeping track of
	your world's objects.


    Revision History:

    Author	    	Date	  Comments
    ------	    	--------  ----------------------------
    Rich Holloway   	01/28/92  Made to be more like vixen.
    Rich Holloway	06/10/91  Updated for v3.0
    Rich Holloway	04/16/91  Initial version


   Developed at the University of North Carolina at Chapel Hill, supported
   by the following grants/contracts:
   
     DARPA #DAEA18-90-C-0044
     ONR #N00014-86-K-0680
     NIH #5-R24-RR-02170
   
 *
 *****************************************************************************/

#include "example.h"


/*****************************************************************************
 *
   myHand - draw hand icon
 *
 *****************************************************************************/


void myHand(void)

{
    static char	fileName[] = HAND_ARCHIVE_FILE_NAME;
    NameType	*structNames;
    int	    	numStructs;


if ( v_open_archive_file(fileName, &structNames, &numStructs, 
    	    	         	    	NULL, NULL, NULL) < 0 )
    v_exit("example", "open of archive file '%s.archive' failed.\n",
    	HAND_ARCHIVE_FILE_NAME);


pg_open_structure("myHand");
    pg_execute_structure(structNames[0]);
pg_close_structure();
				     
v_free(structNames);

}	/* myHand */



/*****************************************************************************
 *
   highlighter - struct for highlighting objects
 *
 *****************************************************************************/

void highlighter(void)
{

pg_open_structure("highlighter");
    
    /* this is what gets updated to move the box to the object's location   */
    v_create_xform(&HighlightXform);
    
    /* this is what gets updated to warp the box to fit	*/
    HighlightScalePtr = pg_local_scaling(1.0, 1.0, 1.0, Postconcatenate);

    pg_execute_structure("wireCube");

pg_close_structure();

}	/* highlighter */



/*****************************************************************************
 *
   intLine - add a line to the display list given two integer points
 
    input:
    	- x1->z2
    
    output:
    	- returns dlistptr to line
 *
 *****************************************************************************/

DlistAddress intLine(int x1, int y1, int z1, int x2, int y2, int z2)
{

    PointType   points[2];

points[0][X] = x1;
points[0][Y] = y1;
points[0][Z] = z1;

points[1][X] = x2;
points[1][Y] = y2;
points[1][Z] = z2;

return( pg_line(points[0], points[1]) );

}	/* intLine */



/*****************************************************************************
 *
   wireCube - draw wire-frame cube
 *
 *****************************************************************************/

void wireCube(void)
{

pg_open_structure("wireCube");
    
    /* front square */
    intLine( 0,  0,  1,    1,  0, 1);
    intLine( 1,  0,  1,    1,  1, 1);
    intLine( 1,  1,  1,    0,  1, 1);
    intLine( 0,  1,  1,    0,  0, 1);

    /* back square */
    intLine( 0,  0,  0,    1,  0,  0);
    intLine( 1,  0,  0,    1,  1,  0);
    intLine( 1,  1,  0,    0,  1,  0);
    intLine( 0,  1,  0,    0,  0,  0);

    /* connect back and front	*/
    intLine( 0,  0,  1,    0,  0,  0);
    intLine( 1,  0,  1,    1,  0,  0);
    intLine( 1,  1,  1,    1,  1,  0);
    intLine( 0,  1,  1,    0,  1,  0);

pg_close_structure();


}	/* wireCube */

/*****************************************************************************
 *
   createObjects - function to create all PPHIGS objects
 *
 *****************************************************************************/

void createObjects(char fileNameList[MAX_FILES][MAX_FILENAME_LENGTH],
		  int numFiles)
{

/* reset this to zero, since we're more interested in how many objects
 *  we have actually used so far, rather than how much space is reserved.
 *  we'll use this as an index for how many objects have been defined so
 *  far, and will increment it in readstructs.c
 */
v_world.objects.numElements = 0;

/* calls to define various objects 	*/
highlighter();
wireCube();
myHand();

/* this stuff defined in readstructs.c;  reads in archive files	*/
createFileObjects(fileNameList, numFiles);

/* replace null object at user's hand with my hand definition	*/
v_replace_structure(v_users[0].dlistPtrs[V_HAND], "myHand");

/* replace null object at tracker with a coordinate system; 
 *  v_coord_sys is defined by vlib
 */
v_replace_structure(v_users[0].dlistPtrs[V_TRACKER], "v_coord_sys");

/* put a coord sys at the room origin	*/
v_replace_structure(v_users[0].dlistPtrs[V_ROOM], "v_coord_sys");


/* vlib expects the user code to define a struct with this name;  put
 *   all of the objects in your world in this structure
 */
pg_open_structure("v_objects");

    /* instance the object  */
    pg_execute_structure(FILE_OBJECTS_NAME);

pg_close_structure();

}	/* createObjects */

