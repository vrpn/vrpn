/*****************************************************************************
 *
    qxform.c - does xform of a vector
    
    	Session:
	
	    - Enter quaternion [(x, y, z), w] (no punctuation)
	    - Enter vector to be xformed
	    - Show quat as col matrix
	    - Xform vector and print result
	    
    rich holloway,  9/25/90
	    
 *
 *****************************************************************************/


#ifdef _MSC_VER
#pragma warning( disable : 4996 ) // Don't complain about scanf
#endif

#include <stdio.h>
#include "quat.h"

int
main(int argc, char	*argv[])
{

   q_type	multQuat;
   q_type 	xformedPoint;
   q_type  	point;
    
   double  	matrix[4][4];

   /*
    * read in, echo, and normalize 2 quaternions
    */
   printf("\nEnter xform quaternion: (vec, s) ");
   scanf("%lf %lf %lf %lf", 
         &multQuat[0], &multQuat[1], &multQuat[2], &multQuat[3]);

   printf("\nEnter point: (x y z) ");
   scanf("%lf %lf %lf", 
         &point[Q_X], &point[Q_Y], &point[Q_Z]);
   point[Q_W] = 0.0;

   /*
    * matrix of product quat
    */
   q_to_col_matrix(matrix, multQuat);
   printf("Transformation (column) matrix:\n");
   q_print_matrix(matrix);

   /* 
    * xformedPoint = (multQuat * candQuat) * invertedQuat	
    */
   q_xform(xformedPoint, multQuat, point);

   printf("Xform Result:\n");
   q_print(xformedPoint);

   return(0);

}	/* main */

