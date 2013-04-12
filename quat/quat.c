
/*****************************************************************************
 *
    quat.c-  code for quaternion-specific routines.

    (see quat.h for revision history and more documentation.)
 *
 *****************************************************************************/


#include "quat.h"

#include <math.h>                       // for sqrt, sin, cos, acos, asin, etc
#include <stdio.h>                      // for printf

/* define local X, Y, Z, W to override any external definition;  don't use
 *  Q_X, etc, because it makes this code a LOT harder to read;  don't use 
 *  pphigs definition of X-W since it could theoretically change.
 */

#undef X
#undef Y
#undef Z
#undef W

#define X      Q_X
#define Y      Q_Y
#define Z      Q_Z
#define W      Q_W


/*****************************************************************************
 *
   q_print- print a quaternion
 *
 *****************************************************************************/
void q_print(const q_type quat)
{
   printf("  [ (%lf, %lf, %lf), %lf ]\n", 
          quat[X], quat[Y], quat[Z], quat[W]);

}  /* q_print */




/*****************************************************************************
 * 
    q_xform: Compute quaternion product destQuat = q * vec * qinv.
          vec can be same storage as destVec
 *
 *****************************************************************************/
void q_xform(q_vec_type destVec, const q_type q, const q_vec_type vec)
{
   q_type  inverse;
   q_type  vecQuat;
   q_type  tempVecQuat;
   q_type  resultQuat;

   /* copy vector into temp quaternion for multiply   */
   q_from_vec(vecQuat, vec);

   /* invert multiplier */
   q_invert(inverse, q);

   /* do q * vec * q(inv)  */
   q_mult(tempVecQuat, q, vecQuat);
   q_mult(resultQuat, tempVecQuat, inverse);

   /* back to vector land  */
   q_to_vec(destVec, resultQuat);

}   /* q_xform */




/*****************************************************************************
 * q_mult: Compute quaternion product destQuat = qLeft * qRight.
 *        destQuat can be same as either qLeft or qRight or both.
 *****************************************************************************/
void q_mult(q_type destQuat, const q_type qLeft, const q_type qRight)
{
   q_type  tempDest;


   tempDest[W] = qLeft[W]*qRight[W] - qLeft[X]*qRight[X] - 
      qLeft[Y]*qRight[Y] - qLeft[Z]*qRight[Z];

   tempDest[X] = qLeft[W]*qRight[X] + qLeft[X]*qRight[W] + 
      qLeft[Y]*qRight[Z] - qLeft[Z]*qRight[Y];

   tempDest[Y] = qLeft[W]*qRight[Y] + qLeft[Y]*qRight[W] + 
      qLeft[Z]*qRight[X] - qLeft[X]*qRight[Z];

   tempDest[Z] = qLeft[W]*qRight[Z] + qLeft[Z]*qRight[W] + 
      qLeft[X]*qRight[Y] - qLeft[Y]*qRight[X];

   q_copy(destQuat, tempDest);

}   /* q_mult  */



/*****************************************************************************
 * q_copy: copy quaternion q to destQuat
 *****************************************************************************/
void q_copy(q_type destQuat, const q_type srcQuat)
{

   destQuat[X] = srcQuat[X];
   destQuat[Y] = srcQuat[Y];
   destQuat[Z] = srcQuat[Z];
   destQuat[W] = srcQuat[W];

}   /* q_copy  */


/*****************************************************************************
 * q_from_vec- convert vec to quaternion
 *****************************************************************************/
void q_from_vec(q_type destQuat, const q_vec_type vec)
{

   destQuat[X] = vec[X];
   destQuat[Y] = vec[Y];
   destQuat[Z] = vec[Z];
   destQuat[W] = 0.0;

}   /* q_from_vec   */


/*****************************************************************************
 * q_to_vec- convert quaternion to vector;  q[W] is ignored
 *****************************************************************************/
void q_to_vec(q_vec_type destVec, const q_type srcQuat)
{

   destVec[X] = srcQuat[X];
   destVec[Y] = srcQuat[Y];
   destVec[Z] = srcQuat[Z];

}   /* q_to_vec   */


/*****************************************************************************
 * q_normalize-  normalize quaternion.  src and dest can be same
 *****************************************************************************/
