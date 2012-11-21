/*****************************************************************************
 *
    matrix.c-  code for matrix utilities for quaternion library;  routines
    	    	here are only those that have nothing to do with quaternions.
 
    (see quat.h for revision history and more documentation.)
 
 *
 *****************************************************************************/


#include <math.h>                       // for atan2, cos, sin, fabs, sqrt
#include <stdio.h>                      // for printf

#include "quat.h"                       // for q_matrix_type, etc

/* define local X, Y, Z, W to override any external definition;  don't use
 *  Q_X, etc, because it makes this code a LOT harder to read;  don't use 
 *  pphigs definition of X-W since it could theoretically change.
 */

#undef X
#undef Y
#undef Z
#undef W

#define X   	Q_X
#define Y   	Q_Y
#define Z   	Q_Z
#define W   	Q_W


/*
 * q_print_matrix - prints a 4x4 matrix
 */
void q_print_matrix(const q_matrix_type matrix)
{
   int	    i, j;

   for ( i = 0; i < 4; i++ )
   {
      printf(" ");
      for ( j = 0; j < 4; j++ )
         printf("%10lf", matrix[i][j]);
      printf("\n");
   }

}	/* q_print_matrix */


/*****************************************************************************
 *
   q_euler_to_col_matrix - euler angles should be in radians
   	computed assuming the order of rotation is: yaw, pitch, roll.
   
    This means the following:
    
    	p' = roll( pitch( yaw(p) ) )
	 
	 or

    	p' = Mr * Mp * My * p

    Yaw is rotation about Z axis, pitch is rotation about Y axis, and roll
    is rotation about X axis.  In terms of these axes, then, the process is:
    
    	p' = Mx * My * Mz * p
 
    where Mx = the standard Foley and van Dam column matrix for rotation
    about the X axis, and similarly for Y and Z.
    
    Thus the calling sequence in terms of X, Y, Z is:
    
    	q_euler_to_col_matrix(destMatrix, zRot, yRot, xRot);
 *
 *****************************************************************************/
void q_euler_to_col_matrix(q_matrix_type destMatrix, 
                           double yaw, double pitch, double roll)
{
   double  cosYaw, sinYaw, cosPitch, sinPitch, cosRoll, sinRoll;


   cosYaw = cos(yaw);
   sinYaw = sin(yaw);

   cosPitch = cos(pitch);
   sinPitch = sin(pitch);

   cosRoll = cos(roll);
   sinRoll = sin(roll);

   /*
    * compute transformation destMatrix
    */
   destMatrix[0][0] = cosYaw * cosPitch;
   destMatrix[0][1] = cosYaw * sinPitch * sinRoll - sinYaw * cosRoll;
   destMatrix[0][2] = cosYaw * sinPitch * cosRoll + sinYaw * sinRoll;
   destMatrix[0][3] = 0.0;

   destMatrix[1][0] = sinYaw * cosPitch;
   destMatrix[1][1] = cosYaw * cosRoll + sinYaw * sinPitch * sinRoll;
   destMatrix[1][2] = sinYaw * sinPitch * cosRoll - cosYaw * sinRoll;
   destMatrix[1][3] = 0.0;

   destMatrix[2][0] = -sinPitch;
   destMatrix[2][1] = cosPitch * sinRoll;
   destMatrix[2][2] = cosPitch * cosRoll;
   destMatrix[2][3] = 0.0;

   destMatrix[3][0] = 0.0;
   destMatrix[3][1] = 0.0;
   destMatrix[3][2] = 0.0;
   destMatrix[3][3] = 1.0;

}	/* q_euler_to_col_matrix */


/*****************************************************************************
 *
    q_col_matrix_to_euler- convert a column matrix to euler angles    
 
    input:
    	- vector to hold euler angles
	- src column matrix
        q_vec_type  	angles    :      Holds outgoing roll, pitch, yaw
        q_matrix_type 	colMatrix :      Holds incoming rotation 
    
    output:
    	- euler angles in radians in the range -pi to pi;
	    vec[0] = yaw, vec[1] = pitch, vec[2] = roll
	    yaw is rotation about Z axis, pitch is about Y, roll -> X rot.
    
    notes:
    	- written by Gary Bishop
        - you cannot use Q_X, Q_Y, and Q_Z to pull the elements out of
          the Euler as if they were rotations about these angles --
          this will invert X and Z.  You need to instead use Q_YAW
          (rotation about Z), Q_PITCH (rotation about Y) and Q_ROLL
          (rotation about X) to get them.
 *
 *****************************************************************************/
