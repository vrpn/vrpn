
/* prevent multiple includes	*/
#ifndef Q_INCLUDED
#define Q_INCLUDED


/*****************************************************************************
 *
    quat.h -  include file for quaternion, vector and matrix routines.  

    
    Overview:
    
    	quatlib is a library of routines that implements a grab-bag of 
	useful routines for dealing with quaternions, vectors, and 
	matrices.  See the quatlib man page for an overview.


    Notes:

      - to address the quaternion elements, use the Q_X, Q_Y, Q_Z and Q_W
    	#defines from this file, or X,Y,Z,W from pphigs.h, although the
	latter is not guaranteed to work forever.
      
      - to find out which version of the library you're using, do:
	    
      	    % ident  <path>/libquat.a
	
	(this information is in the rcsid string in quat.c)
    
      - see /afs/unc/proj/hmd/src/quat/{quat,vector,matrix}.c 
        for implementation details.


    Conventions:

      - general-purpose quaternion routines start with q_

      - pphigs-specific routines start with qp_ and expect data
	   types as defined in pphigs.h (vectors are currently 
	   arrays of floats)

      - all non-integer values are doubles by default-  the exceptions
      	to this are routines dealing with PPHIGS vectors and matrices,
	which use floats.
      
      - vector routines have the string "vec" somewhere in their name
      
      - matrix routines have the string "matrix" somewhere in their name

      - all matrices are 4x4 unless otherwise noted by qp_ prefix

      - positive rotation directions are as follows:

		about Z axis: from X axis to Y axis
		about X axis: from Y axis to Z axis
		about Y axis: from Z axis to X axis

      - all angles are specified in radians

      - destination parameter (if any) is always first argument (as in
      	    Unix string routines)

      - src and dest parameters can always be the same, as long as they 
      	    are of the same type (copying is done if necessary) 

      - naming conventions for conversion routines:
      
	    q{p}_{to,from}_whatever for routines involving quaternions
	    q{p}_x_to_y for all others (ie., no "from" is used)


   RCS Header:
   $Id: quat.h,v 1.1 1997/07/25 16:44:12 chenju Exp $
   
   Revision History (for whole library, not just this file):

   Author	    	Date	  Comments
   ------	    	--------  ----------------------------
   Mark Livingston      01/09/96   Added routines for OpenGL matrices
   Rich Holloway    	09/27/93   Added Gary Bishop's matrix to euler rtn
   Rich Holloway    	07/16/92   Added q_euler_to_col_matrix(), routines
    	    	    	    	    for working with GL matrices, added
				    documentation for euler angle routines
   Erik Erikson/        06/26/92   Added q_xyz_quat_compose
   Stefan Gottschalk/
   Russ Taylor
   Rich Holloway    	05/13/92   Added Q_NULL_VECTOR, Q_ID_MATRIX
   Jon Leech/           04/29/92   Added CM_ prototypes
   Erik Erikson
   
   Rich Holloway	09/21/90   Made into library, made all matrices 4x4,
  	    	    	    	    added matrix routines for 
    	    	    	    	    4x4 (standard) or 3x4 (for PPHIGS),
				    changed names of 
    	    	    	    	    routines (to avoid name conflicts with
				    non-library routines) by prefixing
				    everything with "q_".
   
   Russ Taylor	    	 1990	   Modified q_slerp to pick shortest path
    	    	    	    	    between two angles
   
   Warren Robinett  	12/89 	   Added PPHIGS support routines
   
   Ken Shoemake	    	 1985      Initial version
 
 *
 *****************************************************************************/


#include <stdio.h>
#include <math.h>
#include <cm_macros.h>
#include <pdefs.h>

/*****************************************************************************
 *
    #defines
 *
 *****************************************************************************/

/* for accessing the elements of q_type and q_vec_type	*/
#define Q_X 	0
#define Q_Y 	1
#define Q_Z 	2
#define Q_W 	3

/* tolerance for quaternion operations	*/
#define	Q_EPSILON   (1e-10)

/* min and max macros	*/
#define Q_MAX(x, y)       ( ((x) > (y)) ? (x) : (y) )
#define Q_MIN(x, y)       ( ((x) < (y)) ? (x) : (y) )

#define Q_ABS(x)       ( ((x) > 0 ) ? (x) : (-(x)) )

