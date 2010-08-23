#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#include "plane.h"

//#define VERBOSE
  
#ifdef	VRPN_USE_HDAPI
  #include <HD/hd.h>
  #include <HDU/hduError.h>
#endif

#ifndef	TRUE
#define TRUE 1
#endif
#ifndef	FALSE
#define FALSE 0
#endif

#ifndef	VRPN_USE_HDAPI
gstType Plane::PlaneClassTypeId;
static int temp = Plane::initClass();
#endif

//constructors
Plane::Plane(const vrpn_HapticPlane & p)
{
  plane= vrpn_HapticPlane(p);
  inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
#ifdef	VRPN_USE_HDAPI
#else
  invalidateCumTransf();
#endif
}

Plane::Plane(const vrpn_HapticPlane * p)
{
  plane = vrpn_HapticPlane(*p);
  inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
#ifdef	VRPN_USE_HDAPI
#else
  invalidateCumTransf();
#endif
}

Plane::Plane(const Plane *p)
{
  inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
#ifdef	VRPN_USE_HDAPI
#else
  invalidateCumTransf();
#endif
  plane = vrpn_HapticPlane(p->plane);
}

Plane::Plane(const Plane &p)
{
  plane = vrpn_HapticPlane(p.plane);
  inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
#ifdef	VRPN_USE_HDAPI
#else
  invalidateCumTransf();
#endif
}


Plane::Plane(double a,double b, double c, double d) 
{
  vrpn_HapticVector vec = vrpn_HapticVector(a,b,c);
  plane = vrpn_HapticPlane(vec,d);
#ifdef	VRPN_USE_HDAPI
#else
  invalidateCumTransf();
#endif
  inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
}

void Plane::setPlane(vrpn_HapticVector newNormal, vrpn_HapticPosition point) 
{ plane.setPlane(newNormal,point);
}


void Plane::update(double a, double b, double c, double d)
{
  plane = vrpn_HapticPlane(a,b,c,d);
  isNewPlane = TRUE;
}

#ifdef  VRPN_USE_HDAPI
void Plane::renderHL(void)
{
  // Do nothing if we're not turned on.
  if (!inEffect) { return; }

  // setup materials
  hlMaterialf(HL_FRONT_AND_BACK, HL_STIFFNESS, static_cast<float>(getSurfaceKspring()));
  hlMaterialf(HL_FRONT_AND_BACK, HL_DAMPING, static_cast<float>(getSurfaceKdamping()));
  hlMaterialf(HL_FRONT_AND_BACK, HL_STATIC_FRICTION, static_cast<float>(getSurfaceFstatic()));
  hlMaterialf(HL_FRONT_AND_BACK, HL_DYNAMIC_FRICTION, static_cast<float>(getSurfaceFdynamic()));

  // Render the shape by specifying the callbacks for it
  hlBeginShape(HL_SHAPE_CALLBACK, myId);
    hlCallback(HL_SHAPE_INTERSECT_LS, 
        (HLcallbackProc) intersectSurface, static_cast<void*>(this));
    hlCallback(HL_SHAPE_CLOSEST_FEATURES, 
        (HLcallbackProc) closestSurfaceFeatures, static_cast<void*>(this));
  hlEndShape();
}
#endif

#ifdef VRPN_USE_HDAPI
// intersect the line segment from startPt to endPt with the plane.  Return the closest point of intersection 
// to the start point in intersectionPt.  Return the surface normal at intersectionPt in intersectionNormal.
// Return which face (HL_FRONT or HL_BACK) is being touched in face.
// Return true if there is an intersection.

