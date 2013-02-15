/*	plane.h 
	Plane primitive. This is a user defined geometry node which 
	extends the GHOST SDK. 
	This class represents a planar surface
  */

#ifndef PLANE_H
#define PLANE_H

#include "vrpn_Configure.h"


#ifdef	VRPN_USE_PHANTOM_SERVER
#include "vrpn_Shared.h"


#include "ghost.h"

class Plane:public vrpn_HapticSurface
{
public:
	//Constructor
	Plane(const vrpn_HapticPlane *);

	Plane(const vrpn_HapticPlane &);

	//Constructor
	Plane(const Plane *);

	//Constructor
	Plane(const Plane &);

	//Constructor
	Plane(double a, double b, double c, double d);

	//Destructor
	~Plane() {}



	void update(double a, double b, double c, double d);

	void setPlane(vrpn_HapticVector newNormal, vrpn_HapticPosition point);

	static int initClass() {
		//PlaneClassTypeId = gstUniqueId++ ;
		return 0;}

	void setInEffect(vrpn_HapticBoolean effect) {inEffect = effect;}

	void setNumRecCycles(int nrc) {
			if (nrc < 1) 
			   printf("Error: setRecCycles(cycles < 1): %d\n",nrc);
			else
			   numRecoveryCycles = nrc;}

#ifdef	VRPN_USE_HDAPI
        HLuint  myId;
        void  getHLId(void) { myId = hlGenShapes(1); };
        void  releaseHLId(void) { hlDeleteShapes(myId, 1); };
#else
	// Get type of this class.  No instance needed.
	static gstType	getClassTypeId() { return PlaneClassTypeId; }

	virtual gstType getTypeId() const { return PlaneClassTypeId;}

	// GHOST_IGNORE_IN_DOC:
	// Returns TRUE if subclass is of type.
	static vrpn_HapticBoolean	staticIsOfType(gstType type) {
		if (type == PlaneClassTypeId) return TRUE;
		else return (gstShape::staticIsOfType(type));}

	virtual vrpn_HapticBoolean isOftype(gstType type) const {
	    if( type == PlaneClassTypeId) return TRUE;
		    else return (gstShape::isOfType(type)); } 
#endif

#ifdef	VRPN_USE_HDAPI
        void renderHL(void);

        // intersect the line segment from startPt to endPt with
        // the surface.  Return the closest point of intersection 
        // to the start point in intersectionPt.  Return the
        // surface normal at intersectionPt in intersectionNormal.
        // Return which face (HL_FRONT or HL_BACK) is being touched.
        // Return true if there is an intersection.
        static bool HLCALLBACK intersectSurface(
            const HLdouble startPt_WC[3], 
            const HLdouble endPt_WC[3],
            HLdouble intersectionPt_WC[3], 
            HLdouble intersectionNormal_WC[3],
            HLenum *face,
            void *userdata);

        // find the closest surface feature(s) to queryPt
        static bool HLCALLBACK closestSurfaceFeatures(
            const HLdouble queryPt_WC[3], 
            const HLdouble targetPt_WC[3],
            HLgeom *geom_WC,
            HLdouble closestPt_WC[3],
            void *userdata);
#else
	// FOR_GHOST_EXTENSION:
	// Used by system or for creating sub-classes only.
	//  Returns TRUE if line segment defined by startPt_WC and endPt_WC
	//  intersects this shape object.  If so, intersectionPt_WC is set to
	//  point of intersection and intersectionNormal_WC is set to surface
	//  normal at intersection point.
	virtual vrpn_HapticBoolean	intersection(	const vrpn_HapticPosition &startPt_WC,
							const vrpn_HapticPosition &endPt_WC,
							vrpn_HapticPosition &intersectionPt_WC,
							vrpn_HapticVector &intersectionPtNormal_WC,
							void **);

	// Returns TRUE if PHANToM is currently in contact with this object.
	// If so, the collision is added to the PHANToM's list through
	// gstPHANToM::getCollisionInfo() and gstPHANToM::collisionAdded().
	virtual vrpn_HapticBoolean	collisionDetect(gstPHANToM *PHANToM) ;
#endif
	
#ifndef	VRPN_USE_HDAPI
	void printSelf2() const { plane.printSelf2(); }
#endif

#ifdef	VRPN_USE_HDAPI
	const vrpn_HapticPlane &getPlane(void) const { return plane; }
	bool getActive(void) const { return (inEffect != 0); }
	void setActive(vrpn_HapticBoolean active) { setInEffect(active); }
#endif

protected:
	vrpn_HapticPlane plane;
	vrpn_HapticBoolean inEffect;

	// variables used for recovery
	vrpn_HapticBoolean isNewPlane;	// set to true by update(), set to false
					// 	by collisionDetect()
	vrpn_HapticBoolean isInRecovery;// true after collisionDetect() discovers
					// 	there is a new plane and set to
					//	false when recoveryPlaneCount
					//	reaches numRecoveryCycles
	int recoveryPlaneCount;		// keeps track of how many intermediate
					// 	planes there have been 
	vrpn_HapticPlane originalPlane;	// keeps a copy of the plane set by
					// 	update() so that we can restore this
					//	plane when recovery is complete
        double lastDepth;		// this is the depth below the surface
					// 	for the plane previous to the
					//	new plane - updated in collisionDetect()
	double dIncrement;		// value added to d parameter of plane
					//	equation with each call to
					//	collisionDetect() during a recovery
					//	- this value is computed when a new
					//	plane is received
	int numRecoveryCycles;		// number of recovery cycles

	// end of variables for recovery

private:
#ifndef	VRPN_USE_HDAPI
  static gstType PlaneClassTypeId;
#endif
};

#endif
#endif