/* 
 * use local definition of PI for machines that have no def in math.h; this
 *  value stolen from DEC Ultrix 4.1 math.h
 */
#define Q_PI    3.14159265358979323846

#define Q_ID_QUAT   { 0.0, 0.0, 0.0, 1.0 }

#define Q_ID_MATRIX { {1.0, 0.0, 0.0, 0.0}, \
    	    	      {0.0, 1.0, 0.0, 0.0}, \
		      {0.0, 0.0, 1.0, 0.0}, \
		      {0.0, 0.0, 0.0, 1.0} }

#define QP_ID_MATRIX { {1.0, 0.0, 0.0, 0.0}, \
    	    	       {0.0, 1.0, 0.0, 0.0}, \
		       {0.0, 0.0, 1.0, 0.0} }

#define Q_NULL_VECTOR	{ 0.0, 0.0, 0.0 }

/* 
 * degree/radian conversion
 */
#define Q_DEG_TO_RAD(deg)       ( ((deg)*Q_PI)/180.0 )
#define Q_RAD_TO_DEG(rad)       ( (((rad)*180.0)/Q_PI) )

/* compatibility w/ old for a while */
#define q_vec_set   q_set_vec


/*****************************************************************************
 *
    typedefs
 *
 *****************************************************************************/

/* basic quaternion type- scalar part is last element in array    */
typedef double	q_type[4];

/* basic vector type	*/
typedef double	q_vec_type[3];

/* for row and column matrices	*/
typedef double	q_matrix_type[4][4];

/* for working with gl or other 4x4 float matrices  */
typedef float	qgl_matrix_type[4][4];

/* for working with OpenGL matrices - these are really just like row matrices
** (i.e. same bits in same order), but the decl is a 1-D array, not 2-D, sigh
*/
typedef double  qogl_matrix_type[16];

/* special transformation type using quaternions and vectors	*/
typedef struct  q_xyz_quat_struct
    {
    q_vec_type	xyz;  /* translation  */
    q_type  	quat;  /* rotation	    */
    } q_xyz_quat_type;



/*****************************************************************************
 *****************************************************************************
 *
    function declarations
 *
 *****************************************************************************
 *****************************************************************************/



/*****************************************************************************
 *
    strictly quaternion operations
 *
 *****************************************************************************/

/*  prints a quaternion	*/
CM_EXTERN_FUNCTION( void q_print, ( q_type quat ));

/* make a quaternion given an axis and an angle;  x,y,z is axis of 
 *  rotation;  angle is angle of rotation in radians (see also q_from_two_vecs)
 *
 *  rotation is counter-clockwise when rotation axis vector is 
 *	    	pointing at you
 *
 * if angle or vector are 0, the identity quaternion is returned.
 */
CM_EXTERN_FUNCTION( void q_make, ( q_type destQuat,
				   double x,  double y,  double z,
				   double angle ));

/*  copy srcQuat to destQuat    */
CM_EXTERN_FUNCTION( void q_copy, ( q_type destQuat, q_type srcQuat ));

/* normalizes quaternion;  src and dest can be same */
CM_EXTERN_FUNCTION( void q_normalize, ( q_type destQuat, 
				        q_type srcQuat ));

/* invert quat;  src and dest can be the same	*/
CM_EXTERN_FUNCTION( void q_invert, ( q_type destQuat, q_type srcQuat ));

/*
 * computes quaternion product destQuat = qLeft * qRight.
 *  	    destQuat can be same as either qLeft or qRight or both.
 */
CM_EXTERN_FUNCTION( void q_mult, ( q_type destQuat, 
				   q_type qLeft,  
				   q_type qRight ));

/* conjugate quat; src and dest can be same */
CM_EXTERN_FUNCTION( void q_conjugate, ( q_type destQuat, 
				        q_type srcQuat ));

/* take natural log of unit quat; src and dest can be same  */
CM_EXTERN_FUNCTION( void q_log, ( q_type destQuat, 
				  q_type srcQuat ));

/* exponentiate quaternion, assuming scalar part 0.  src can be same as dest */
CM_EXTERN_FUNCTION( void q_exp, ( q_type destQuat, 
				  q_type srcQuat ));