void q_col_matrix_to_euler(q_vec_type yawpitchroll, const q_matrix_type colMatrix)
{

   double sinPitch, cosPitch, sinRoll, cosRoll, sinYaw, cosYaw;


   sinPitch = -colMatrix[2][0];
   cosPitch = sqrt(1 - sinPitch*sinPitch);

   if ( fabs(cosPitch) > Q_EPSILON ) 
   {
      sinRoll = colMatrix[2][1] / cosPitch;
      cosRoll = colMatrix[2][2] / cosPitch;
      sinYaw = colMatrix[1][0] / cosPitch;
      cosYaw = colMatrix[0][0] / cosPitch;
   } 
   else 
   {
      sinRoll = -colMatrix[1][2];
      cosRoll = colMatrix[1][1];
      sinYaw = 0;
      cosYaw = 1;
   }

   /* yaw */
   yawpitchroll[Q_YAW] = atan2(sinYaw, cosYaw);

   /* pitch */
   yawpitchroll[Q_PITCH] = atan2(sinPitch, cosPitch);

   /* roll */
   yawpitchroll[Q_ROLL] = atan2(sinRoll, cosRoll);

} /* col_matrix_to_euler */


/*****************************************************************************
 *
   q_matrix_mult - does a 4x4 matrix multiply (the input matrices are 4x4) and
    	    	  puts the result in a 4x4 matrix.  src == dest ok.
 *
 *****************************************************************************/
void q_matrix_mult(q_matrix_type resultMatrix, const q_matrix_type leftMatrix, 
                   const q_matrix_type rightMatrix)
{
   int	    	    i;
   int	    	    r, c;
   q_matrix_type   tmpResultMatrix;


   /* pick up a row of the multiplier matrix and multiply by a column of the
    *  multiplicand
    */
   for ( r = 0; r < 4; r++ )
      /* multiply each column of the multiplicand by row r of the multiplier   */
      for ( c = 0; c < 4; c++ )
      {
         tmpResultMatrix[r][c] = 0.0;

         /*
          * for each element in the multiplier row, multiply it with each 
          *  element in column c of the multiplicand.
          * i ranges over the length of the rows in multiplier and the length
          *  of the columns in the multiplicand
          */
         for ( i = 0; i < 4; i++ )
            /*
             *  uses 
             *	    	C[r][c] += A[r][i] * B[i][c]
             */
            tmpResultMatrix[r][c] +=  leftMatrix[r][i] * rightMatrix[i][c];
      }


   q_matrix_copy(resultMatrix, ( const double (*)[4] ) tmpResultMatrix);

}	/* q_matrix_mult */

/* 
 *  multiplication of OpenGL-style matrices 
 */
void qogl_matrix_mult(qogl_matrix_type result, const qogl_matrix_type left, 
                      const qogl_matrix_type right )
{
   int i, r, c;
   qogl_matrix_type tmp;

   /* straightforward matrix multiplication */
   for ( r = 0; r < 4; r++ ) {
      for( c = 0; c < 4; c++ ) {
         tmp[r*4+c] = 0.0;
         for( i = 0; i < 4; i++ ) {
            tmp[r*4+c] += left[i*4+c] * right[r*4+i];
         }
      }
   }

   qogl_matrix_copy( result, tmp );
}

/*****************************************************************************
 *
   q_matrix_copy - copies srcMatrix to destMatrix (both matrices are 4x4) 
 *
 *****************************************************************************/
void q_matrix_copy(q_matrix_type destMatrix, const q_matrix_type srcMatrix)
{

   int	    i, j;

   for ( i = 0; i < 4; i++ )
      for ( j = 0; j < 4; j++ )
         destMatrix[i][j] = srcMatrix[i][j];

}	/* q_matrix_copy */

/* 
 * qogl_matrix_copy - copies src to dest, both OpenGL matrices
 */
void qogl_matrix_copy(qogl_matrix_type dest, const qogl_matrix_type src )
{
   int i;

   for( i = 0; i < 16; i++ )
      dest[i] = src[i];
}

/*****************************************************************************
 *
   qgl_print_matrix - print gl-style matrix
 *
 *****************************************************************************/
void qgl_print_matrix(const qgl_matrix_type matrix)
{

   int	    i, j;

   for ( i = 0; i < 4; i++ )
   {
      printf(" ");
      for ( j = 0; j < 4; j++ )
         printf("%10f", matrix[i][j]);
      printf("\n");
   }

}	/* qgl_print_matrix */

/* 
 * print OpenGL-style matrix 
 */
void qogl_print_matrix(const qogl_matrix_type  m )
{
   int i, j;

   for( j = 0; j < 4; j++ ) {
      for( i = 0; i < 4; i++ )
         printf( "%10lf", m[i*4+j] );
      printf( "\n" );
   }

} /* qogl_print_matrix */
