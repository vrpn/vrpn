#ifndef CONSTRAINT_H
#define CONSTRAINT_H

#include "GHOST.H"
#include <math.h>

// Constraint effect for PHANToM

class  ConstraintEffect:public gstEffect 

{
  public:

	// Constructor.
	ConstraintEffect():gstEffect(){}

	// Destructor.
	~ConstraintEffect() {}

	// set the point to which this effect is attached
	// units are mm and Newtons/mm
	void setPoint(double *pnt, double kSpr) {
		fixedEnd = gstPoint(pnt[0],pnt[1],pnt[2]);
		kSpring = kSpr;
	}

	// FOR_GHOST_EXTENSION:
	// Start the effect.  WARNING:  When re-starting an effect,
        // make sure to reset any state, such as past PHANToM position.
    	// Otherwise, the next call to calcEffectForce could 
	// generate unexpectedly large forces.
	virtual gstBoolean start() { 
		if (!active) printf("starting force\n");
		active = TRUE; time = 0.0; return TRUE;}

	// FOR_GHOST_EXTENSION:
	// Stop the effect.
	virtual void stop() { 
		if (active) printf("stopping force\n");
		active = FALSE;}

	// FOR_GHOST_EXTENSION:
	// Calculate the force.  Force is returned in parent 
	// reference frame of phantom. When subclassing, the first 
	// parameter should be cast to gstPHANToM to retrieve 
	// any information about the state of the PHANToM that 
	// is needed to calculate the forces.  deltaT should 
	// be used to update the time.  Also, if the effect is
	// not active, the zero vector should be returned.
	// ACTUNG!
	// WARNING!: Never call PHANToM->setForce or
	//			PHANToM->setForce_WC from this function.
	//			It will cause an infinite recursion.
	virtual gstVector	calcEffectForce(void *phantom);


  protected:

	gstPoint fixedEnd;
	double kSpring;
};



#endif // GST_ADHESION_EFFECT