/*
 * q_slerp: Spherical linear interpolation of unit quaternions.
 *
 *  	As t goes from 0 to 1, destQuat goes from startQ to endQuat.
 *      This routine should always return a point along the shorter
 *  	of the two paths between the two.  That is why the vector may be
 *  	negated in the end.
 *  	
 *  	src == dest should be ok, although that doesn't seem to make much
 *  	sense here.
 */
CM_EXTERN_FUNCTION( void q_slerp, ( q_type       destQuat, 
				    q_type startQuat,
				    q_type endQuat, 
				    double       t ));

/*****************************************************************************
 *  
    q_from_euler - converts 3 euler angles (in radians) to a quaternion
     
	Assumes roll is rotation about X, pitch
	is rotation about Y, yaw is about Z.  Assumes order of 
	yaw, pitch, roll applied as follows:
	    
	    p' = roll( pitch( yaw(p) ) )

    	See comments for q_euler_to_col_matrix for more on this.
 *
 *****************************************************************************/
CM_EXTERN_FUNCTION( void q_from_euler, ( q_type destQuat, 
					 double yaw, 
					 double pitch, 
					 double roll ));


/*****************************************************************************
 *
    mixed quaternion operations:  conversions to and from vectors & matrices
 *
 *****************************************************************************/

/* destVec = q * vec * q(inverse);  vec can be same storage as destVec	*/
CM_EXTERN_FUNCTION( void q_xform, ( q_vec_type destVec, 
				    q_type q,
				    q_vec_type vec ));

/* quat/vector conversion	*/
/* create a quaternion from two vectors that rotates v1 to v2 
 *   about an axis perpendicular to both
 */
CM_EXTERN_FUNCTION( void q_from_two_vecs, ( q_type destQuat,
					    q_vec_type v1, 
					    q_vec_type v2 ));

/* simple conversion	*/
CM_EXTERN_FUNCTION( void q_from_vec, ( q_type destQuat, 
				       q_vec_type srcVec ));
CM_EXTERN_FUNCTION( void q_to_vec, ( q_vec_type destVec, 
				     q_type srcQuat ));

/* quaternion/4x4 matrix conversions	*/
CM_EXTERN_FUNCTION( void q_from_row_matrix, ( q_type destQuat, 
					      q_matrix_type matrix ));
CM_EXTERN_FUNCTION( void q_from_col_matrix, ( q_type destQuat, 
					      q_matrix_type matrix ));
CM_EXTERN_FUNCTION( void q_to_row_matrix, ( q_matrix_type destMatrix, 
					    q_type srcQuat ));
CM_EXTERN_FUNCTION( void q_to_col_matrix, ( q_matrix_type destMatrix, 
					    q_type srcQuat ));

/* quat/pphigs conversion   */
CM_EXTERN_FUNCTION( void qp_to_matrix, ( Q_MatrixType destMatrix, 
					 q_type srcQuat ));
CM_EXTERN_FUNCTION( void qp_from_matrix, ( q_type destQuat, 
					   Q_MatrixType srcMatrix ));

/* quat/ogl conversion */
CM_EXTERN_FUNCTION( void q_from_ogl_matrix, ( q_type destQuat,
					      qogl_matrix_type matrix ));
CM_EXTERN_FUNCTION( void q_to_ogl_matrix, ( qogl_matrix_type matrix,
					    q_type srcQuat ));

/*****************************************************************************
 *
    strictly vector operations
 *
 *****************************************************************************/


/* prints a vector to stdout  */
CM_EXTERN_FUNCTION( void q_vec_print, ( q_vec_type  vec ));

/* sets vector equal to 3 values given	*/
CM_EXTERN_FUNCTION( void q_set_vec, ( q_vec_type vec, 
				      double x, double y, double z ));

/* copies srcVec to destVec */
CM_EXTERN_FUNCTION( void q_vec_copy, ( q_vec_type destVec, 
				       q_vec_type srcVec ));

/* adds two vectors */
CM_EXTERN_FUNCTION( void q_vec_add, ( q_vec_type destVec, 
				      q_vec_type aVec, 
				      q_vec_type bVec ));

/* destVec = v1 - v2 (v1, v2, destVec need not be distinct storage) */
CM_EXTERN_FUNCTION( void q_vec_subtract, ( q_vec_type       destVec, 
					   q_vec_type v1, 
					   q_vec_type v2 ));