void q_normalize(q_type destQuat, const q_type srcQuat)
{
   double normalizeFactor;


   normalizeFactor = 1.0 / sqrt( srcQuat[X]*srcQuat[X] + srcQuat[Y]*srcQuat[Y] + 
                                 srcQuat[Z]*srcQuat[Z] + srcQuat[W]*srcQuat[W] );

   destQuat[X] = srcQuat[X] * normalizeFactor;
   destQuat[Y] = srcQuat[Y] * normalizeFactor;
   destQuat[Z] = srcQuat[Z] * normalizeFactor;
   destQuat[W] = srcQuat[W] * normalizeFactor;

}   /* q_normalize  */



/*****************************************************************************
 * q_conjugate- conjugate quaternion.  src == dest ok.
 *****************************************************************************/
void q_conjugate(q_type destQuat, const q_type srcQuat)
{

   destQuat[X] = -srcQuat[X];
   destQuat[Y] = -srcQuat[Y];
   destQuat[Z] = -srcQuat[Z];
   destQuat[W] =  srcQuat[W];

}   /* q_conjugate  */


/*****************************************************************************
 * q_invert: Invert quaternion; That is, form its multiplicative inverse.
 *          src == dest ok.
 *****************************************************************************/
void q_invert(q_type destQuat, const q_type srcQuat)
{
   double srcQuatNorm;

   srcQuatNorm = 1.0 / (srcQuat[X]*srcQuat[X] + srcQuat[Y]*srcQuat[Y] + 
                        srcQuat[Z]*srcQuat[Z] + srcQuat[W]*srcQuat[W]);

   destQuat[X] = -srcQuat[X] * srcQuatNorm;
   destQuat[Y] = -srcQuat[Y] * srcQuatNorm;
   destQuat[Z] = -srcQuat[Z] * srcQuatNorm;
   destQuat[W] =  srcQuat[W] * srcQuatNorm;

}   /* q_invert   */


/*****************************************************************************
 * q_exp: Exponentiate quaternion, assuming scalar part 0.  src == dest ok.
 *****************************************************************************/
void q_exp(q_type destQuat, const q_type srcQuat)
{
   double theta, scale;


   theta = sqrt(srcQuat[X]*srcQuat[X] + srcQuat[Y]*srcQuat[Y] + 
                srcQuat[Z]*srcQuat[Z]);

   if (theta > Q_EPSILON)
      scale = sin(theta)/theta;
   else
      scale = 1.0;

   destQuat[X] = scale*srcQuat[X];
   destQuat[Y] = scale*srcQuat[Y];
   destQuat[Z] = scale*srcQuat[Z];
   destQuat[W] = cos(theta);

}   /* q_exp   */



/*****************************************************************************
 * q_log: Take the natural logarithm of unit quaternion.  src == dest ok.
 *****************************************************************************/
void q_log(q_type destQuat, const q_type srcQuat)
{
   double theta, scale;


   scale = sqrt(srcQuat[X]*srcQuat[X] + srcQuat[Y]*srcQuat[Y] + 
                srcQuat[Z]*srcQuat[Z]);
   theta = atan2(scale, srcQuat[W]);

   if (scale > 0.0)
      scale = theta/scale;

   destQuat[X] = scale * srcQuat[X];
   destQuat[Y] = scale * srcQuat[Y];
   destQuat[Z] = scale * srcQuat[Z];
   destQuat[W] = 0.0;

}   /* q_log   */




/*****************************************************************************
 *
   q_slerp: Spherical linear interpolation of unit quaternions.
 
      As t goes from 0 to 1, destQuat goes from startQ to endQuat.
       This routine should always return a point along the shorter
      of the two paths between the two.  That is why the vector may be
      negated in the end.
      
      src == dest should be ok, although that doesn't seem to make much
      sense here.
 *
 *****************************************************************************/
