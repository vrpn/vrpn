/*	plane.h 
	Plane primitive. This is a user defined geometry node which 
	extends the GHOST SDK. 
	This class represents a planar surface
  */

#ifndef PLANE_H
#define PLANE_H
/*
#include <gstPHANToM.h>
#include <gstShape.h>
*/ 
#include "GHOST.H"
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

	static int initClass() {PlaneClassTypeId = gstUniqueId++ ;return 0;}

	void setInEffect(gstBoolean effect) {inEffect = effect;}

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
	//virtual int			checkIfPointIsInside_WC(const gstPoint &pt) = 0;

    void printSelf2() const { plane.printSelf2(); }
protected:
	gstPlane plane;
	gstBoolean inEffect;

private:
	static gstType PlaneClassTypeId;
};

#endif