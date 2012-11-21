/*****************************************************************************
 *
    vector.c -  vector routines for quatlib that are unrelated to quaternions
    
 
    (see quat.h for revision history and more documentation.)
 
 *
 *****************************************************************************/


#include <math.h>                       // for sqrt
#include <stdio.h>                      // for fprintf, printf, stderr

#include "quat.h"                       // for Q_X, Q_Y, Q_Z, q_vec_type, etc

/*****************************************************************************
 *
   q_vec_print - prints a vector to stdout
 *
 *****************************************************************************/
void q_vec_print(const q_vec_type vec)
{

   printf("(%lf, %lf, %lf)\n", vec[Q_X], vec[Q_Y], vec[Q_Z]);

}	/* q_vec_print */



/*****************************************************************************
 *
   q_vec_set - set vector equal to 3 values given
 *
 *****************************************************************************/
void q_vec_set(q_vec_type vec, double x, double y, double z)
{
   vec[Q_X] = x;
   vec[Q_Y] = y;
   vec[Q_Z] = z;

}	/* q_set_vec */


/*****************************************************************************
 *
   q_vec_copy - copies srcVec to destVec
 *
 *****************************************************************************/
void q_vec_copy(q_vec_type destVec, const q_vec_type srcVec)
{

   destVec[Q_X] = srcVec[Q_X];
   destVec[Q_Y] = srcVec[Q_Y];
   destVec[Q_Z] = srcVec[Q_Z];

}	/* q_vec_copy */

    

/*****************************************************************************
 *
   q_vec_add - adds two vectors
 
    input:
    	- destVec, aVec, bVec
    
    output:
    	- destVec = aVec + bVec
    
    notes:
    	- src and dest may be same storage
 *
 *****************************************************************************/
void q_vec_add(q_vec_type destVec, const q_vec_type aVec, const q_vec_type bVec)
{
   int	    i;

   for ( i = 0; i < 3; i++ )
      destVec[i] = aVec[i] + bVec[i];

}	/* q_vec_add */


/*****************************************************************************
 *
   q_vec_subtract - destVec = v1 - v2
 
    input:
    	- dest vector
	- v1, v2
    
    output:
    	- dest vec
    
    notes:
    	- v1, v2, destVec need not be distinct storage
 *
 *****************************************************************************/
void q_vec_subtract(q_vec_type destVec, const q_vec_type v1, const q_vec_type v2)
{

   destVec[Q_X] = v1[Q_X] - v2[Q_X];
   destVec[Q_Y] = v1[Q_Y] - v2[Q_Y];
   destVec[Q_Z] = v1[Q_Z] - v2[Q_Z];

}	/* q_vec_subtract */



/*****************************************************************************
 *
   q_vec_dot_product - returns value of dot product of v1 and v2
 *
 *****************************************************************************/
double q_vec_dot_product(const q_vec_type v1, const q_vec_type v2)
{

   return((v1[Q_X] * v2[Q_X]) + (v1[Q_Y] * v2[Q_Y]) + (v1[Q_Z] * v2[Q_Z]));

}	/* q_vec_dot_product */




/*****************************************************************************
 *
   q_vec_scale - scale a vector
 
    input:
    	- pointer to destination vector
	- scale factor
	- pointer to src vector
	
    output:
    	- dest vec is scaled by given amount
    
    notes:
    	- src and dest need not be distinct
 *
 *****************************************************************************/
void q_vec_scale(q_vec_type destVec, double scaleFactor, const q_vec_type srcVec)
{

   destVec[Q_X] = srcVec[Q_X] * scaleFactor;
   destVec[Q_Y] = srcVec[Q_Y] * scaleFactor;
   destVec[Q_Z] = srcVec[Q_Z] * scaleFactor;

}	/* q_vec_scale */



/*****************************************************************************
 *
   q_vec_invert - negate a vector to point in the opposite direction
 
    input:
    	- pointer to destination vector
	- pointer to src vector
	
    output:
    	- dest vec is negated/inverted
    
    notes:
    	- src and dest need not be distinct
	- this routine is called invert rather than negate to be consistent
	  with q_invert for quaternions
 *
 *****************************************************************************/
void q_vec_invert(q_vec_type destVec, const q_vec_type srcVec)
{

   destVec[Q_X] = -srcVec[Q_X];
   destVec[Q_Y] = -srcVec[Q_Y];
   destVec[Q_Z] = -srcVec[Q_Z];

}	/* q_vec_invert */



/*****************************************************************************
 *
   q_vec_magnitude - returns magnitude of vector
 
    input:
    	- vector
    
    output:
    	- magnitude
    
 *
 *****************************************************************************/
double q_vec_magnitude(const q_vec_type vec)
{

   return( sqrt( vec[Q_X]*vec[Q_X] + vec[Q_Y]*vec[Q_Y] + vec[Q_Z]*vec[Q_Z] ) );

}	/* q_vec_magnitude */



/*****************************************************************************
 *
   q_vec_normalize - normalize a vector
 
    input:
    	vec - a vector
	destVec - output parameter
    
    output:
    	normalized vector is put in destVec
    
    notes:
    	destVec and srcVec may be the same
 *
 *****************************************************************************/
void q_vec_normalize(q_vec_type destVec, const q_vec_type srcVec)
{
   double normalizeFactor;
   double magnitude;

   if ( (magnitude = q_vec_magnitude(srcVec)) < Q_EPSILON )
   {
      fprintf(stderr, "quatlib: q_vec_normalize: vector has 0 magnitude.\n");
      return;
   }

   normalizeFactor = 1.0 / magnitude;

   destVec[Q_X] = srcVec[Q_X] * normalizeFactor;
   destVec[Q_Y] = srcVec[Q_Y] * normalizeFactor;
   destVec[Q_Z] = srcVec[Q_Z] * normalizeFactor;


}	/* q_vec_normalize */



/*****************************************************************************
 *
   q_vec_distance - returns distance between two points/vectors
 *
 *****************************************************************************/
double q_vec_distance(const q_vec_type vec1, const q_vec_type vec2)
{

   return( sqrt((vec2[Q_X] - vec1[Q_X]) * (vec2[Q_X] - vec1[Q_X]) + 
                (vec2[Q_Y] - vec1[Q_Y]) * (vec2[Q_Y] - vec1[Q_Y]) + 
                (vec2[Q_Z] - vec1[Q_Z]) * (vec2[Q_Z] - vec1[Q_Z])) );

}	/* q_vec_distance */



/*****************************************************************************
 *
   q_vec_cross_product - computes cross product of two vectors:
    
    	    	    	destVec = aVec X bVec
 
    input:
    	destVec:    pointer to output vector
	aVec:  	    first vector
	bVec:  	    second vector
    
    output:
    	destVec = aVec X bVec
    
    notes:
    	destVec == aVec or bVec ok.
 *
 *****************************************************************************/
void q_vec_cross_product(q_vec_type destVec, const q_vec_type aVec, const q_vec_type bVec)
{
   q_vec_type	tmpDestVec;


   tmpDestVec[Q_X] = aVec[Q_Y] * bVec[Q_Z] - aVec[Q_Z] * bVec[Q_Y];
   tmpDestVec[Q_Y] = aVec[Q_Z] * bVec[Q_X] - aVec[Q_X] * bVec[Q_Z];
   tmpDestVec[Q_Z] = aVec[Q_X] * bVec[Q_Y] - aVec[Q_Y] * bVec[Q_X];

   q_vec_copy(destVec, tmpDestVec);

}	/* q_vec_cross_product */