void q_slerp(q_type destQuat, const q_type  startQuat, const q_type endQuat, double t)
{
   q_type  startQ;               /* temp copy of startQuat  */
   double  omega, cosOmega, sinOmega;
   double  startScale, endScale;
   int     i;


   q_copy(startQ, startQuat);

   cosOmega = startQ[X]*endQuat[X] + startQ[Y]*endQuat[Y] + 
      startQ[Z]*endQuat[Z] + startQ[W]*endQuat[W];

   /* If the above dot product is negative, it would be better to
    *  go between the negative of the initial and the final, so that
    *  we take the shorter path.  
    */
   if ( cosOmega < 0.0 ) 
   {
      cosOmega *= -1;
      for (i = X; i <= W; i++)
         startQ[i] *= -1;
   }

   if ( (1.0 + cosOmega) > Q_EPSILON ) 
   {
      /* usual case */
      if ( (1.0 - cosOmega) > Q_EPSILON ) 
      {
         /* usual case */
         omega = acos(cosOmega);
         sinOmega = sin(omega);
         startScale = sin((1.0 - t)*omega) / sinOmega;
         endScale = sin(t*omega) / sinOmega;
      } 
      else 
      {
         /* ends very close */
         startScale = 1.0 - t;
         endScale = t;
      }
      for (i = X; i <= W; i++)
         destQuat[i] = startScale*startQ[i] + endScale*endQuat[i];
   } 
   else 
   {
      /* ends nearly opposite */
      destQuat[X] = -startQ[Y];  
      destQuat[Y] =  startQ[X];
      destQuat[Z] = -startQ[W];  
      destQuat[W] =  startQ[Z];
    
      startScale = sin((0.5 - t) * Q_PI);
      endScale = sin(t * Q_PI);
      for (i = X; i <= Z; i++)
         destQuat[i] = startScale*startQ[i] + endScale*destQuat[i];
   }

}   /* q_slerp */



/*****************************************************************************
 * 
    q_make:  make a quaternion given an axis and an angle (in radians)
    
      notes:
       - rotation is counter-clockwise when rotation axis vector is 
         pointing at you
          - if angle or vector are 0, the identity quaternion is returned.
       
    q_type destQuat :    quat to be made  
    double x, y, z  :    axis of rotation 
    double angle    :    angle of rotation about axis in radians 
 *
 *****************************************************************************/
void q_make( q_type destQuat,double x, double y, double z, double angle)
{
   double length, cosA, sinA;

   /* normalize vector */
   length = sqrt( x*x + y*y + z*z );

   /* if zero vector passed in, just return identity quaternion   */
   if ( length < Q_EPSILON ) {
      destQuat[X] = 0;
      destQuat[Y] = 0;
      destQuat[Z] = 0;
      destQuat[W] = 1;
      return;
   }

   x /= length;
   y /= length;
   z /= length;

   cosA = cos(angle / 2.0);
   sinA = sin(angle / 2.0);

   destQuat[W] = cosA;
   destQuat[X] = sinA * x;
   destQuat[Y] = sinA * y;
   destQuat[Z] = sinA * z;

}   /* q_make */

/*****************************************************************************
 * 
    q_from_axis_angle:  Another name for q_make
    
 *
 *****************************************************************************/
void q_from_axis_angle(q_type destQuat, double x,  double y,  double z, double angle)
{
    q_make(destQuat, x, y, z, angle);
}

/*****************************************************************************
 * 
    q_to_axis_angle:  make an axis and an angle (in radians) given a quaternion
    
      notes:
       - rotation is counter-clockwise when rotation axis vector is 
         pointing at you
	    - If identity quaternion is passed in, return zero rotation around
	      the Z axis.
       
    double x, y, z  :    axis of rotation 
    double angle    :    angle of rotation about axis in radians 
    q_type srcQuat :     quat that is to be described
 *
 *****************************************************************************/

void q_to_axis_angle (double *x, double *y, double *z, double *angle,
		      const q_type srcQuat)
{
    /* If we have the identity quaternion, return zero rotation about Z */
    double length = sqrt( srcQuat[X]*srcQuat[X] + srcQuat[Y]*srcQuat[Y] + srcQuat[Z]*srcQuat[Z]);
    if (length < Q_EPSILON) {
	*x = *y = *angle = 0;
	*z = 1;
	return;
    }

    /* According to an article by Sobiet Void (robin@cybervision.com) on Game Developer's
     * Network, the following conversion is appropriate. */

    *x = srcQuat[X] / length;
    *y = srcQuat[Y] / length;
    *z = srcQuat[Z] / length;
    *angle = 2 * acos(srcQuat[W]);

}   /* q_to_axis_angle */



