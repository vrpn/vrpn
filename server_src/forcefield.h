#ifndef FORCEFIELD_H
#define FORCEFIELD_H

#include "gstEffect.h"
#include <math.h>

// force field effect for PHANToM

class  ForceFieldEffect:public gstEffect 

{
  public:

	// Constructor.
	ForceFieldEffect():gstEffect(){}

	// Destructor.
	~ForceFieldEffect() {}

	void setForce(float ori[3], float f[3], float jm[3][3],
		float r) {
	    int i,j;
	    for (i=0; i < 3; i++){
		origin[i] = ori[i];
		force[i] = f[i];
		for (j=0; j < 3; j++)
		    jacobian[i][j] = jm[i][j];
	    }
	    radius = r;
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

	gstPoint origin;
	gstVector force;
	double radius;
	double jacobian[3][3];
};



#endif
