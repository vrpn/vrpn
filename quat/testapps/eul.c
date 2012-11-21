

/*****************************************************************************
 *
    eul.c - convert from euler angles to quaternion
    
    gary bishop	    01/24/92	- added q_col_matrix_to_euler() test
    rich holloway   10/15/90	- first version
	    
 *
 *****************************************************************************/

#ifdef _MSC_VER
#pragma warning( disable : 4996 ) // Don't complain about scanf
#endif

#include <stdio.h>
#include "quat.h"

int
main(int argc, char *argv[])
{

    q_type 	    q;
    double  	    yaw, pitch, roll;
    q_matrix_type   matrix, matrix2;
    q_vec_type	    yawPitchRoll;


/*
 * read in euler angles
 */
while (1) 
    {
    printf("\nEnter euler angles in degrees: (yaw, pitch, roll) ");
    scanf("%lf %lf %lf", &yaw, &pitch, &roll);
    yaw *= Q_PI/180;
    pitch *= Q_PI/180;
    roll *= Q_PI/180;

    /* matrix from euler	*/
    q_euler_to_col_matrix(matrix, yaw, pitch, roll);
    printf("Col Matrix directly:\n");
    q_print_matrix(matrix);

    /* q from euler	*/
    q_from_euler(q, yaw, pitch, roll);
    printf("q :\n");
    q_print(q);

    /* matrix of q */
    q_to_col_matrix(matrix, q);
    printf("Col Matrix from q:\n");
    q_print_matrix(matrix);

    q_col_matrix_to_euler(yawPitchRoll, matrix);
    
    printf("euler angles from matrix\n");
    printf("%g %g %g\n", 180*yawPitchRoll[0]/Q_PI, 
	    180*yawPitchRoll[1]/Q_PI, 180*yawPitchRoll[2]/Q_PI);

    q_euler_to_col_matrix(matrix2, yawPitchRoll[0], 
    	    	    	    yawPitchRoll[1], yawPitchRoll[2]);
    printf("matrix from euler\n");
    q_print_matrix(matrix2);
    }

}	/* main */