bool HLCALLBACK Plane::intersectSurface(const HLdouble startPt_WC[3], const HLdouble endPt_WC[3],
    HLdouble intersectionPt_WC[3], HLdouble intersectionNormal_WC[3], HLenum *face,
    void *userdata)
{
  // The planes are half-spaces, so we intersect the front surface
  // whenever the endpoint is inside the space and we intersect the
  // back whenever the endpoint is outside the space.  Pretend that
  // the endpoint dropped perpendicularly onto the plane is the
  // intersection point in all cases.

  // Get our Plane pointer from userdata
  Plane *plane = static_cast<Plane *>(userdata);

  // Convert the list of doubles into a point, so that we can
  // use the HDU helper functions.
  vrpn_HapticPosition P = vrpn_HapticPosition(endPt_WC);
  vrpn_HapticPosition closest = plane->plane.projectPoint(P);
  vrpn_HapticVector normal(plane->plane.normal());
  normal.normalize();

  // See if we are inside our outside by evaluating the plane
  // equation.
  double height = plane->plane.perpDistance(P);
  if (height <= 0) {
    *face = HL_FRONT;
  } else {
    normal *= -1;
    *face = HL_BACK;
  }

  // Copy results into the lists of doubles
  int i;
  for (i = 0; i < 3; i++) {
    intersectionPt_WC[i] = closest[i];
    intersectionNormal_WC[i] = normal[i];
  }

  // We always hit either the front or the back!
  return true;

#if XXX_version_that_works_but_you_slip_through
  // Get our Plane pointer from userdata
  Plane *plane = static_cast<Plane *>(userdata);

  // Convert the list of doubles into points, then put these into a
  // line segment so that we can use the HDU helper functions.
  vrpn_HapticPosition P1 = vrpn_HapticPosition(startPt_WC);
  vrpn_HapticPosition P2 = vrpn_HapticPosition(endPt_WC);
  hduLineSegmentd     Seg(P1,P2);

  // See if we intersect either in the front or in the back.
  vrpn_HapticPlane::IntersectResult front_or_back;
  double t;
  int i;
  front_or_back = plane->plane.intersectSegmentFront( Seg, t, 1e-8);
  if (front_or_back == vrpn_HapticPlane::ResultFront) {
    vrpn_HapticPosition intersection = P1 + t * (P2 - P1);
    *face = HL_FRONT;
    for (i = 0; i < 3; i++) {
      intersectionPt_WC[i] = intersection[i];
      intersectionNormal_WC[i] = plane->plane.normal()[i];
    }
    return true;
  } else if (front_or_back == vrpn_HapticPlane::ResultBack) {
    // Hit the back -- invert the normal.
    vrpn_HapticPosition intersection = P1 + t * (P2 - P1);
    *face = HL_BACK;
    for (i = 0; i < 3; i++) {
      intersectionPt_WC[i] = intersection[i];
      intersectionNormal_WC[i] = -plane->plane.normal()[i];
    }
    return true;
  } else {
    // No intersection.
    return false;
  }
#endif
}

// find the closest surface feature(s) to queryPt

bool HLCALLBACK Plane::closestSurfaceFeatures(const HLdouble queryPt_WC[3],
    const HLdouble targetPt_WC[3],  /* Unused in example I copied from */
    HLgeom *geom_WC, HLdouble closestPt_WC[3],
    void *userdata)
{
  // Get our Plane pointer from userdata
  Plane *plane = static_cast<Plane *>(userdata);

  // Convert the list of doubles into a point, so that we can use the HDU helper functions.
  vrpn_HapticPosition P = vrpn_HapticPosition(queryPt_WC);

  // The closest point is the perpendicular projection of the point
  vrpn_HapticPosition closest = plane->plane.projectPoint(P);

  // Return the plane itself as the closest 2D local feature
  vrpn_HapticVector normal(plane->plane.a(), plane->plane.b(), plane->plane.c());
  normal.normalize();

  hlLocalFeature2dv(geom_WC, HL_LOCAL_FEATURE_PLANE, normal, closest);
  int i;
  for (i = 0; i < 3; i++) {
    closestPt_WC[i] = closest[i];
  }

  return true;
}

#endif

#ifndef VRPN_USE_HDAPI
vrpn_HapticBoolean Plane::intersection(const vrpn_HapticPosition &startPt_WC,
			       const vrpn_HapticPosition &endPt_WC,
			       vrpn_HapticPosition &intersectionPt_WC,
			       vrpn_HapticVector &intersectionPtNormal_WC,
			       void **)
{
    vrpn_HapticPosition intersectionPoint;
    if(!inEffect) { return FALSE; }

    //get the start and end position of the line segment against
    //which the collision will be checked
    vrpn_HapticPosition P1 = fromWorld(startPt_WC);
    vrpn_HapticPosition P2 = fromWorld(endPt_WC);

    // XXX This always returns false here for GHOST, but that doesn't seem right...
    // How could it possibly have been working?  Comments in the texture_plane code
    // seem to indicate that perhaps it doesn't work when they are any other objects
    // in the scene but does work in that case (because this code is never used).
    return FALSE;

    float t1 = (float) plane.error(P1);
    float t2 = (float) plane.error(P2);
    if(t1*t2 > 0 )  { // Segment from P2 to P1 does not intersect the plane
     return FALSE;
    }
    //else segment from p1 to p2 intersects the plane, find the intersection point
    plane.pointOfLineIntersection(P1,P2,intersectionPoint);

    //set the intersection point and normal
    intersectionPt_WC = toWorld(intersectionPoint);
    //set normal
    intersectionPtNormal_WC = toWorld(plane.normal());

    return TRUE;
}
#endif

