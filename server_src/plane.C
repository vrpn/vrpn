#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#include "plane.h"

//#define VERBOSE
  
#ifdef	VRPN_USE_HDAPI
  #include <HD/hd.h>
  #include <HDU/hduError.h>
#endif

#ifndef	VRPN_USE_HDAPI
gstType Plane::PlaneClassTypeId;
static int temp = Plane::initClass();
#endif

//constructors
Plane::Plane(const vrpn_HapticPlane & p)
{ plane= vrpn_HapticPlane(p);
  inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
#ifndef	VRPN_USE_HDAPI
  invalidateCumTransf();
#endif
}

Plane::Plane(const vrpn_HapticPlane * p)
{ plane = vrpn_HapticPlane(*p);
  inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
#ifndef	VRPN_USE_HDAPI
  invalidateCumTransf();
#endif
}

Plane::Plane(const Plane *p)
{ inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
#ifndef	VRPN_USE_HDAPI
  invalidateCumTransf();
#endif
  plane = vrpn_HapticPlane(p->plane);
}

Plane::Plane(const Plane &p)
{ plane = vrpn_HapticPlane(p.plane);
  inEffect= FALSE;
  isNewPlane = FALSE;
  isInRecovery = FALSE;
  recoveryPlaneCount = 0;
  originalPlane = plane;
  lastDepth = 0;
  dIncrement = 0;
  numRecoveryCycles = 1;
#ifndef	VRPN_USE_HDAPI
  invalidateCumTransf();
#endif
}


Plane::Plane(double a,double b, double c, double d) 
{ vrpn_HapticVector vec = vrpn_HapticVector(a,b,c);
  plane = vrpn_HapticPlane(vec,d);
#ifndef	VRPN_USE_HDAPI
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
#ifdef	VRPN_USE_HDAPI
    //XXX We rely on planes only using world-space coordinates in VRPN.
    //XXX Actually, there are now transformations, which need to be worked into the equations.
    vrpn_HapticPosition P1 = startPt_WC;
    vrpn_HapticPosition P2 = endPt_WC;
#else
    vrpn_HapticPosition P1 = fromWorld(startPt_WC);
    vrpn_HapticPosition P2 = fromWorld(endPt_WC);
#endif

    // XXX This always returns false here for GHOST, but that doesn't seem right...
    // How could it possibly have been working?
#ifndef	VRPN_USE_HDAPI
    return FALSE;
#endif

#ifdef	VRPN_USE_HDAPI
    double t;
    if (plane.intersectSegmentFront( hduLineSegmentd(P1,P2), t, 1e-9 ) == vrpn_HapticPlane::ResultNone) {
      return FALSE;
    } else {
      //XXX We rely on planes only using world-space coordinates in VRPN
      intersectionPt_WC = P1 + t * (P2 - P1);
      intersectionPtNormal_WC = plane.normal();
    }
#else
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
#endif

    return TRUE;
}


#ifdef	VRPN_USE_HDAPI
vrpn_HapticBoolean Plane::collisionDetect(vrpn_HapticCollisionState *collision)
#else
vrpn_HapticBoolean Plane::collisionDetect(gstPHANToM *PHANToM)
#endif
{ 
	vrpn_HapticPosition lastSCP, phantomPos, intersectionPoint;
	vrpn_HapticBoolean  inContact;
	double depth, d_new, d_goal;

	//if the plane node is not in effect
	if(inEffect== FALSE)
		return FALSE;

#ifdef	VRPN_USE_HDAPI
	inContact = collision->inContact();
#else
	inContact = getStateForPHANToM(PHANToM);
#endif

#ifndef	VRPN_USE_HDAPI
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
#endif

#ifdef	VRPN_USE_HDAPI
	phantomPos = collision->d_currentPosition;
#else	
	PHANToM->getPosition_WC(phantomPos);
	phantomPos= fromWorld(phantomPos);
#endif
	
	// note: this depth is not necessarily in the same units
	// as the coordinates because the plane equation is the
	// same if we multiply it by a constant but this doesn't matter
	// for our purposes since we are only interested in making 
	// sure the perceived depth does not change too quickly and
	// this is done by interpolation
#ifdef	VRPN_USE_HDAPI
	depth = -plane.perpDistance(phantomPos);
#else
	depth = -plane.error(phantomPos);
#endif
	if (depth <= 0) {	// positive depth is below surface
	    inContact = FALSE;
#ifdef	VRPN_USE_HDAPI
	    collision->updateState(inContact);
#else
	    (void) updateStateForPHANToM(PHANToM,inContact);
#endif
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
#ifndef	VRPN_USE_HDAPI
	    SCP=toWorld(SCP);
	    SCPnormal = toWorld(SCPnormal);
#endif
	    inContact= TRUE;
#ifdef	VRPN_USE_HDAPI
	    collision->updateState(inContact);
	    collision->addCollision(SCP, SCPnormal);
#else
	    (void)updateStateForPHANToM(PHANToM, inContact);
	    addCollision(PHANToM,SCP,SCPnormal);  
#endif
	}	

	return inContact;
}

#endif
