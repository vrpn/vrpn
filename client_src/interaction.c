
/*****************************************************************************
 *
    interaction.c - handles all interaction for example program
    
    
    Overview:
    	- handles all button presses for grabbing & flying
	- fly with left button;  grab with right
    
    Revision History:

    Author	    	Date	  Comments
    ------	    	--------  ----------------------------
    Rich Holloway	01/28/92  Added proper grabbing w/ bounding boxes
    Rich Holloway	06/18/91  Changed to work w/ adlib
    Rich Holloway	06/10/91  Simplified more for vlib v3.0
    Rich Holloway	04/18/91  Ported to pxpl5 & simplified
    Rich Holloway	01/16/91  Initial version


   Developed at the University of North Carolina at Chapel Hill, supported
   by the following grants/contracts:
   
     DARPA #DAEA18-90-C-0044
     ONR #N00014-86-K-0680
     NIH #5-R24-RR-02170
   
 *
 *****************************************************************************/

#include "example.h"
#include <sys/time.h>

static	int	flying = 0;	/* Is the fly button being held? */
static	int	grabbing = 0;	/* Is the grab button being held? */
static	int	objectIndex = NULL_OBJECT;	/* Object we'll grab */


/*****************************************************************************
 *
   turnOnHighlighter - do that.
 *
 *****************************************************************************/

void turnOnHighlighter(void)
{

if ( ! HighlighterActive )
    {
    playSound(HIGHLIGHT_SOUND, snd_LOOPED);
    HighlighterActive = V_TRUE;
    pg_post_structure("highlighter");
    }

}	/* turnOnHighlighter */



/*****************************************************************************
 *
   turnOffHighlighter - do that.
 *
 *****************************************************************************/

void turnOffHighlighter(void)
{


if ( HighlighterActive )
    {
    /* kill highlight sound	*/
    snd_KillSound(snd_ChAll);
    
    HighlighterActive = V_FALSE;
    pg_unpost_structure("highlighter");
    }

}	/* turnOffHighlighter */

    
/*****************************************************************************
 *
   calcBoxIntersect - calculate the intersection of a point and a box
 
    input:
    	- a vector for the point
	- two bounding vectors for the box
    
    output:
    	- distance from point to center of bounding box
 *
 *****************************************************************************/

double calcBoxIntersect(q_vec_type point, q_vec_type boundVecs[2])
{
    int	    	i;
    double  	distance;
    q_vec_type	centerVec;


/* make sure point is greater than all min values   */
for ( i = Q_X; i <= Q_Z; i++ )
    {
    if ( point[i] < boundVecs[0][i] )
    	return(-1);
    }

/* make sure point is less than all max values   */
for ( i = Q_X; i <= Q_Z; i++ )
    {
    if ( point[i] > boundVecs[1][i] )
    	return(-1);
    }

q_vec_add(centerVec, boundVecs[0], boundVecs[1]);
q_vec_scale(centerVec, 0.5, centerVec);
distance = q_vec_distance(centerVec, point);

/* passed all of the tests, so return true  */
return(distance);

}	/* calcBoxIntersect */

/*****************************************************************************
 *
   calculateNearObject - calculate if hand is near any object in objectList
 
    strategy:
    
    	- put each object in world space
	- put hand in world space
	- do distance calculations
 *
 *****************************************************************************/

/* min distance for grabbing in meters	*/
#define MIN_DISTANCE	0.1

int calculateNearObject(void)
{
    int	    	    object;
    double  	    distance;
    double  	    curMin = HUGE_VAL;
    double  	    curObject = NULL_OBJECT;
    v_xform_type    objectFromHand, objectFromWorld;
    v_xform_type    worldFromHand;


/* get hand in world space 	*/
v_get_world_from_hand(0, &worldFromHand);

/* put object into world space to see where it is relative to hand   */
for ( object = 0; object < v_world.objects.numElements; object++ )
    {
    /* get object from world xform  */
    v_x_invert(&objectFromWorld, &v_world.objects.xforms[object]);

    v_x_compose(&objectFromHand, &objectFromWorld, &worldFromHand);

    if ( (distance = calcBoxIntersect(objectFromHand.xlate, 
 	    	    (q_vec_type *) v_world.objects.info[object])) >= 0 )
    	{
	/* got an intersection;  is it the best so far?	*/
	if ( distance < curMin )
	    {
    	    /* save new min distance    */
	    curMin = distance;
	    curObject = object;
	    }
	}
    }

/* if there is no object near, turn off the highlighter	*/
if ( curObject == NULL_OBJECT )
    turnOffHighlighter();

return(curObject);

}	/* calculateNearObject */



