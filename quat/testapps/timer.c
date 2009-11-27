#include <stdlib.h>
#include <stdio.h>
#include "quat.h"

int
main(argc, argv)
    
    short	argc;
    char	*argv[];

{

    int	    	i, j;
    int	    	numCycles;
    
    static q_type  	    quat;
    static q_matrix_type   matrix = 
			    { 0.475299, 0.278958, 0.834430, 0.000000 ,
			      0.314176, 0.832064 -0.457125, 0.000000 ,
			     -0.821818, 0.479428, 0.307837, 0.000000 ,
			      0.000000, 0.000000, 0.000000, 1.000000 };

    static q_xyz_quat_type xyzQuat = { {44.6, 99999, -889.9}, 
    	    	    	    	       {78.9, .3, -322, 1} };


    printf("starting...\n");

   /* parse command line	*/
    for ( i = 1; i < argc; i++ ) 
    {
       /* command line flags	*/
       if ( argv[i][0] == '-' ) 
       {
          switch ( argv[i][1] )
          {
             case 'c':
                /* numeric arg follows	*/
                i++;
                numCycles = atoi(argv[i]);
                printf("%d cycles\n", numCycles);
                break;
             default:
                /* bogus flag value	*/
                fprintf(stderr, "Bogus argument: '%s'\n", argv[i] );
                exit(-1);
          }
       }
       else
       {
          fprintf(stderr, "bogus arg '%s'\n", argv[i]);
       }
    }

    for ( i = 0; i < numCycles; i++ )
    {
       j = i;
       q_xyz_quat_to_row_matrix(matrix, &xyzQuat);
    }

    printf("done, j = %d.\n", j);

    return(0);

}	/* main */
