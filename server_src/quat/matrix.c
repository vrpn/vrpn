
/*****************************************************************************
 *
    matrix.c-  code for matrix utilities for quaternion library;  routines
    	    	here are only those that have nothing to do with quaternions.
 
    (see quat.h for revision history and more documentation.)
 
 *
 *****************************************************************************/


#include "quat.h"

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
void
q_print_matrix(q_matrix_type matrix)
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


void
q_euler_to_col_matrix(q_matrix_type destMatrix,double yaw, 
					  double pitch, double roll)
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
    
    output:
    	- euler angles in radians in the range -pi to pi;
	    vec[0] = yaw, vec[1] = pitch, vec[2] = roll
	    yaw is rotation about Z axis, pitch is about Y, roll -> X rot.
    
    notes:
    	- written by Gary Bishop
 *
 *****************************************************************************/

void
q_col_matrix_to_euler(q_vec_type angles, q_matrix_type colMatrix)
 //       q_vec_type  	angles;        /* Holds outgoing roll, pitch, yaw */
  //      q_matrix_type 	colMatrix;      /* Holds incoming rotation */
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
angles[0] = atan2(sinYaw, cosYaw);

/* pitch */
angles[1] = atan2(sinPitch, cosPitch);

/* roll */
angles[2] = atan2(sinRoll, cosRoll);

} /* col_matrix_to_euler */


/*****************************************************************************
 *
    pphigs support routines
 *
 *****************************************************************************/


/*
 * qp_print_matrix - prints a 3x4 PPHIGS matrix
 */
void
qp_print_matrix(Q_MatrixType  matrix)
{
    int	    i, j;


for ( i = 0; i < 3; i++ )
    {
    printf(" ");
    for ( j = 0; j < 4; j++ )
	printf("%10lf", matrix[i][j]);
    printf("\n");
    }

}	/* qp_print_matrix */



/*****************************************************************************
 *
   qp_file_print_matrix - print a PPHIGS matrix to file
 
    input:
    	- file pointer
	- matrix
    
    output:  none
 *
 *****************************************************************************/

void
qp_file_print_matrix(FILE *filePtr, Q_MatrixType matrix)
{

    int	    i, j;


for ( i = 0; i < 3; i++ )
    {
    fprintf(filePtr, " ");
    for ( j = 0; j < 4; j++ )
	fprintf(filePtr, "%10lf", matrix[i][j]);
    fprintf(filePtr, "\n");
    }

}	/* qp_file_print_matrix */




/*****************************************************************************
 *
    qp_row_to_pmatrix- converts row matrix (ie, with translation amounts
	    	    on last row) to PPHIGS matrix, which is the transpose
   	    	    of the normal matrix, then leave off the last row (which
   	    	    is always (0, 0, 0, 1)).
 
    
    notes:
    	- when compiled w/ -O on an Iris Indigo, each call to this function
	    took ~4.4us (6/92).
    
 *
 *****************************************************************************/

void
qp_row_to_pmatrix(Q_MatrixType pMatrix, q_matrix_type rowMatrix)
{
    Q_MatrixType	    tempMatrix;

/* transpose and copy */
tempMatrix[X][X] = rowMatrix[X][X];
tempMatrix[X][Y] = rowMatrix[Y][X];
tempMatrix[X][Z] = rowMatrix[Z][X];
tempMatrix[X][W] = rowMatrix[W][X];

tempMatrix[Y][X] = rowMatrix[X][Y];
tempMatrix[Y][Y] = rowMatrix[Y][Y];
tempMatrix[Y][Z] = rowMatrix[Z][Y];
tempMatrix[Y][W] = rowMatrix[W][Y];

tempMatrix[Z][X] = rowMatrix[X][Z];
tempMatrix[Z][W] = rowMatrix[W][Z];
tempMatrix[Z][Y] = rowMatrix[Y][Z];
tempMatrix[Z][Z] = rowMatrix[Z][Z];

qp_matrix_copy(pMatrix, tempMatrix);

}	/* qp_row_to_pmatrix */



