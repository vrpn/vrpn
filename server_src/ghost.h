#ifndef	vrpn_GHOST_H
#define	vrpn_GHOST_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

#include <math.h>
#include <fstream.h>
#include <stdio.h>
#include <quat.h>
#include <vector>

//----------------------------------------------------------------------------
// This section contains helper classes that might make life easier.  They were
// put here when porting the Phantom code from GHOST to HDAPI.  It either
// implements the functions that were needed form GHOST or else defines the
// VRPN types to use the relevant GHOST types.

// XXX WARNING!  These classes are not a sound foundation on which to build
// an application.  They were fiddled with until they implemented sufficient
// functions to do what was needed for the port.  The space, row/column, and
// other features of the matrices in particular are not tested beyond their
// ability to do what was needed.

#ifdef	VRPN_USE_HDAPI
  #include <HD/hd.h>
  #include <HDU/hduError.h>

  #ifndef M_PI
  #define M_PI (3.14159265358979323846)
  #endif

  typedef bool	vrpn_HapticBoolean;

  class vrpn_HapticVector {
  public:
    vrpn_HapticVector() { d[0] = d[1] = d[2] = 0; }
    vrpn_HapticVector(double x, double y, double z)
      { d[0] = x; d[1] = y; d[2] = z; }
    vrpn_HapticVector(const double *vec) { d[0] = vec[0]; d[1] = vec[1]; d[2] = vec[2]; }

    void set(double val[3]) { d[0] = val[0]; d[1] = val[1]; d[2] = val[2]; }
    void set(double x, double y, double z) { d[0] = x; d[1] = y; d[2] = z; }
    void setx(double x) { d[0] = x; }
    void sety(double y) { d[1] = y; }
    void setz(double z) { d[2] = z; }

    double distToOrigin(void) const { return sqrt( d[0]*d[0] + d[1]*d[1] + d[2]*d[2] ); }
    double norm(void) const { return distToOrigin(); }
    void normalize(void) { double len = distToOrigin(); d[0] /= len; d[1] /= len; d[2] /= len; }

    double dot(const vrpn_HapticVector &dot_me) const {
      return d[0]*dot_me.d[0] + d[1]*dot_me.d[1] + d[2]*dot_me.d[2];
    }
    vrpn_HapticVector cross(const vrpn_HapticVector &cross_me) const {
      vrpn_HapticVector c( d[1]*cross_me[2] - d[2]*cross_me[1],
				  d[2]*cross_me[0] - d[0]*cross_me[2],
				  d[0]*cross_me[1] - d[1]*cross_me[0]);
      return c;
    }

    vrpn_HapticVector operator+= (const vrpn_HapticVector &add_me)
      { d[0] += add_me.d[0]; d[1] += add_me.d[1]; d[2] += add_me.d[2]; return *this; }
    vrpn_HapticVector operator-= (const vrpn_HapticVector &sub_me)
      { d[0] -= sub_me.d[0]; d[1] -= sub_me.d[1]; d[2] -= sub_me.d[2]; return *this; }

    vrpn_HapticVector operator*= (double mul_me)
      { d[0] *= mul_me; d[1] *= mul_me; d[2] *= mul_me; return *this; }
    vrpn_HapticVector operator/= (double div_me)
      { d[0] /= div_me; d[1] /= div_me; d[2] /= div_me; return *this; }

    double &operator[] (unsigned index) { return d[index]; }
    double operator[] (unsigned index) const { return d[index]; }

  protected:
    double d[3];
  };

  inline vrpn_HapticVector operator+ ( const vrpn_HapticVector &a, const vrpn_HapticVector &b)
    { vrpn_HapticVector ret = a; return ret += b; }
  inline vrpn_HapticVector operator- ( const vrpn_HapticVector &a, const vrpn_HapticVector &b)
    { vrpn_HapticVector ret = a; return ret -= b; }
  inline vrpn_HapticVector operator* ( const vrpn_HapticVector &a, double b)
    { vrpn_HapticVector ret = a; return ret *= b; }
  inline vrpn_HapticVector operator* ( double b, const vrpn_HapticVector &a)
    { vrpn_HapticVector ret = a; return ret *= b; }
  inline vrpn_HapticVector operator/ ( const vrpn_HapticVector &a, double b)
    { vrpn_HapticVector ret = a; return ret /= b; }

  class vrpn_HapticPosition : public vrpn_HapticVector {
  public:
    vrpn_HapticPosition() : vrpn_HapticVector() { }
    vrpn_HapticPosition(double x, double y, double z) : vrpn_HapticVector(x,y,z) {}

    vrpn_HapticPosition(const vrpn_HapticVector &v) {
      d[0] = v[0]; d[1] = v[1]; d[2] = v[2];
    }
  };

  class vrpn_HapticPlane {
  public:
    vrpn_HapticPlane() { d_A = d_B = d_D = 0; d_C = 1; }
    vrpn_HapticPlane(double a, double b, double c, double d)
      { d_A = a; d_B = b; d_C = c; d_D = d; };
    vrpn_HapticPlane(const vrpn_HapticPlane &p)
      { d_A = p.d_A; d_B = p.d_B; d_C = p.d_C; d_D = p.d_D; }
    vrpn_HapticPlane(const vrpn_HapticPlane *p)
      { d_A = p->d_A; d_B = p->d_B; d_C = p->d_C; d_D = p->d_D; }
    vrpn_HapticPlane(const vrpn_HapticVector &v, double d)
      { d_A = v[0]; d_B = v[1]; d_C = v[2]; d_D = d; }
    vrpn_HapticPlane(const vrpn_HapticVector &v, const vrpn_HapticPosition &p) {
      d_A = v[0]; d_B = v[1]; d_C = v[2];
      d_D = - ( d_A*p[0] + d_B*p[1] + d_C*p[2] );
    }

    double a() const { return d_A; }
    double b() const { return d_B; }
    double c() const { return d_C; }
    double d() const { return d_D; }
    vrpn_HapticVector normal(void) const {
      double len = sqrt( d_A*d_A + d_B*d_B + d_C*d_C );
      return vrpn_HapticVector(d_A/len, d_B/len, d_C/len);
    }
    double error(vrpn_HapticPosition p) const {
      return p[0]*d_A + p[1]*d_B + p[2]*d_C + d_D;
    }
    vrpn_HapticBoolean pointOfLineIntersection(const vrpn_HapticPosition P1, const vrpn_HapticPosition P2, vrpn_HapticPosition &intersection) const {
      if (error(P1) * error(P2) > 0) {
	return false;
      } else {
	double frac = ( d_A*P1[0] + d_B*P1[1] + d_C*P1[2] + d_D ) /
		      ( d_A * (P1[0]-P2[0]) + d_B * (P1[1] - P2[1]) + d_C * (P1[2] - P2[2]) );
	intersection = P1 + P2 * frac;
	return true;
      }
    }
    vrpn_HapticPosition	projectPoint(const vrpn_HapticPosition &p) const {
      // Find the signed distance from the plane along the un-normalized plane normal
      double dist = error(p);
      // Translate along the negative plane normal by this distance to find the point
      // of intersection
      vrpn_HapticVector	t(d_A, d_B, d_C);
      t *= -dist;
      return p + t;
    }

    void setPlane(const vrpn_HapticVector &v, const vrpn_HapticPosition &p) {
      d_A = v[0]; d_B = v[1]; d_C = v[2];
      d_D = - ( d_A*p[0] + d_B*p[1] + d_C*p[2] );
    }
  protected:
    double d_A, d_B, d_C, d_D;
  };

  // XXX Translation is stored in first three entries in last row from HDAPI.
  // XXX Rotation seems to follow row-major order (that's the call that is
  // needed from Quatlib to convert it correctly for VRPN).
  // XXX This class DOES NOT WORK.  I tried to get it working for the
  // DynamicPlane, but ended up giving up.  The problem seems to be within
  // the fromLocal() functions, but I can't guarantee it.
  class vrpn_HapticMatrix {
  public:
    vrpn_HapticMatrix() {
      identity();
    }

    void identity(void) {
      d[0][0] = d[1][1] = d[2][2] = d[3][3] = 1;
      d[0][1] = d[0][2] = d[0][3] = 0;
      d[1][0] = d[1][2] = d[1][3] = 0;
      d[2][0] = d[2][1] = d[2][3] = 0;
      d[3][0] = d[3][1] = d[3][2] = 0;
    }

    void getRotationMatrix(double rot[3][3]) const {
      rot[0][0] = d[0][0]; rot[0][1] = d[0][1]; rot[0][2] = d[0][2];
      rot[1][0] = d[1][0]; rot[1][1] = d[1][1]; rot[1][2] = d[1][2];
      rot[2][0] = d[2][0]; rot[2][1] = d[2][1]; rot[2][2] = d[2][2];
    }

    void setRotate(const vrpn_HapticVector &a, double radians) {
      q_type  quat;
      q_make(quat, a[0], a[1], a[2], radians);
      q_matrix_type mat;
      q_to_row_matrix(mat, quat);
      d[0][0] = mat[0][0]; d[0][1] = mat[0][1]; d[0][2] = mat[0][2];
      d[1][0] = mat[1][0]; d[1][1] = mat[1][1]; d[1][2] = mat[1][2];
      d[2][0] = mat[2][0]; d[2][1] = mat[2][1]; d[2][2] = mat[2][2];
    }
    void setRotationMatrix(const double rot[3][3]) {
      d[0][0] = rot[0][0]; d[0][1] = rot[0][1]; d[0][2] = rot[0][2];
      d[1][0] = rot[1][0]; d[1][1] = rot[1][1]; d[1][2] = rot[1][2];
      d[2][0] = rot[2][0]; d[2][1] = rot[2][1]; d[2][2] = rot[2][2];
    }
    void setTranslate(const vrpn_HapticVector &t) {
      d[3][0] = t[0]; d[3][1] = t[1]; d[3][2] = t[2];
    }

    // Vectors are transformed by 3x3 rotation part of the matrix,
    // in fact by its inverse (its transpose).
    // XXX These may not work!
    vrpn_HapticVector fromLocal1(const vrpn_HapticVector &v) const {
      vrpn_HapticVector ret;
      ret[0] = v[0] * d[0][0] + v[1] * d[0][1] + v[2] * d[0][2];
      ret[1] = v[0] * d[1][0] + v[1] * d[1][1] + v[2] * d[1][2];
      ret[2] = v[0] * d[2][0] + v[1] * d[2][1] + v[2] * d[2][2];
      return ret;
    }

    // Vectors are transformed by 3x3 rotation part of the matrix,
    // in fact by its inverse (its transpose).
    // XXX These may not work!
    vrpn_HapticVector fromLocal2(const vrpn_HapticVector &v) const {
      vrpn_HapticVector ret;
      ret[0] = v[0] * d[0][0] + v[1] * d[1][0] + v[2] * d[2][0];
      ret[1] = v[0] * d[0][1] + v[1] * d[1][1] + v[2] * d[2][1];
      ret[2] = v[0] * d[0][2] + v[1] * d[1][2] + v[2] * d[2][2];
      return ret;
    }

    // Positions are transformed by 3x4 part of the matrix, including
    // the translation.
    // XXX These may not work!
    vrpn_HapticPosition	fromLocal1(const vrpn_HapticPosition &p) const {
      vrpn_HapticPosition ret;
      ret[0] = p[0] * d[0][0] + p[1] * d[1][0] + p[2] * d[2][0] + d[3][0];
      ret[1] = p[0] * d[0][1] + p[1] * d[1][1] + p[2] * d[2][1] + d[3][1];
      ret[2] = p[0] * d[0][2] + p[1] * d[1][2] + p[2] * d[2][2] + d[3][2];
      return ret;
    }

    // Positions are transformed by 3x4 part of the matrix, including
    // the translation.
    // XXX These may not work!
    vrpn_HapticPosition	fromLocal2(const vrpn_HapticPosition &p) const {
      vrpn_HapticPosition ret;
      ret[0] = p[0] * d[0][0] + p[1] * d[0][1] + p[2] * d[0][2] + d[3][0];
      ret[1] = p[0] * d[1][0] + p[1] * d[1][1] + p[2] * d[1][2] + d[3][1];
      ret[2] = p[0] * d[2][0] + p[1] * d[2][1] + p[2] * d[2][2] + d[3][2];
      return ret;
    }

    // Positions are transformed by 3x4 part of the matrix, including
    // the translation.
    // XXX These may not work!
    vrpn_HapticPosition	fromLocal3(const vrpn_HapticPosition &p) const {
      vrpn_HapticPosition inv(p[0] - d[3][0], p[1] - d[3][1], p[2] - d[3][2]);
      vrpn_HapticPosition ret;
      ret[0] = inv[0] * d[0][0] + inv[1] * d[1][0] + inv[2] * d[2][0];
      ret[1] = inv[0] * d[0][1] + inv[1] * d[1][1] + inv[2] * d[2][1];
      ret[2] = inv[0] * d[0][2] + inv[1] * d[1][2] + inv[2] * d[2][2];
      return ret;
    }

    // Positions are transformed by 3x4 part of the matrix, including
    // the translation.
    // XXX These may not work!
    vrpn_HapticPosition	fromLocal4(const vrpn_HapticPosition &p) const {
      vrpn_HapticPosition inv(p[0] - d[3][0], p[1] - d[3][1], p[2] - d[3][2]);
      vrpn_HapticPosition ret;
      ret[0] = inv[0] * d[0][0] + inv[1] * d[0][1] + inv[2] * d[0][2];
      ret[1] = inv[0] * d[1][0] + inv[1] * d[1][1] + inv[2] * d[1][2];
      ret[2] = inv[0] * d[2][0] + inv[1] * d[2][1] + inv[2] * d[2][2];
      return ret;
    }

    // XXX Horrible -- programming by simulated annealing -- trying out all of the
    // possible combinations to see which one works...
    // (1,1) --> Often zero forces all over the space (persistent) Forces in non-axis-aligned directions?
    // (1,2) --> Forces in non-axis-aligned directions
    // (1,3) --> Forces in non-axis-aligned directions
    // (1,4) --> Forces in non-axis-aligned directions
    // (2,1) --> Forces in correct direction, with reasonable dead zone; along some axes the force jumps in and out
    // (2,2) --> Forces in correct direction, but onset is immediate and some are sudden
    // (2,3) --> Forces in correct direction, seems to be good dead zone, +X, -Y, -Z have slow onset (others sudden)
    // (2,4) --> Forces in correct direction, more sudden onsets than with (2,3)
    vrpn_HapticVector fromLocal(const vrpn_HapticVector &v) { return fromLocal2(v); }
    vrpn_HapticPosition fromLocal(const vrpn_HapticPosition &p) { return fromLocal3(p); }

  protected:
    double  d[4][4];
  };

  class vrpn_HapticSurface {
  public:
    double  getSurfaceFstatic(void) const { return d_surfaceFstatic; }
    void    setSurfaceFstatic(double f) { d_surfaceFstatic = f; }

    double  getSurfaceFdynamic(void) const { return d_surfaceFdynamic; }
    void    setSurfaceFdynamic(double f) { d_surfaceFdynamic = f; }

    double  getSurfaceKspring(void) const { return d_surfaceKspring; }
    void    setSurfaceKspring(double f) { d_surfaceKspring = f; }

    double  getSurfaceKdamping(void) const { return d_surfaceKdamping; }
    void    setSurfaceKdamping(double f) { d_surfaceKdamping = f; }

  protected:
    double  d_surfaceFstatic;
    double  d_surfaceFdynamic;
    double  d_surfaceKspring;
    double  d_surfaceKdamping;
  };

  class	vrpn_HapticCollision {
  public:
    vrpn_HapticCollision(const vrpn_HapticPosition &point, const vrpn_HapticVector &dir) {
      d_location = point;
      d_normal = dir;
      d_normal.normalize();
    }

    vrpn_HapticPosition location(void) const { return d_location; }
    vrpn_HapticVector   normal(void) const { return d_normal; }

  protected:
    vrpn_HapticPosition	d_location;
    vrpn_HapticVector	d_normal;
  };

  typedef std::vector<vrpn_HapticCollision>	vrpn_HapticCollisionList;

  class vrpn_HapticCollisionState {
  public:
    // XXX Temporary for half-baked collision code
    vrpn_HapticCollisionState(const vrpn_HapticPosition curr_pos) {
      d_inContact = false;
      d_currentPosition = curr_pos;
    }

    vrpn_HapticBoolean	inContact(void) const { return d_inContact; }

    //XXX These methods will be used during the object collision
    // query routine to indicate collision states and such.
    void updateState(vrpn_HapticBoolean inContact) {
      d_inContact = inContact;
    }
    void addCollision(const vrpn_HapticPosition &point, const vrpn_HapticVector &dir) {
      if (d_collisions.size() != 0) {
	fprintf(stderr, "vrpn_HapticCollisionState::addCollision(): Only one allowed\n");
      } else {
	d_collisions.push_back(vrpn_HapticCollision(point, dir));
      }
    }

    const vrpn_HapticCollisionList	&collisions(void) const { return d_collisions; }
    void  clear(void) { d_collisions.clear(); }

    // XXX Temporary for half-based collision code
    vrpn_HapticPosition	      d_currentPosition;

  protected:
    vrpn_HapticBoolean	      d_inContact;
    vrpn_HapticCollisionList  d_collisions;
  };

  // Structure to hold the Phantom device state, passed back from readDeviceState().
  typedef struct {
    double  pose[4][4];
    double  last_pose[4][4];
    int	  instant_rate;
    int	  buttons;
  } HDAPI_state;

#else
  #include <gstBasic.h>
  #include <gstBoundaryCube.h>
  #include <gstButton.h>
  #include <gstDynamic.h>
  #include <gstNode.h>
  #include <gstPHANToM.h>
  #include <gstPHANToM_SCP.h>
  #include <gstTriPolyMeshHaptic.h>
  #include <gstPoint.h>
  #include <gstScene.h>
  #include <gstSeparator.h>
  #include <gstSlider.h>
  #include <gstTransformMatrix.h>

  #include <gstCube.h>
  #include <gstSphere.h>

  #include <gstForceField.h>

  typedef gstBoolean	      vrpn_HapticBoolean;
  typedef gstVector	      vrpn_HapticVector;
  typedef gstPoint	      vrpn_HapticPosition;
  typedef gstPlane	      vrpn_HapticPlane;
  typedef gstTransformMatrix  vrpn_HapticMatrix;
  typedef gstShape	      vrpn_HapticSurface;
#endif

#endif

#endif
