

/*****************************************************************************
 *
    qpmult.c - mult 2 pmatrices
	   
    rich holloway,  7/26/90
	    
 *
 *****************************************************************************/



#include <stdio.h>
#include "quat.h"

typedef float MatrixType[3][4]; /* Transformation Matrix */


int
main(argc, argv)
    
    short	argc;
    char	*argv[];

{
    
    int	    	i, j;
    double  	mag;
    q_vec_type	vec;
    MatrixType	A, B, result;


printf("\nEnter 1st pphigs matrix: \n");
/* 3 rows, 4 columns	*/
for ( i = 0; i < 3; i++ )
    {
    for ( j = 0; j < 4; j++ )
	scanf("%f", &A[i][j]);
    }

printf("\nEnter 2nd pphigs matrix: \n");
/* 3 rows, 4 columns	*/
for ( i = 0; i < 3; i++ )
    {
    for ( j = 0; j < 4; j++ )
	scanf("%f", &B[i][j]);
    }

qp_matrix_mult(result, A, B);

/* mag of one axis = mag of all	*/
mag = qp_matrix_3x3_determinant(result);

printf("result determinant = %lf\n", mag);

printf("result:\n");
qp_print_matrix(result);

/* put each row into a qvec	*/
for ( j = 0; j < 3; j++ )
    vec[j] = result[0][j];

/* mag of one axis = mag of all	*/
mag = q_vec_magnitude(vec);

printf("result scale = %lf\n", mag);

/* normalize matrix */
for ( i = 0; i < 3; i++ )
    for ( j = 0; j < 3; j++ )
	result[i][j] /= mag;

printf("normalized result:\n");
qp_print_matrix(result);

/* put each row into a qvec	*/
for ( j = 0; j < 3; j++ )
    vec[j] = result[0][j];

/* mag of one axis = mag of all	*/
mag = q_vec_magnitude(vec);

printf("normalized mag = %lf\n", mag);

}	/* main */

