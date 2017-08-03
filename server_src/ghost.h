#ifndef	vrpn_GHOST_H
#define	vrpn_GHOST_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

#include <math.h>
#include <stdio.h>
#include <quat.h>
#include <vector>

//----------------------------------------------------------------------------
// This section contains helper classes that might make life easier.  They were
// put here when porting the Phantom code from GHOST to HDAPI.  It either
// implements the functions that were needed form GHOST or else defines the
// VRPN types to use the relevant GHOST types.  They do not completely solve
// the GHOST vs. HDAPI special-purpose code because not all of the methods
// implemented in GHOST objects are implemented in HDAPI objects (the names
// changed and some features were dropped).

// XXX WARNING!  These classes are not a sound foundation on which to build
// an application.  They were fiddled with until they implemented sufficient
// functions to do what was needed for the port.  The space, row/column, and
// other features of the matrices in particular are not tested beyond their
// ability to do what was needed.

#ifdef	VRPN_USE_HDAPI
  #if defined(_WIN32) && !defined(WIN32)
  // Through version 3.1, the test macro in Sensable's code is wrong.
  #define WIN32
  #endif

  #include <HL/hl.h>
  #include <HDU/hduVector.h>
  #include <HDU/hduPlane.h>
  #include <HDU/hduMatrix.h>
  #include <HDU/hduLineSegment.h>
  typedef HDboolean	vrpn_HapticBoolean;
  typedef hduVector3Dd  vrpn_HapticVector;
  typedef hduVector3Dd  vrpn_HapticPosition;
  typedef hduPlaned     vrpn_HapticPlane;
  typedef hduMatrix     vrpn_HapticMatrix;


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
      if (!d_collisions.empty()) {
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
    double  max_stiffness;
	double	current_force[3];
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
