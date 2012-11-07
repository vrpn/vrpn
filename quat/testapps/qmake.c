/*****************************************************************************
 *
    qmake.c -  make a quaternion from an axis & angle;  show result as
    	    	quat and matrix
    
    	Session:
	
	    - Enter axis
	    - Enter angle in degrees
	    - Show resulting quat as col matrix and quaternion
	    
    rich holloway,  4/8/91
 *
 *****************************************************************************/

#ifdef _MSC_VER
#pragma warning( disable : 4996 ) // Don't complain about scanf
#endif

#include <stdio.h>
#include "quat.h"

int
main(argc, argv)
    
   short	argc;
char	*argv[];

{

   q_vec_type	axis;
   double  	angle;

   q_type  	resultQuat;
    
   q_matrix_type   resultMatrix;

   /*
    * read in vector and angle
    */
   printf("\nEnter vector (x, y, z) followed by angle (in degrees): ");
   scanf("%lf %lf %lf %lf", 
         &axis[0], &axis[1], &axis[2], &angle);

   q_make(resultQuat, axis[Q_X], axis[Q_Y], axis[Q_Z], Q_DEG_TO_RAD(angle));

   printf("Result quaternion:\n");
   q_print(resultQuat);

   /*
    * matrix of product quat
    */
   q_to_col_matrix(resultMatrix, resultQuat);
   printf("Equivalent column matrix:\n");
   q_print_matrix(resultMatrix);

   return 0;

}	/* main */

