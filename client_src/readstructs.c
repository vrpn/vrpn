/*****************************************************************************
 *
    readstructs.c - read structures in from file and put into display list

    
    Overview:
	
    	This file handles the reading in of all archive files and creates
	a struct for instancing them in the display list.  This struct is
	then used in pobjects.c.
    	
	Only the posted objects in the archive file are added to the
	display list;  this prevents multiple executes of hierarchical
	objects.
    
    
    Notes:
    	- If no structs in the file are posted, we quit.
	
	- Will crash for more than MAX_STRUCTS in one file;  this is
	    defined below.
	


    Revision History:

    Author	    	Date	  Comments
    ------	    	--------  ----------------------------
    Rich Holloway   	05/19/92  Fixed to work w/ unposted structs
    Rich Holloway   	01/28/92  Stolen from vixen.
    Rich Holloway	04/25/91  Initial version


   Developed at the University of North Carolina at Chapel Hill, supported
   by the following grants/contracts:
   
     DARPA #DAEA18-90-C-0044
     ONR #N00014-86-K-0680
     NIH #5-R24-RR-02170
   
 *
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "example.h"

/* there had better not be more than MAX_STRUCTS posted in a file   */
#define MAX_STRUCTS 	1000

/*****************************************************************************
 *
   addPostedStructs - find posted structures in struct list and add them
    	to list of posted structs
 
    input:
    	- pointer to pointer to list of posted struct names
	- pointer to num of currently posted structs
	- pointer to list of new struct names to check
	- number of new structs to check
	- posted list for new structs, one flag per struct
    
    output:
    	- adds posted structs to posted list and bumps counter
 *
 *****************************************************************************/

int addPostedStructs(NameType **postedHandle, int *numPostedPtr,
		     NameType structNames[], int numStructs,
		     v_boolean postedList[], char filename[])
{
    int	    	i;
    int	    	newNumPosted = 0;

/* find out how many posted structs in this batch   */
for ( i = 0; i < numStructs; i++ )
    {
    if ( postedList[i] )
    	newNumPosted++;
    }

/* if nothing posted, post everything	*/
if ( newNumPosted == 0 )
    {
    fprintf(stderr, 
     "\nexample:  no posted structures in '%s';\n", filename);
    fprintf(stderr, 
     "    treating all structs in file as individual objects.\n");
    for ( i = 0; i < numStructs; i++ )
	postedList[i] = V_TRUE;
    newNumPosted = numStructs;
    }

/* otherwise, get enough space for new entries	*/
*postedHandle = (NameType *) v_renew(*postedHandle, 
    	    	    (newNumPosted + *numPostedPtr) * sizeof(NameType));

/* now copy new ones in	*/
for ( i = 0; i < numStructs; i++ )
    {
    if ( postedList[i] )
	{
    	strncpy((char*)((*postedHandle)+(*numPostedPtr)), structNames[i],
		PG_NAMELEN);
	(*numPostedPtr)++;
	}
    }

return(V_OK);

}	/* addPostedStructs */

/*****************************************************************************
 *
   createObject - create an object for file struct
 
    input:
    	- object list
	- pointer to number of objects in list
	- struct to be put into this object
    
    output:
	- instances structName into v_world.objects with its own
	    xform
	- calculates bounding box for object and sticks result in boundVecs
    	- updates number of objects pointed to by numObjectsPtr
    
    notes:
    	- increments v_world.objects.numElements
 *
 *****************************************************************************/

void createObject(NameType structName, q_vec_type boundVecs[2])
{

    q_vec_type	    	*objectBoundVecs;
    int	    	    	numObjects = v_world.objects.numElements;


/* create a transformation for this object  */
v_world.objects.xforms[numObjects].modOption = Replace;
v_create_xform(&v_world.objects.xforms[numObjects]);

/* copy in bounding vectors */
objectBoundVecs = (q_vec_type *) v_world.objects.info[numObjects];

q_vec_copy(objectBoundVecs[0], boundVecs[0]);
q_vec_copy(objectBoundVecs[1], boundVecs[1]);

/* instance the object	*/
strcpy(v_world.objects.names[numObjects], structName);
v_world.objects.dlistPtrs[numObjects] = pg_execute_structure(structName);

/* update object count	*/
v_world.objects.numElements++;

}	/* createObject */

/*****************************************************************************
 *
   createFileObjects - function to create all PPHIGS file objects
 
    notes:
    	- function can handle numFiles == 0
 *
 *****************************************************************************/

int createFileObjects(char fileNameList[MAX_FILES][MAX_FILENAME_LENGTH],
		      int numFiles)
{
    int	    	    i;
    int	    	    numStructs;
    NameType	    *postedStructNames;
    int	    	    numPostedStructs = 0;
    NameType	    *structNames;
    v_boolean	    *postedList;
    
    /* two points for bounding box */
    q_vec_type	    boundVecs[MAX_STRUCTS][2];

/* initial malloc for posted list so realloc doesn't fail   */
postedStructNames = (NameType *) v_new(0);

fprintf(stderr, "Retrieving file objects...");
fflush(stderr);

/* get objects out of file and into display list    */
for ( i = 0; i < numFiles; i++ )
    {
    /* retrieve all of the structs in this file	*/
    if ( v_open_archive_file(fileNameList[i], &structNames, &numStructs,
    	    NULL, NULL, &postedList) < 0 ) 
	{
	fprintf(stderr, "example:  error in open of '%s'; num structs = %d\n", 
				fileNameList[i], numStructs);
	exit(V_ERROR);
	}
    
    /* now find all posted structs in this batch and add them to list	*/
    addPostedStructs(&postedStructNames, &numPostedStructs, structNames,
    	    numStructs, postedList, fileNameList[i]);
    
    fprintf(stderr, ".");
    fflush(stderr);
    
    /* these are malloc'ed by v_open_archive_file() */
    v_free(structNames);
    v_free(postedList);
    }

/* do bounding boxes for posted structs	*/
for ( i = 0; i < numPostedStructs; i++ )
    {
    /* get object's bounding box	*/
    if ( v_get_extent(boundVecs[i], postedStructNames[i]) == V_ERROR )
	{
	fprintf(stderr, "example: couldn't do bounding box for '%s'\n",
			postedStructNames[i]);
	return(V_ERROR);
	}
    
    fprintf(stderr, ".");
    fflush(stderr);
    }

/* now instance all posted structs in virtual world */
pg_open_structure(FILE_OBJECTS_NAME);
    
    for ( i = 0; i < numPostedStructs; i++ )
	createObject(postedStructNames[i], boundVecs[i]);

pg_close_structure();

v_free(postedStructNames);

fprintf(stderr, "   done.\n");

return(V_OK);

}	/* createFileObjects */