/*****************************************************************************
 *
   qp_matrix_mult - multiply two pphigs matrices
 
    input:
    	- result matrix (can be the same as either input matrix)
	- first and second matrices
    
    output:
    	- result = A*B
    
    notes:
    	- this code stolen from pphigs source
 *
 *****************************************************************************/

void
qp_matrix_mult(Q_MatrixType result, Q_MatrixType a, Q_MatrixType b)
{

    int 	i;
    Q_MatrixType	temp;


for ( i = 0; i < 3; i++ ) 
    {
    temp[i][0] = a[i][0] * b[0][0] + a[i][1] * b[1][0] + a[i][2] * b[2][0];
    temp[i][1] = a[i][0] * b[0][1] + a[i][1] * b[1][1] + a[i][2] * b[2][1];
    temp[i][2] = a[i][0] * b[0][2] + a[i][1] * b[1][2] + a[i][2] * b[2][2];
    temp[i][3] = a[i][0] * b[0][3] + a[i][1] * b[1][3] + a[i][2] * b[2][3] +
      	    	    a[i][3];
    }

qp_matrix_copy(result, temp);

}	/* qp_matrix_mult */



/*****************************************************************************
 *
   qp_matrix_copy - copy dest to src, both PPHIGS matrices
 *
 *****************************************************************************/

void
qp_matrix_copy(Q_MatrixType destMatrix,Q_MatrixType srcMatrix)
{
    int	    i, j;

/* 3 rows   */
for ( i = 0; i < 3; i++ )
    /* 4 columns    */
    for ( j = 0; j < 4; j++ )
	destMatrix[i][j] = srcMatrix[i][j];

}	/* qp_matrix_copy */



/*****************************************************************************
 *
   qp_matrix_3x3_determinant - calc determinant of the rotation part of
    	    	    	    	    a pphigs matrix
 *
 *****************************************************************************/

double
qp_matrix_3x3_determinant(Q_MatrixType mat)
{

    double  row0, row1, row2;

    
row0 = mat[0][0] * ( mat[1][1]*mat[2][2] - mat[1][2]*mat[2][1] );
row1 = mat[0][1] * ( mat[1][0]*mat[2][2] - mat[1][2]*mat[2][0] );
row2 = mat[0][2] * ( mat[1][0]*mat[2][1] - mat[1][1]*mat[2][0] );

return( row0 - row1 + row2 );

}	/* qp_matrix_3x3_determinant */



/*****************************************************************************
 *
    qp_pmatrix_to_row_matrix- converts PPHIGS matrix to row matrix
 *
 *****************************************************************************/

void
qp_pmatrix_to_row_matrix(q_matrix_type rowMatrix, Q_MatrixType pMatrix)
{
    q_matrix_type   tempMatrix;

/* transpose and copy; last column is constant 	*/
tempMatrix[X][X] = pMatrix[X][X];
tempMatrix[X][Y] = pMatrix[Y][X];
tempMatrix[X][Z] = pMatrix[Z][X];
tempMatrix[X][W] = 0;

tempMatrix[Y][X] = pMatrix[X][Y];
tempMatrix[Y][Y] = pMatrix[Y][Y];
tempMatrix[Y][Z] = pMatrix[Z][Y];
tempMatrix[Y][W] = 0;

tempMatrix[Z][X] = pMatrix[X][Z];
tempMatrix[Z][Y] = pMatrix[Y][Z];
tempMatrix[Z][Z] = pMatrix[Z][Z];
tempMatrix[Z][W] = 0;

tempMatrix[W][X] = pMatrix[X][W];
tempMatrix[W][Y] = pMatrix[Y][W];
tempMatrix[W][Z] = pMatrix[Z][W];
tempMatrix[W][W] = 1;

q_matrix_copy(rowMatrix, tempMatrix);

}	/* qp_pmatrix_to_row_matrix */