/*****************************************************************************
 *
   indicateNear - indicate when hand is near given object by highlighting it
 *
 *****************************************************************************/

void indicateNear(int objectIndex)
{
    int	    	    i;
    v_xform_type    *xformPtr;
    q_vec_type	    *srcBoundVecs;
    q_vec_type	    scale;
    q_vec_type	    boundVecs[2];
    
    static v_xform_type	objFromMinCorner = V_ID_XFORM;
    

/* local pointer    */
xformPtr = &v_world.objects.xforms[objectIndex];
srcBoundVecs = (q_vec_type *) v_world.objects.info[objectIndex];

/* get object's bounding box in world space;  xform bounding vecs by
 *  world-object xform
 */
v_x_xform_vector(boundVecs[0], xformPtr, srcBoundVecs[0]);
v_x_xform_vector(boundVecs[1], xformPtr, srcBoundVecs[1]);

v_x_copy(&HighlightXform, &v_world.objects.xforms[objectIndex]);
q_vec_copy(objFromMinCorner.xlate, srcBoundVecs[0]);

/* xlate to the min corner  */
v_x_compose(&HighlightXform, &HighlightXform, &objFromMinCorner);

v_replace_xform(&HighlightXform);

/* now scale box into object's scale	*/
for ( i = 0; i < 3; i++ )
    scale[i] = srcBoundVecs[1][i] - srcBoundVecs[0][i];

pg_set_element_pointer(HighlightScalePtr);
pg_local_scaling(scale[Q_X], scale[Q_Y], scale[Q_Z], Postconcatenate);

turnOnHighlighter();

}	/* indicateNear */


/*****************************************************************************
 *
	Callback routine to handle button press and release events.  This
   will start the user flying when the first button is pressed and stop when
   it is release.  It will start grabbing when the second is pressed and
   stop when it is released.
 *
 *****************************************************************************/

void button_callback(struct timeval t, int button_num, int pressed)
{
	printf("Button %d %s at %ld:%ld\n",button_num,
		pressed?"pressed":"released",
		t.tv_sec, t.tv_usec);

	switch (button_num) {
	  case 0:	/* Flying button */
		if (pressed) {
		  flying = 1;

		  /* change hand to arrow for flying  */
		  v_replace_structure(v_users[0].dlistPtrs[V_HAND], "v_arrow");
		
		  /* play flying sound so it will continue until killed */
		  playSound(FLY_SOUND, snd_LOOPED);

		} else {	/* Released */
		  flying = 0;

		  /* change hand back on release    */
		  v_replace_structure(v_users[0].dlistPtrs[V_HAND], "myHand");
		
		  /* shut off sound now	*/
		  snd_KillSound(snd_ChAll);
		}
		break;

	  case 1:	/* Grabbing button */
		if (pressed) {
		  grabbing = 1;

		  /* kills highlighter and stops the highlighter sound	*/
		  turnOffHighlighter();
		
		  /* play sound to indicate a grab  */
		  if ( objectIndex != NULL_OBJECT )
		    playSound(GRAB_SOUND, snd_DEFAULT);
		
		} else {	/* Released */
		  grabbing = 0;

		  /* release object   */
		  if ( objectIndex != NULL_OBJECT )
		    v_release(0, &v_world.objects.xforms[objectIndex]);
		}
		break;

	  default:
		printf("Button %d %s (no effect)\n",button_num,
			pressed?"pressed":"released");
	}
}


/*****************************************************************************
 *
   doFly - fly through the world when left button is pressed
 *
 *****************************************************************************/

void doFly(void)
{
	if (flying) {
		v_fly(0, 1.0 / UPDATE_RATE);
	}
}



/*****************************************************************************
 *
   doGrab - handle grab operation w/ right button
 *
 *****************************************************************************/

void doGrab(void)
{
    if (grabbing) {
	/* if inside object, grab it  */
    	if ( objectIndex != NULL_OBJECT )
	    v_grab(0, &v_world.objects.xforms[objectIndex]);
    } else {
	/* see if hand is near any object	*/
	if ( (objectIndex = calculateNearObject()) != NULL_OBJECT )
	    indicateNear(objectIndex);
    }
}	/* doGrab */

/*****************************************************************************
 *
   interaction - handle button presses
 *
 *****************************************************************************/

void interaction(void)
{
	doFly();
	doGrab();
}

/*****************************************************************************
 *
   playSound - play given sound
 
    input:
    	- string for sound to play
    
    output:
    	- sound is played on macintosh
 *
 *****************************************************************************/

void playSound(char soundString[], char mode)
{

if ( strcmp(soundString, "") != 0 )
    snd_PlaySound(soundString, 255, mode, snd_Ch1);

}	/* playSound */