/*****************************************************************************
 *
   q_from_two_vecs - create a quaternion from two vectors that rotates
                  v1 to v2 about an axis perpendicular to both
 *
 *****************************************************************************/
void q_from_two_vecs(q_type destQuat, const q_vec_type v1, const q_vec_type v2 )
{
   q_vec_type u1, u2 ;
   q_vec_type axis ;              /* axis of rotation */
   double theta ;               /* angle of rotation about axis */
   double theta_complement ;
   double crossProductMagnitude ;

   /*
   ** Normalize both vectors and take cross product to get rotation axis. 
   */
   q_vec_normalize( u1, v1 ) ;
   q_vec_normalize( u2, v2 ) ;
   q_vec_cross_product( axis, u1, u2 ) ;

   /*
   ** | u1 X u2 | = |u1||u2|sin(theta)
   **
   ** Since u1 and u2 are normalized, 
   **
   **  theta = arcsin(|axis|)
   */
   crossProductMagnitude = sqrt( q_vec_dot_product( axis, axis ) ) ;

   /*
   ** Occasionally, even though the vectors are normalized, the magnitude will
   ** be calculated to be slightly greater than one.  If this happens, just
   ** set it to 1 or asin() will barf.
   */
   if( crossProductMagnitude > 1.0 )
      crossProductMagnitude = 1.0 ;

   /*
   ** Take arcsin of magnitude of rotation axis to compute rotation angle.
   ** Since crossProductMagnitude=[0,1], we will have theta=[0,pi/2].
   */
   theta = asin( crossProductMagnitude ) ;
   theta_complement = Q_PI - theta ;

   /*
   ** If cos(theta) < 0, use complement of theta as rotation angle.
   */
   if( q_vec_dot_product(u1, u2) < 0.0 )
   {
      theta = theta_complement ;
      theta_complement = Q_PI - theta ;
   }

   /* if angle is 0, just return identity quaternion   */
   if( theta < Q_EPSILON )
   {
      destQuat[Q_X] = 0.0 ;
      destQuat[Q_Y] = 0.0 ;
      destQuat[Q_Z] = 0.0 ;
      destQuat[Q_W] = 1.0 ;
   }
   else
   {
      if( theta_complement < Q_EPSILON )
      {
         /*
         ** The two vectors are opposed.  Find some arbitrary axis vector.
         ** First try cross product with x-axis if u1 not parallel to x-axis.
         */
         if( (u1[Y]*u1[Y] + u1[Z]*u1[Z]) >= Q_EPSILON )
         {
            axis[X] = 0.0 ;
            axis[Y] = u1[Z] ;
            axis[Z] = -u1[Y] ;
         }
         else
         {
            /*
            ** u1 is parallel to to x-axis.  Use z-axis as axis of rotation.
            */
            axis[X] = axis[Y] = 0.0 ;
            axis[Z] = 1.0 ;
         }
      }

      q_vec_normalize( axis, axis ) ;
      q_make( destQuat, axis[Q_X], axis[Q_Y], axis[Q_Z], theta ) ;
      q_normalize( destQuat, destQuat ) ;
   }
}  /* q_from_two_vecs */

/* converts quat to euler angles (yaw, pitch, roll).  see
 * q_col_matrix_to_euler() for conventions.  Note that you
 * cannot use Q_X, Q_Y, and Q_Z to pull the elements out of
 * the Euler as if they were rotations about these angles --
 * this will invert X and Z.  You need to instead use Q_YAW
 * (rotation about Z), Q_PITCH (rotation about Y) and Q_ROLL
 * (rotation about X) to get them.
 */
void q_to_euler(q_vec_type yawPitchRoll, const q_type q)
{
   q_matrix_type matrix;

   q_to_col_matrix(matrix, q);
   q_col_matrix_to_euler(yawPitchRoll, ( const double (*)[4] ) matrix);
}


/*****************************************************************************
 *  
    q_from_euler - converts 3 euler angles (in radians) to a quaternion
     
   angles are in radians;  Assumes roll is rotation about X, pitch
   is rotation about Y, yaw is about Z.  Assumes order of 
   yaw, pitch, roll applied as follows:
       
       p' = roll( pitch( yaw(p) ) )

      See comments for q_euler_to_col_matrix for more on this.
 *
 *****************************************************************************/