#ifndef	VRPN_USE_HDAPI
vrpn_HapticBoolean Plane::collisionDetect(gstPHANToM *PHANToM)
{ 
	vrpn_HapticPosition lastSCP, phantomPos, intersectionPoint;
	vrpn_HapticBoolean  inContact;
	double depth, d_new, d_goal;

	//if the plane node is not in effect
	if(inEffect== FALSE)
		return FALSE;

	inContact = getStateForPHANToM(PHANToM);

#ifdef VRPN_USE_GHOST_31
	if(!_touchableByPHANToM || _resetPHANToMContacts) {
#else // Ghost 4.0 (and the default case)
	if(!isTouchableByPHANToM() || _resetPHANToMContacts) {
#endif
		_resetPHANToMContacts = FALSE;
		inContact = FALSE;
		(void) updateStateForPHANToM(PHANToM,inContact);
		//printf("in if incontact is false\n");

		return inContact;
	}

	PHANToM->getPosition_WC(phantomPos);
	phantomPos= fromWorld(phantomPos);
	
	// note: this depth is not necessarily in the same units
	// as the coordinates because the plane equation is the
	// same if we multiply it by a constant but this doesn't matter
	// for our purposes since we are only interested in making 
	// sure the perceived depth does not change too quickly and
	// this is done by interpolation
	depth = -plane.error(phantomPos);
	if (depth <= 0) {	// positive depth is below surface
	    inContact = FALSE;
	    (void) updateStateForPHANToM(PHANToM,inContact);
	    return inContact;
	}
	else {	// In contact

	    // START OF RECOVERY IMPLEMENTATION
	    if (isNewPlane) {	// set up stuff for recovery
		    isNewPlane = FALSE;
		    isInRecovery = TRUE;
		    recoveryPlaneCount = 0;
		    originalPlane = plane;
		    d_goal = plane.d();
		    // calculate a new value for d such that the depth below 
		    // the surface is the same as the last depth
		    d_new = depth + d_goal;
		    if (lastDepth > 0) {
			d_new -= lastDepth;
		    }
		    // set first value for plane
		    plane = vrpn_HapticPlane(originalPlane.a(),originalPlane.b(),
			originalPlane.c(), d_new);
		    if (numRecoveryCycles < 1) {
			numRecoveryCycles = 1;
		    }
		    dIncrement = (d_goal - d_new)/(float)numRecoveryCycles;
	    }

	    if (isInRecovery) {	// move the recovery along one more step
		recoveryPlaneCount++;
		if (recoveryPlaneCount == numRecoveryCycles) {
		    isInRecovery = FALSE;
		    plane = originalPlane;
		}
		else {
		    plane = vrpn_HapticPlane(plane.a(),plane.b(),plane.c(),
			plane.d() + dIncrement);
		}
	    }
	    lastDepth = -(phantomPos[0]*plane.a() + phantomPos[1]*plane.b() +
		phantomPos[2]*plane.c() + plane.d());

	    // END OF RECOVERY IMPLEMENTATION

	    //project the current phantomPosition onto the plane to get the SCP
	    vrpn_HapticPosition SCP = plane.projectPoint(phantomPos);
	
	    //set the SCPnormal to be the normal of the plane
	    vrpn_HapticVector SCPnormal = plane.normal();
	
	    //SCP = SCP + SCPnormal*0.001;

	    //convert the SCP and SCPnormal to the world coordinate
	    //frame and add to the collision list of Phanotom

	    // XXX We rely on VRPN not using transforms for planes.
	    SCP=toWorld(SCP);
	    SCPnormal = toWorld(SCPnormal);
	    inContact= TRUE;
	    (void)updateStateForPHANToM(PHANToM, inContact);
	    addCollision(PHANToM,SCP,SCPnormal);  
	}	

	return inContact;
}
#endif

#endif
