/*	plane.h 
	Plane primitive. This is a user defined geometry node which 
	extends the GHOST SDK. 
	This class represents a planar surface
  */

#ifndef PLANE_H
#define PLANE_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
/*
#include <gstPHANToM.h>
#include <gstShape.h>
*/ 
#include "ghost.h"

class Plane:public gstShape
{
public:
	//Constructor
	Plane(const gstPlane *);

	Plane(const gstPlane &);

	//Constructor
	Plane(const Plane *);

	//Constructor
	Plane(const Plane &);

	//Constructor
	Plane(double a, double b, double c, double d);
	//desstructor
	~Plane(){}

	void update(double a, double b, double c, double d);

	void setPlane(gstVector newNormal, gstPoint point);

	static int initClass() {
		//PlaneClassTypeId = gstUniqueId++ ;
		return 0;}

	void setInEffect(gstBoolean effect) {inEffect = effect;}

	void setNumRecCycles(int nrc) {
			if (nrc < 1) 
			   printf("Error: setRecCycles(cycles < 1): %d\n",nrc);
			else
			   numRecoveryCycles = nrc;}

	// Get type of this class.  No instance needed.
	static gstType	getClassTypeId() { return PlaneClassTypeId; }

	virtual gstType getTypeId() const { return PlaneClassTypeId;}

	// GHOST_IGNORE_IN_DOC:
	// Returns TRUE if subclass is of type.
	static gstBoolean	staticIsOfType(gstType type) {
		if (type == PlaneClassTypeId) return TRUE;
		else return (gstShape::staticIsOfType(type));}

    virtual gstBoolean isOftype(gstType type) const {
        if( type == PlaneClassTypeId) return TRUE;
		else return (gstShape::isOfType(type)); } 

	// FOR_GHOST_EXTENSION:
	// Used by system or for creating sub-classes only.
	//  Returns TRUE if line segment defined by startPt_WC and endPt_WC
	//  intersects this shape object.  If so, intersectionPt_WC is set to
	//  point of intersection and intersectionNormal_WC is set to surface
	//  normal at intersection point.
	virtual gstBoolean	intersection(	const gstPoint &startPt_WC,
										const gstPoint &endPt_WC,
										gstPoint &intersectionPt_WC,
										gstVector &intersectionPtNormal_WC,
										void **);

	// Used by system or for creating sub-classes only.
	// Returns TRUE if PHANToM is currently in contact with this object.
	// If so, the collision is added to the PHANToM's list through
	// gstPHANToM::getCollisionInfo() and gstPHANToM::collisionAdded().
	virtual gstBoolean	collisionDetect(gstPHANToM *PHANToM) ;

	
	// Used by system or for creating sub-classes only.
	// Returns TRUE if pt is inside of this object.
	//virtual int	checkIfPointIsInside_WC(const gstPoint &pt) = 0;

    void printSelf2() const { plane.printSelf2(); }
protected:
	gstPlane plane;
	gstBoolean inEffect;

	// variables used for recovery
	gstBoolean isNewPlane;	// set to true by update(), set to false
				// 	by collisionDetect()
	gstBoolean isInRecovery;// true after collisionDetect() discovers
				// 	there is a new plane and set to
				//	false when recoveryPlaneCount
				//	reaches numRecoveryCycles
	int recoveryPlaneCount;	// keeps track of how many intermediate
				// 	planes there have been 
	gstPlane originalPlane;	// keeps a copy of the plane set by
				// 	update() so that we can restore this
				//	plane when recovery is complete
        double lastDepth;	// this is the depth below the surface
				// 	for the plane previous to the
				//	new plane - updated in collisionDetect()
	double dIncrement;	// value added to d parameter of plane
				//	equation with each call to
				//	collisionDetect() during a recovery
				//	- this value is computed when a new
				//	plane is encountered
	int numRecoveryCycles;	// number of recovery cycles

	// end of variables for recovery

private:
	static gstType PlaneClassTypeId;
};

#endif
#endif