void q_from_euler(q_type destQuat, double yaw, double pitch, double roll)
{
   double  cosYaw, sinYaw, cosPitch, sinPitch, cosRoll, sinRoll;
   double  half_roll, half_pitch, half_yaw;


   /* put angles into radians and divide by two, since all angles in formula
    *  are (angle/2)
    */

   half_yaw = yaw / 2.0;
   half_pitch = pitch / 2.0;
   half_roll = roll / 2.0;

   cosYaw = cos(half_yaw);
   sinYaw = sin(half_yaw);

   cosPitch = cos(half_pitch);
   sinPitch = sin(half_pitch);

   cosRoll = cos(half_roll);
   sinRoll = sin(half_roll);


   destQuat[X] = sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw;
   destQuat[Y] = cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw;
   destQuat[Z] = cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw;

   destQuat[W] = cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw;

}  /* q_from_euler */





/*****************************************************************************
 * 
    q_to_row_matrix: Convert quaternion to 4x4 row-major rotation matrix.
             Quaternion need not be unit magnitude.
 *
 *****************************************************************************/
void q_to_row_matrix(q_matrix_type destMatrix, const q_type srcQuat)
{
   double  s;
   double  xs, ys, zs,
      wx, wy, wz,
      xx, xy, xz,
      yy, yz, zz;


   /* 
    * For unit srcQuat, just set s = 2.0; or set xs = srcQuat[X] + 
    *   srcQuat[X], etc. 
    */

   s = 2.0 / (srcQuat[X]*srcQuat[X] + srcQuat[Y]*srcQuat[Y] + 
              srcQuat[Z]*srcQuat[Z] + srcQuat[W]*srcQuat[W]);

   xs = srcQuat[X] * s;   ys = srcQuat[Y] * s;   zs = srcQuat[Z] * s;
   wx = srcQuat[W] * xs;  wy = srcQuat[W] * ys;  wz = srcQuat[W] * zs;
   xx = srcQuat[X] * xs;  xy = srcQuat[X] * ys;  xz = srcQuat[X] * zs;
   yy = srcQuat[Y] * ys;  yz = srcQuat[Y] * zs;  zz = srcQuat[Z] * zs;

   /* set up 4x4 destMatrixrix */
   destMatrix[X][X] = 1.0 - (yy + zz);
   destMatrix[X][Y] = xy + wz;
   destMatrix[X][Z] = xz - wy;
   destMatrix[X][W] = 0.0;

   destMatrix[Y][X] = xy - wz;
   destMatrix[Y][Y] = 1.0 - (xx + zz);
   destMatrix[Y][Z] = yz + wx;
   destMatrix[Y][W] = 0.0;

   destMatrix[Z][X] = xz + wy;
   destMatrix[Z][Y] = yz - wx;
   destMatrix[Z][Z] = 1.0 - (xx + yy);
   destMatrix[Z][W] = 0.0;

   destMatrix[W][X] = 0.0;
   destMatrix[W][Y] = 0.0;
   destMatrix[W][Z] = 0.0;
   destMatrix[W][W] = 1.0;

}   /* q_to_row_matrix */



/*****************************************************************************
 * q_to_col_matrix: Convert quaternion to 4x4 column-major rotation matrix.
 *              Quaternion need not be unit magnitude.
 *****************************************************************************/