/* returns value of dot product of v1 and v2	*/
CM_EXTERN_FUNCTION( double q_vec_dot_product, ( q_vec_type v1, 
					        q_vec_type v2 ));

/* scale a vector  (src and dest need not be distinct) */
CM_EXTERN_FUNCTION( void q_vec_scale, ( q_vec_type  	  destVec,
				        double  	  scaleFactor,
				        q_vec_type  srcVec ));


/* negate a vector to point in the opposite direction	*/
CM_EXTERN_FUNCTION( void q_vec_invert, ( q_vec_type destVec, 
					 q_vec_type srcVec ));

/*  normalize a vector  (destVec and srcVec may be the same) */
CM_EXTERN_FUNCTION( void q_vec_normalize, ( q_vec_type destVec, 
					    q_vec_type srcVec ));

/* returns magnitude of vector	*/
CM_EXTERN_FUNCTION( double q_vec_magnitude, ( q_vec_type  vec ));

/*  returns distance between two points/vectors	*/
CM_EXTERN_FUNCTION( double q_vec_distance, ( q_vec_type vec1, 
					     q_vec_type vec2 ));

/* computes cross product of two vectors:  destVec = aVec X bVec
 *  	destVec same as aVec or bVec ok
 */
CM_EXTERN_FUNCTION( void q_vec_cross_product, ( q_vec_type       destVec, 
					        q_vec_type aVec, 
					        q_vec_type bVec ));


/* copies PPHIGS 3-vectors */
CM_EXTERN_FUNCTION( void qp_vec_copy, ( Q_VectorType destVec, 
				        Q_VectorType srcVec ));

/* convert PPHIGS srcVec to quatlib destVec */
CM_EXTERN_FUNCTION( void qp_pvec_to_vec, ( q_vec_type destVec, 
					   Q_VectorType srcVec ));

/* convert quatlib srcVec to PPHIGS destVec */
CM_EXTERN_FUNCTION( void qp_vec_to_pvec, ( Q_VectorType destVec, 
					   q_vec_type srcVec ));


/*****************************************************************************
 *
    strictly matrix operations
 *
 *****************************************************************************/

/* q_matrix_copy - copies srcMatrix to destMatrix (both matrices are 4x4)   */
CM_EXTERN_FUNCTION( void q_matrix_copy, ( q_matrix_type destMatrix, 
					  q_matrix_type srcMatrix ));

/* copy dest to src, both PPHIGS matrices   */
CM_EXTERN_FUNCTION( void qp_matrix_copy, ( Q_MatrixType destMatrix, 
					   Q_MatrixType srcMatrix ));

CM_EXTERN_FUNCTION( void qogl_matrix_copy, ( qogl_matrix_type dest,
					    qogl_matrix_type src ));

/* does a 4x4 matrix multiply (the input matrices are 4x4) and
 *   	    	  puts the result in a 4x4 matrix.  src == dest ok.
 */
CM_EXTERN_FUNCTION( void q_matrix_mult, ( q_matrix_type resultMatrix,
					  q_matrix_type leftMatrix,
					  q_matrix_type rightMatrix ));

/* multiply two pphigs matrices	(dest = source ok)  */
CM_EXTERN_FUNCTION( void qp_matrix_mult, ( Q_MatrixType result, 
					   Q_MatrixType A, 
					   Q_MatrixType B ));

CM_EXTERN_FUNCTION( void qogl_matrix_mult, ( qogl_matrix_type result,
					     qogl_matrix_type left,
					     qogl_matrix_type right ));

/* calc determinant the rotation part of a pphigs matrix	*/
CM_EXTERN_FUNCTION( double qp_matrix_3x3_determinant, 
		   ( Q_MatrixType mat ));

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
CM_EXTERN_FUNCTION( void q_euler_to_col_matrix, ( q_matrix_type destMatrix,
						  double yaw, 
						  double pitch, 
						  double roll ));

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
CM_EXTERN_FUNCTION( void q_col_matrix_to_euler, ( q_vec_type 	angles,
            	    	    	    	    	  q_matrix_type colMatrix ));

/* prints 4x4 matrix	*/
CM_EXTERN_FUNCTION( void q_print_matrix, ( q_matrix_type matrix ));

