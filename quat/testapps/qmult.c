/*****************************************************************************
 *
    qmult.c - multiplies two quaternions and shows result as a matrix and
    	    	a quat
    
    	Session:
	
	    - Enter 2 quaternions [(x, y, z), w] (no punctuation)
	    - Both are normalized and echoed
	    - Multiply 2 quats and print
	    - Print matrix for product
	    
    rich holloway,  9/25/90
	    
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

   q_type	multQuat;
   q_type	candQuat;
   q_type	invertedQuat;
   q_type	productQuat;
    
   double  	matrix[4][4];

   /*
    * read in, echo, and normalize 2 quaternions
    */
   printf("\nEnter multiplier: (vec, s) ");
   scanf("%lf %lf %lf %lf", 
         &multQuat[0], &multQuat[1], &multQuat[2], &multQuat[3]);
   q_normalize(multQuat, multQuat);
   printf("Multiplier quat = \n");
   q_print(multQuat);

   /* get and normalize inverse of 1st quaternion	*/
   q_invert(invertedQuat, multQuat);
   q_normalize(invertedQuat, invertedQuat);
   printf("Inverse = \n");
   q_print(invertedQuat);

   printf("Enter multiplicand: ");
   scanf("%lf %lf %lf %lf", 
         &candQuat[0], &candQuat[1], &candQuat[2], &candQuat[3]);
   q_normalize(candQuat, candQuat);
   printf("Multiplicand quat = \n");
   q_print(candQuat);

   /* 
    * productQuat = multQuat * candQuat   
    */
   q_mult(productQuat, multQuat, candQuat);
   q_normalize(productQuat, productQuat);
   printf("Product = \n");
   q_print(productQuat);

   /*
    * matrix of product quat
    */
   q_to_col_matrix(matrix, productQuat);
   printf("Matrix:\n");
   q_print_matrix(matrix);

   return(0);

}	/* main */