void q_to_col_matrix(q_matrix_type destMatrix, const q_type srcQuat)
{
   double s,xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;


   /* For unit srcQuat, just set s = 2.0; or set xs = srcQuat[X] + 
    *  srcQuat[X], etc. 
    *****************************************************************************/
   s = 2.0 / (srcQuat[X]*srcQuat[X] + srcQuat[Y]*srcQuat[Y] + 
              srcQuat[Z]*srcQuat[Z] + srcQuat[W]*srcQuat[W]);

   xs = srcQuat[X] * s;   ys = srcQuat[Y] * s;   zs = srcQuat[Z] * s;
   wx = srcQuat[W] * xs;  wy = srcQuat[W] * ys;  wz = srcQuat[W] * zs;
   xx = srcQuat[X] * xs;  xy = srcQuat[X] * ys;  xz = srcQuat[X] * zs;
   yy = srcQuat[Y] * ys;  yz = srcQuat[Y] * zs;  zz = srcQuat[Z] * zs;

   /* set up 4x4 destMatrixrix */
   destMatrix[X][X] = 1.0 - (yy + zz);
   destMatrix[X][Y] = xy - wz;
   destMatrix[X][Z] = xz + wy;
   destMatrix[X][W] = 0.0;

   destMatrix[Y][X] = xy + wz;
   destMatrix[Y][Y] = 1.0 - (xx + zz);
   destMatrix[Y][Z] = yz - wx;
   destMatrix[Y][W] = 0.0;

   destMatrix[Z][X] = xz - wy;
   destMatrix[Z][Y] = yz + wx;
   destMatrix[Z][Z] = 1.0 - (xx + yy);
   destMatrix[Z][W] = 0.0;

   destMatrix[W][X] = 0.0;
   destMatrix[W][Y] = 0.0;
   destMatrix[W][Z] = 0.0;
   destMatrix[W][W] = 1.0;

}   /* q_to_col_matrix */



/*****************************************************************************
 * q_from_col_matrix- Convert 4x4 column-major rotation matrix 
 *                to unit quaternion.
 *****************************************************************************/
void q_from_col_matrix(q_type destQuat, const q_matrix_type srcMatrix)
{
   double   trace, s;
   int      i, j, k;
   static int  next[3] = {Y, Z, X};


   trace = srcMatrix[X][X] + srcMatrix[Y][Y] + srcMatrix[Z][Z];

   if (trace > 0.0) 
   {
      s = sqrt(trace + 1.0);
    
      destQuat[W] = s * 0.5;
    
      s = 0.5 / s;
    
      destQuat[X] = (srcMatrix[Z][Y] - srcMatrix[Y][Z]) * s;
      destQuat[Y] = (srcMatrix[X][Z] - srcMatrix[Z][X]) * s;
      destQuat[Z] = (srcMatrix[Y][X] - srcMatrix[X][Y]) * s;
   } 

   else 
   {
      i = X;
      if (srcMatrix[Y][Y] > srcMatrix[X][X])
         i = Y;
      if (srcMatrix[Z][Z] > srcMatrix[i][i])
         i = Z;
      j = next[i];  
      k = next[j];
    
      s = sqrt( (srcMatrix[i][i] - (srcMatrix[j][j]+srcMatrix[k][k])) + 1.0 );
      destQuat[i] = s * 0.5;
      s = 0.5 / s;
    
      destQuat[W] = (srcMatrix[k][j] - srcMatrix[j][k]) * s;
      destQuat[j] = (srcMatrix[j][i] + srcMatrix[i][j]) * s;
      destQuat[k] = (srcMatrix[k][i] + srcMatrix[i][k]) * s;
   }

}   /* q_from_col_matrix */



/*****************************************************************************
 * 
   q_from_row_matrix: Convert 4x4 row-major rotation matrix to unit quaternion
 *
 *****************************************************************************/
void q_from_row_matrix(q_type destQuat, const q_matrix_type srcMatrix)
{
   double   trace, s;
   int      i, j, k;
   static int  next[3] = {Y, Z, X};


   trace = srcMatrix[X][X] + srcMatrix[Y][Y]+ srcMatrix[Z][Z];

   if (trace > 0.0)
   {
      s = sqrt(trace + 1.0);
      destQuat[W] = s * 0.5;
      s = 0.5 / s;
    
      destQuat[X] = (srcMatrix[Y][Z] - srcMatrix[Z][Y]) * s;
      destQuat[Y] = (srcMatrix[Z][X] - srcMatrix[X][Z]) * s;
      destQuat[Z] = (srcMatrix[X][Y] - srcMatrix[Y][X]) * s;
   } 

   else 
   {
      i = X;
      if (srcMatrix[Y][Y] > srcMatrix[X][X])
         i = Y;
      if (srcMatrix[Z][Z] > srcMatrix[i][i])
         i = Z;
      j = next[i];  
      k = next[j];
    
      s = sqrt( (srcMatrix[i][i] - (srcMatrix[j][j]+srcMatrix[k][k])) + 1.0 );
      destQuat[i] = s * 0.5;
    
      s = 0.5 / s;
    
      destQuat[W] = (srcMatrix[j][k] - srcMatrix[k][j]) * s;
      destQuat[j] = (srcMatrix[i][j] + srcMatrix[j][i]) * s;
      destQuat[k] = (srcMatrix[i][k] + srcMatrix[k][i]) * s;
   }

}   /* q_from_row_matrix */