/*****************************************************************************
 *
   qp_invert_matrix - inverts a PPHIGS matrix
 *
 *****************************************************************************/

void
qp_invert_matrix(Q_MatrixType invertedMatrix,Q_MatrixType srcMatrix)

{

    int	       	i, j;
    Q_MatrixType copy;	/* temp copy of dest in case src == dest    */


/* invert translation offsets   */
for ( i = 0; i < 3; i++ )
    copy[i][3] = -srcMatrix[0][i] * srcMatrix[0][3] 
	    	 -srcMatrix[1][i] * srcMatrix[1][3]
	    	 -srcMatrix[2][i] * srcMatrix[2][3];

/* transpose rotation part	*/
for ( i = 0; i < 3; i++ )
    for ( j = 0; j < 3; j++ )
    	copy[i][j] = srcMatrix[j][i];

for ( i = 0; i < 3; i++ )
    {
    for ( j = 0; j < 3; j++ )
    	invertedMatrix[i][j] = copy[i][j];
    
    invertedMatrix[i][3] = copy[i][3];
    }

}	/* qp_invert_matrix */


/*****************************************************************************
 *
   q_matrix_mult - does a 4x4 matrix multiply (the input matrices are 4x4) and
    	    	  puts the result in a 4x4 matrix.  src == dest ok.
 *
 *****************************************************************************/

void
q_matrix_mult(q_matrix_type resultMatrix,q_matrix_type leftMatrix,
			  q_matrix_type rightMatrix)
{
    int	    	    i;
    int	    	    r, c;
    q_matrix_type   tmpResultMatrix;


/* pick up a row of the multiplier matrix and multiply by a column of the
 *  multiplicand
 */
for ( r = 0; r < 4; r++ )
    /* multiply each colum of the multiplicand by row r of the multiplier   */
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


q_matrix_copy(resultMatrix, tmpResultMatrix);

}	/* qp_matrix_mult */

/* multiplication of OpenGL-style matrices */
void
qogl_matrix_mult( qogl_matrix_type result,qogl_matrix_type left,
				 qogl_matrix_type right )
{
    int i, r, c;
    qogl_matrix_type tmp;

    /* straightforward matrix multiplication */
    for( r = 0; r < 4; r++ )
      for( c = 0; c < 4; c++ ) {
	tmp[r*4+c] = 0.0;
	for( i = 0; i < 4; i++ )
	  tmp[r*4+c] += left[r*4+i] * right[i*4+c];
      }

    qogl_matrix_copy( result, tmp );
    return;
}

/*****************************************************************************
 *
   q_matrix_copy - copies srcMatrix to destMatrix (both matrices are 4x4) 
 *
 *****************************************************************************/

void
q_matrix_copy(q_matrix_type destMatrix, q_matrix_type srcMatrix)
{

    int	    i, j;

for ( i = 0; i < 4; i++ )
    for ( j = 0; j < 4; j++ )
    	destMatrix[i][j] = srcMatrix[i][j];

}	/* q_matrix_copy */

/* qogl_matrix_copy - copies src to dest, both OpenGL matrices */
void
qogl_matrix_copy(qogl_matrix_type  dest, qogl_matrix_type src )
{
    int i;

    for( i = 0; i < 16; i++ )
      dest[i] = src[i];

    return;
}

/*****************************************************************************
 *
   qgl_print_matrix - print gl-style matrix
 *
 *****************************************************************************/

void
qgl_print_matrix(qgl_matrix_type matrix)
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

/* print OpenGL-style matrix */
void
qogl_print_matrix(qogl_matrix_type m )

{
    int i, j;

    for( j = 0; j < 4; j++ ) {
      for( i = 0; i < 4; i++ )
	printf( "%10lf", m[i*4+j] );
      printf( "\n" );
    }

    return;
} /* qogl_print_matrix */