/* prints PPHIGS 3x4 matrix */
CM_EXTERN_FUNCTION( void qp_print_matrix, ( Q_MatrixType matrix ));

CM_EXTERN_FUNCTION( void qogl_print_matrix, ( qogl_matrix_type ));

/* prints PPHIGS 3x4 matrix to file */
CM_EXTERN_FUNCTION( void qp_file_print_matrix, ( FILE 	      *filePtr,
    	    	    	    	    	    	 Q_MatrixType matrix ));

/* from 4x4 row matrix to PPHIGS 3x4 matrix	*/
CM_EXTERN_FUNCTION( void qp_row_to_pmatrix, ( Q_MatrixType          pMatrix,
					      q_matrix_type rowMatrix ));

/* converts PPHIGS matrix to row matrix	*/
CM_EXTERN_FUNCTION( void qp_pmatrix_to_row_matrix, 
		   ( q_matrix_type rowMatrix,
		     Q_MatrixType pMatrix ));


/* inverts a PPHIGS matrix  */
CM_EXTERN_FUNCTION( void qp_invert_matrix, ( Q_MatrixType invertedMatrix, 
					     Q_MatrixType srcMatrix ));



/*****************************************************************************
 *
    xyz_quat routines
 *
 *****************************************************************************/

/* invert a vector/quaternion transformation pair   */
CM_EXTERN_FUNCTION( void q_xyz_quat_invert, ( q_xyz_quat_type *destPtr, 
					      q_xyz_quat_type *srcPtr ));



/* converts a row matrix to an xyz_quat	*/
CM_EXTERN_FUNCTION( void q_row_matrix_to_xyz_quat, 
		   ( q_xyz_quat_type     *xyzQuatPtr,
		     q_matrix_type  rowMatrix ));

/* convert an xyz_quat to a row matrix	*/
CM_EXTERN_FUNCTION( void q_xyz_quat_to_row_matrix, 
		   ( q_matrix_type   rowMatrix,
		     q_xyz_quat_type *xyzQuatPtr ));

/* converts a pphigs matrix to an xyz_quat	*/
CM_EXTERN_FUNCTION( void qp_pmatrix_to_xyz_quat, 
		   ( q_xyz_quat_type *xyzQuatPtr,
		     Q_MatrixType pMatrix ));

/* convert an xyz_quat to a pphigs matrix	*/
CM_EXTERN_FUNCTION( void qp_xyz_quat_to_pmatrix, 
		   ( Q_MatrixType    	pMatrix,
		     q_xyz_quat_type	*xyzQuatPtr ));

CM_EXTERN_FUNCTION( void q_ogl_matrix_to_xyz_quat,
		    ( q_xyz_quat_type *xyzQuatPtr,
		      qogl_matrix_type matrix ));

CM_EXTERN_FUNCTION( void q_xyz_quat_to_ogl_matrix,
		    ( qogl_matrix_type matrix,
		      q_xyz_quat_type *xyzQuatPtr ));

/* compose q_xyz_quat_vecs to form a third. */
/* C_from_A_ptr may be = to either C_from_B_ptr or B_from_A_ptr (or both) */
CM_EXTERN_FUNCTION( void q_xyz_quat_compose, ( q_xyz_quat_type *C_from_A_ptr,
                                               q_xyz_quat_type *C_from_B_ptr,
                                               q_xyz_quat_type *B_from_A_ptr));

/*****************************************************************************
 *
    GL support
 *
 *****************************************************************************/

/* convert from quat to GL 4x4 float row matrix	*/
CM_EXTERN_FUNCTION( void  qgl_to_matrix,
		   (  qgl_matrix_type 	destMatrix,
	    	      q_type  	    	srcQuat ));


/* qgl_from_matrix- Convert GL 4x4 row-major rotation matrix to 
 * unit quaternion.
 *   	- same as q_from_row_matrix, except basic type is float, not double
 */
CM_EXTERN_FUNCTION( void  qgl_from_matrix,
		   (  q_type  	    	destQuat,
	    	      qgl_matrix_type 	srcMatrix ));

/* print gl-style matrix    */
CM_EXTERN_FUNCTION( void  qgl_print_matrix,
		   (  qgl_matrix_type 	matrix ));

#endif /* Q_INCLUDED */