/*****************************************************************************
 *
   qgl_to_matrix - convert from quat to GL 4x4 float row matrix
    
    notes:
      - same as q_to_row_matrix, except base type is float, not double
 *
 *****************************************************************************/
void qgl_to_matrix(qgl_matrix_type destMatrix, const q_type srcQuat)
{
   double  s;
   double  xs, ys, zs,
      wx, wy, wz,
      xx, xy, xz,
      yy, yz, zz;


   /* 
    * For unit srcQuat, just set s = 2.0; or set xs = srcQuat[X] + 
    *   srcQuat[X], etc. 
    */

   s = 2.0 / (srcQuat[X]*srcQuat[X] + srcQuat[Y]*srcQuat[Y] + 
              srcQuat[Z]*srcQuat[Z] + srcQuat[W]*srcQuat[W]);

   xs = srcQuat[X] * s;   ys = srcQuat[Y] * s;   zs = srcQuat[Z] * s;
   wx = srcQuat[W] * xs;  wy = srcQuat[W] * ys;  wz = srcQuat[W] * zs;
   xx = srcQuat[X] * xs;  xy = srcQuat[X] * ys;  xz = srcQuat[X] * zs;
   yy = srcQuat[Y] * ys;  yz = srcQuat[Y] * zs;  zz = srcQuat[Z] * zs;

   /* set up 4x4 destMatrixrix */
   destMatrix[X][X] = (float) (1.0 - (yy + zz));
   destMatrix[X][Y] = (float) (xy + wz);
   destMatrix[X][Z] = (float) (xz - wy);
   destMatrix[X][W] = 0.0;

   destMatrix[Y][X] = (float) (xy - wz);
   destMatrix[Y][Y] = (float) (1.0 - (xx + zz));
   destMatrix[Y][Z] = (float) (yz + wx);
   destMatrix[Y][W] = 0.0;

   destMatrix[Z][X] = (float) (xz + wy);
   destMatrix[Z][Y] = (float) (yz - wx);
   destMatrix[Z][Z] = (float) (1.0 - (xx + yy));
   destMatrix[Z][W] = 0.0;

   destMatrix[W][X] = 0.0;
   destMatrix[W][Y] = 0.0;
   destMatrix[W][Z] = 0.0;
   destMatrix[W][W] = 1.0;

}  /* qgl_to_matrix */


/*****************************************************************************
 * 
   qgl_from_matrix: Convert GL 4x4 row-major 
                      rotation matrix to unit quaternion.
 
    notes:
      - same as q_from_row_matrix, except basic type is float, not double
 *
 *****************************************************************************/
void qgl_from_matrix(q_type destQuat, const qgl_matrix_type srcMatrix)
{
   double   trace, s;
   int      i, j, k;
   static int  next[3] = {Y, Z, X};


   trace = srcMatrix[X][X] + srcMatrix[Y][Y]+ srcMatrix[Z][Z];

   if (trace > 0.0)
   {
      s = sqrt(trace + 1.0);
      destQuat[W] = s * 0.5;
      s = 0.5 / s;
    
      destQuat[X] = (srcMatrix[Y][Z] - srcMatrix[Z][Y]) * s;
      destQuat[Y] = (srcMatrix[Z][X] - srcMatrix[X][Z]) * s;
      destQuat[Z] = (srcMatrix[X][Y] - srcMatrix[Y][X]) * s;
   } 

   else 
   {
      i = X;
      if (srcMatrix[Y][Y] > srcMatrix[X][X])
         i = Y;
      if (srcMatrix[Z][Z] > srcMatrix[i][i])
         i = Z;
      j = next[i];  
      k = next[j];
    
      s = sqrt( (srcMatrix[i][i] - (srcMatrix[j][j]+srcMatrix[k][k])) + 1.0 );
      destQuat[i] = s * 0.5;
    
      s = 0.5 / s;
    
      destQuat[W] = (srcMatrix[j][k] - srcMatrix[k][j]) * s;
      destQuat[j] = (srcMatrix[i][j] + srcMatrix[j][i]) * s;
      destQuat[k] = (srcMatrix[i][k] + srcMatrix[k][i]) * s;
   }

}   /* qgl_from_matrix */

