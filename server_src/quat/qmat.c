

/*****************************************************************************
 *
    qmat.c - matrix ops tool
	    
    rich holloway,  7/18/93
	    
 *
 *****************************************************************************/



#include <stdio.h>
#include "quat.h"


int
main(argc, argv)
    
    short	argc;
    char	*argv[];

{
    static char	    	    cmdStr[80] = ' ';	/* anything but 'q' */


while ( cmdStr[0] != 'q' )
    {
    fprintf(stderr, "Command [h- help, q- quit]: ");
    scanf("%s", cmdStr);
    doCommand(cmdStr);
    }

return(0);

}	/* main */



/*****************************************************************************
 *
   doCommand - 
 *
 *****************************************************************************/

int
doCommand(cmdStr)
    char    cmdStr[];

{

switch ( cmdStr[0] )
    {
    case 'h':
    case '?':
    	doHelp();
	break;
    
    case 'i':
    	invertPmatrix();
	break;
    
    case 'm':
    	matrixToQuat();
	break;
	
    case 'q':
    	return;
    
    case 'u':
    	quatToMatrix();
	break;
    }

}	/* doCommand */



/*****************************************************************************
 *
   doHelp - 
 *
 *****************************************************************************/

int
doHelp()

{

fprintf(stderr, "Commands:\n");
fprintf(stderr, "h- help\n");
fprintf(stderr, "m- matrix to quat\n");
fprintf(stderr, "i- invert\n");
fprintf(stderr, "q- quit\n");
fprintf(stderr, "u- quat to matrix\n");
fprintf(stderr, "?- help\n");

}	/* doHelp */



/*****************************************************************************
 *
   readPmatrix - 
 *
 *****************************************************************************/

int
readPmatrix(matrix)
    Q_MatrixType   matrix;
{


fprintf(stderr, "\nEnter PPHIGS matrix:\n");
scanf("%f%f%f%f %f%f%f%f %f%f%f%f",
	&matrix[0][0],
	&matrix[0][1],
	&matrix[0][2],
	&matrix[0][3],
	
	&matrix[1][0],
	&matrix[1][1],
	&matrix[1][2],
	&matrix[1][3],
	
	&matrix[2][0],
	&matrix[2][1],
	&matrix[2][2],
	&matrix[2][3] );

fprintf(stderr, "mat in =\n");
qp_print_matrix(matrix);

}	/* readPmatrix */



/*****************************************************************************
 *
   invertPmatrix - 
 *
 *****************************************************************************/

int
invertPmatrix()

{
    Q_MatrixType  pMatrix, invMatrix;

readPmatrix(pMatrix);
qp_invert_matrix(invMatrix, pMatrix);

fprintf(stderr, "Inverted pMatrix:\n");
qp_print_matrix(invMatrix);

}	/* invertPmatrix */


/*****************************************************************************
 *
   matrixToQuat - 
 *
 *****************************************************************************/

int
matrixToQuat()

{
    Q_MatrixType    pMatrix;
    q_type  	    quat;

readPmatrix(pMatrix);
qp_from_matrix(quat, pMatrix);

fprintf(stderr, "Quat:\n");
q_print(quat);

}	/* matrixToQuat */

/*****************************************************************************
 *
   quatToMatrix - 
 *
 *****************************************************************************/

int
quatToMatrix()

{
    Q_MatrixType    pMatrix;
    q_type  	    quat;

readQuat(quat);
qp_to_matrix(pMatrix, quat);

fprintf(stderr, "PPHIGS matrix:\n");
qp_print_matrix(pMatrix);

}	/* quatToMatrix */


/*****************************************************************************
 *
   readQuat - 
 *
 *****************************************************************************/

int
readQuat(quat)
    q_type  quat;
{


fprintf(stderr, "\nEnter quaternion:\n");
scanf("%lf%lf%lf%lf",
	&quat[0],
	&quat[1],
	&quat[2],
	&quat[3]);

fprintf(stderr, "quat in =\n");
q_print(quat);

}	/* readQuat */

