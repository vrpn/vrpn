/*****************************************************************************
 *
    xyzquat.c - source for all operations related to the xyzquat data type

    Revision History:

    Author	    	Date  	 Comments
    ------	    	--------  ----------------------------
    Erik Erikson  06/26/92  Added q_xyz_quat_compose
    Stefan Gottschalk
    Russ Taylor
    Rich Holloway	01/25/91  Initial version


   Developed at the University of North Carolina at Chapel Hill, supported
   by the following grants/contracts:
   
     DARPA #DAEA18-90-C-0044
     ONR #N00014-86-K-0680
     NIH #5-R24-RR-02170
 *
 *****************************************************************************/

#include "quat.h"


/*****************************************************************************
 *
   q_xyz_quat_invert - invert a vector/quaternion transformation pair 
 
    input:
    	- dest and source pointers
    
    output:
    	- src is inverted and put into dest
    
    notes:
    	- src and dest may be same
 *
 *****************************************************************************/
void q_xyz_quat_invert(q_xyz_quat_type *destPtr, const q_xyz_quat_type *srcPtr)
{

   /* invert rotation first  */
   q_invert(destPtr->quat, srcPtr->quat);

   /* vec = -vec	*/
   q_vec_invert(destPtr->xyz, srcPtr->xyz);

   /* rotate translation offsets into inverted system	*/
   q_xform(destPtr->xyz, destPtr->quat, destPtr->xyz);


}	/* q_xyz_quat_invert */



/*****************************************************************************
 *
   q_row_matrix_to_xyz_quat - converts a row matrix to an xyz_quat
 
    input:  
    	- pointer to each
    
    output:
    	- new xyz_quat
    
    notes:
    	- each call to this function takes about .1 milliseconds on pxpl4
	    a VAX 3200 GPX, as of 4/11/91.
 *
 *****************************************************************************/
void q_row_matrix_to_xyz_quat(q_xyz_quat_type *xyzQuatPtr, const q_matrix_type rowMatrix)
{
   int	    i;

   q_from_row_matrix(xyzQuatPtr->quat, rowMatrix);

   for ( i = 0; i < 3; i++ )
      xyzQuatPtr->xyz[i] = rowMatrix[3][i];

}	/* q_row_matrix_to_xyz_quat */


/*****************************************************************************
 *
   q_xyz_quat_to_row_matrix - convert an xyz_quat to a row matrix
 
    input:
    	- pointer to each
    
    output:
    	- new row matrix
    
    notes:
    	- each call to this function takes about .14 milliseconds on pxpl4
	    a VAX 3200 GPX, as of 4/11/91.
 *
 *****************************************************************************/
void q_xyz_quat_to_row_matrix(q_matrix_type rowMatrix, const q_xyz_quat_type *xyzQuatPtr)
{

   int	    i;


   q_to_row_matrix(rowMatrix, xyzQuatPtr->quat);

   for ( i = 0; i < 3; i++ )
      rowMatrix[3][i] = xyzQuatPtr->xyz[i];

}	/* q_xyz_quat_to_row_matrix */


/*
 * q_ogl_matrix_to_xyz_quat
 */
void q_ogl_matrix_to_xyz_quat(q_xyz_quat_type *xyzQuatPtr, const qogl_matrix_type matrix )
{
   q_from_ogl_matrix( xyzQuatPtr->quat, matrix );
   xyzQuatPtr->xyz[Q_X] = matrix[12+Q_X];
   xyzQuatPtr->xyz[Q_Y] = matrix[12+Q_Y];
   xyzQuatPtr->xyz[Q_Z] = matrix[12+Q_Z];
}

/*
 * q_xyz_quat_to_ogl_matrix
 */
void q_xyz_quat_to_ogl_matrix(qogl_matrix_type matrix, const q_xyz_quat_type *xyzQuatPtr )
{
   q_to_ogl_matrix( matrix, xyzQuatPtr->quat );
   matrix[12+Q_X] = xyzQuatPtr->xyz[Q_X];
   matrix[12+Q_Y] = xyzQuatPtr->xyz[Q_Y];
   matrix[12+Q_Z] = xyzQuatPtr->xyz[Q_Z];
}


/*****************************************************************************
 *
   q_xyz_quat_compose - compose q_xyz_quat_types CfromB and BfromA to 
                        get CfromA

    input:
        - pointers to q_xyz_quat_types CfromA, CfromB and BfromA

    output:
        - CfromA

    overview:
        - treat the BA q_xyz_quat_type as local, since it's closer to 
	    the point:

            newp = CB*BA*p

        - first xform the xlate part of BA into the CB system by rotating
            and scaling it

        - add the xformed local xlate to the unchanged global xlate

        - then compose the rotation parts by multiplying global*local
 *
 *****************************************************************************/
void q_xyz_quat_compose(q_xyz_quat_type *C_from_A_ptr, 
                        const q_xyz_quat_type *C_from_B_ptr, const q_xyz_quat_type *B_from_A_ptr)
{
   q_vec_type rotated_BA_vec;


   /* rotate local xlate into global   */
   q_xform(rotated_BA_vec, C_from_B_ptr->quat, B_from_A_ptr->xyz);

   /* now add the xformed local vec to the unchanged global vec    */
   q_vec_add(C_from_A_ptr->xyz, C_from_B_ptr->xyz, rotated_BA_vec);

   /* compose the rotations    */
   /* CA_rotate = CB_rotate . BA_rotate */
   q_mult(C_from_A_ptr->quat, C_from_B_ptr->quat, B_from_A_ptr->quat);

   q_normalize(C_from_A_ptr->quat, C_from_A_ptr->quat);

} 	/* q_xyz_quat_compose */


// xform a vector by an xyzquat.  dest == src ok.
void q_xyz_quat_xform(q_vec_type dest, const q_xyz_quat_type *xf, const q_vec_type src)
{
   q_xform(dest, xf->quat, src);
   q_vec_add(dest, xf->xyz, dest);
}