/*****************************************************************************
 * 
    q_from_ogl_matrix: Convert OpenGL matrix to quaternion

    notes:
        - same as q_from_row_matrix, except matrix is single index
 *
 *****************************************************************************/
void q_from_ogl_matrix(q_type destQuat, const qogl_matrix_type matrix )
{
   double     trace, s;
   int        i, j, k;
   static int next[3] = { Y, Z, X };

   trace = matrix[X*4+X] + matrix[Y*4+Y] + matrix[Z*4+Z];
   if( trace > 0.0 ) {
      s = sqrt( trace + 1.0 );
      destQuat[ W ] = s * 0.5;
      s = 0.5 / s;

      destQuat[ X ] = ( matrix[Y*4+Z] - matrix[Z*4+Y] ) * s;
      destQuat[ Y ] = ( matrix[Z*4+X] - matrix[X*4+Z] ) * s;
      destQuat[ Z ] = ( matrix[X*4+Y] - matrix[Y*4+X] ) * s;
   } else {
      i = X;
      if( matrix[Y*4+Y] > matrix[X*4+X] )
         i = Y;
      if( matrix[Z*4+Z] > matrix[i*4+i] )
         i = Z;
      j = next[i];
      k = next[j];

      s = sqrt(( matrix[i*4+i] - ( matrix[j*4+j] + matrix[k*4+k] )) + 1.0 );
      destQuat[i] = s * 0.5;
      s = 0.5 / s;

      destQuat[W] = ( matrix[j*4+k] - matrix[k*4+j] ) * s;
      destQuat[j] = ( matrix[i*4+j] + matrix[j*4+i] ) * s;
      destQuat[k] = ( matrix[i*4+k] + matrix[k*4+i] ) * s;
   }

} /* q_from_ogl_matrix */

/*****************************************************************************
 * 
    q_to_ogl_matrix: Convert quaternion to OpenGL matrix.
             Quaternion need not be unit magnitude.
 *
 *****************************************************************************/
void q_to_ogl_matrix(qogl_matrix_type matrix, const q_type srcQuat )
{
   double s;
   double  xs, ys, zs, wx, wy, wz, xx, xy, xz, yy, yz, zz;

   /* For unit srcQuat, just set s = 2.0, or set xs = srcQuat[X] +
   ** srcQuat[X], etc.
   */
   s = 2.0 / ( srcQuat[X] * srcQuat[X] + srcQuat[Y] * srcQuat[Y] + 
               srcQuat[Z] * srcQuat[Z] + srcQuat[W] * srcQuat[W] );

   xs = srcQuat[X] * s;   ys = srcQuat[Y] * s;   zs = srcQuat[Z] * s;
   wx = srcQuat[W] * xs;  wy = srcQuat[W] * ys;  wz = srcQuat[W] * zs;
   xx = srcQuat[X] * xs;  xy = srcQuat[X] * ys;  xz = srcQuat[X] * zs;
   yy = srcQuat[Y] * ys;  yz = srcQuat[Y] * zs;  zz = srcQuat[Z] * zs;

   /* set up 4x4 matrix */
   matrix[X*4+X] = 1.0 - ( yy + zz );
   matrix[X*4+Y] = xy + wz;
   matrix[X*4+Z] = xz - wy;
   matrix[X*4+W] = 0.0;

   matrix[Y*4+X] = xy - wz;
   matrix[Y*4+Y] = 1.0 - ( xx + zz );
   matrix[Y*4+Z] = yz + wx;
   matrix[Y*4+W] = 0.0;

   matrix[Z*4+X] = xz + wy;
   matrix[Z*4+Y] = yz - wx;
   matrix[Z*4+Z] = 1.0 - ( xx + yy );
   matrix[Z*4+W] = 0.0;

   matrix[W*4+X] = 0.0;
   matrix[W*4+Y] = 0.0;
   matrix[W*4+Z] = 0.0;
   matrix[W*4+W] = 1.0;

} /* q_to_ogl_matrix */
