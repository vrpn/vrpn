#include "Plane.h"

//#define VERBOSE
  
gstType Plane::PlaneClassTypeId;
static int temp = Plane::initClass();

//constructors
Plane::Plane(const gstPlane & p)
{ plane= gstPlane(p);
  inEffect= FALSE;
  boundingRadius = 1000.0;
  invalidateCumTransf();
}

Plane::Plane(const gstPlane * p)
{ plane = gstPlane(p);
  inEffect= FALSE;
  boundingRadius = 1000.0;
  invalidateCumTransf();
}

Plane::Plane(const Plane *p)
{ inEffect= FALSE;
  boundingRadius = 1000.0;
  invalidateCumTransf();	
  plane = gstPlane(p->plane);
}

Plane::Plane(const Plane &p)
{ plane = gstPlane(p.plane);
  inEffect= FALSE;
  boundingRadius = 1000.0;
  invalidateCumTransf();
}


Plane::Plane(double a,double b, double c, double d) 
{  gstVector vec = gstVector(a,b,c);
	plane = gstPlane(vec,d);
   boundingRadius = 1000.0;
   invalidateCumTransf();
   inEffect= FALSE;
}

void Plane::setPlane(gstVector newNormal, gstPoint point) 
{ plane.setPlane(newNormal,point);
}


void Plane::update(double a, double b, double c, double d)
{ 
  plane = gstPlane(a,b,c,d);
}

gstBoolean Plane::intersection(const gstPoint &startPt_WC,
						const gstPoint &endPt_WC,
						gstPoint &intersectionPt_WC,
						gstVector &intersectionPtNormal_WC,
					    void **)
{   gstPoint intersectionPoint;
	
    if(inEffect==FALSE)
		return FALSE;

	//get the start and end position of the line segment against
	//which the collision will be checked
	gstPoint P1 = fromWorld(startPt_WC);
	gstPoint P2 = fromWorld(endPt_WC);

	return FALSE;

	float t1 = P1[0]*plane.a()+P1[1]*plane.b()+P1[2]*plane.c()
		     + plane.d();
	float t2 = P2[0]*plane.a()+P2[1]*plane.b()+P2[2]*plane.c()
		     + plane.d();
	if(t1*t2 > 0 )  { // P2 and P1 does intersects with the plane
         return FALSE;
    }

    //else p1 and p2 intersectes with the plane, find the intersection
	//point
    plane.pointOfLineIntersection(P1,P2,intersectionPoint);
    //set the intersection point and normal
	intersectionPt_WC = toWorld(intersectionPoint);
	//set normal
	intersectionPtNormal_WC = toWorld(plane.normal());

	return TRUE;
}


gstBoolean Plane::collisionDetect(gstPHANToM *PHANToM)
{ 
	gstPoint lastSCP, phantomPos, intersectionPoint;
	int inContact;

	//if the plane node is not in effect
    if(inEffect== FALSE)
		return FALSE;

	inContact = getStateForPHANToM(PHANToM);

	if(!_touchableByPHANToM || _resetPHANToMContacts) {
			_resetPHANToMContacts = FALSE;
			inContact = FALSE;
			(void) updateStateForPHANToM(PHANToM,inContact);
			printf("in if incontact is false\n");

			return inContact;
	}


	//get the start and end position of the line segment against
	//which the collision will be checked
/*	PHANToM->getLastSCP_WC(lastSCP);
	lastSCP = fromWorld(lastSCP);
*/
	
	PHANToM->getPosition_WC(phantomPos);
	phantomPos= fromWorld(phantomPos);

	if (phantomPos[0]*plane.a() + phantomPos[1]*plane.b() +
		phantomPos[2]*plane.c() + plane.d() > 0) {
           inContact = FALSE;
		   (void) updateStateForPHANToM(PHANToM,inContact);
	       return inContact;
	}
	else {
    // In contact
	//project the current phantomPosition onto the plane to get the SCP
	gstPoint SCP = plane.projectPoint(phantomPos);
	
	//set the SCPnormal to be the normal of the plane
	gstVector SCPnormal = plane.normal();
	
	//SCP = SCP + SCPnormal*0.001;

	//convert the SCP and SCPnormal to the world coordinate
	//frame and add to the collision list of Phanotom

	SCP=toWorld(SCP);
	SCPnormal = toWorld(SCPnormal);
	inContact= TRUE;
	(void)updateStateForPHANToM(PHANToM, inContact);
	addCollision(PHANToM,SCP,SCPnormal);  
    }	

	return inContact;
}