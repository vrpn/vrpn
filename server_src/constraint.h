#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

#include "ghost.h"
#include <math.h>

/// Constraint effect for PHANToM (superceded by ForceField)

#ifdef	VRPN_USE_HDAPI
class  ConstraintEffect
#else
class  ConstraintEffect:public gstEffect
#endif

{
  public:

	// Constructor.
#ifdef	VRPN_USE_HDAPI
    ConstraintEffect(): active(false), time(0) { }
#else
    ConstraintEffect(): gstEffect() {}
#endif

	// Destructor.
	~ConstraintEffect() {}

	// set the point to which this effect is attached
	// units are mm and Newtons/mm
	void setPoint(double *pnt, double kSpr) {
		fixedEnd = vrpn_HapticPosition(pnt[0],pnt[1],pnt[2]);
		kSpring = kSpr;
	}

	// FOR_GHOST_EXTENSION:
	// Start the effect.  WARNING:  When re-starting an effect,
        // make sure to reset any state, such as past PHANToM position.
    	// Otherwise, the next call to calcEffectForce could 
	// generate unexpectedly large forces.
	virtual vrpn_HapticBoolean start() { 
//		if (!active) printf("starting force\n");
		active = true; time = 0.0; return true;}

	// FOR_GHOST_EXTENSION:
	// Stop the effect.
	virtual void stop() { 
//		if (active) printf("stopping force\n");
		active = false;}

	// FOR_GHOST_EXTENSION:
	// Calculate the force.  Force is returned in parent 
	// reference frame of phantom. When subclassing, the first 
	// parameter should be cast to gstPHANToM to retrieve 
	// any information about the state of the PHANToM that 
	// is needed to calculate the forces.  deltaT should 
	// be used to update the time.  Also, if the effect is
	// not active, the zero vector should be returned.
	// ACHTUNG!
	// WARNING!: Never call PHANToM->setForce or
	//			PHANToM->setForce_WC from this function.
	//			It will cause an infinite recursion.
	virtual vrpn_HapticVector	calcEffectForce(void *phantom_info);

  protected:
#ifdef	VRPN_USE_HDAPI
	vrpn_HapticBoolean	active;
	double			time;
#endif
	vrpn_HapticPosition fixedEnd;
	double kSpring;
};

#endif
#endif
