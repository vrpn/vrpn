#ifndef FORCEFIELD_H
#define FORCEFIELD_H

#include "ghost.h"
//#include "gstEffect.h"
#include <math.h>

/// force field effect for PHANToM

class  ForceFieldEffect:public gstEffect 
{
  public:

	/// Constructor.
	ForceFieldEffect():gstEffect() {}

	/// Destructor.
	~ForceFieldEffect() {}

	/// Sets the member variables to the specified values
	void setForce(float ori[3], float f[3], float jm[3][3],
		float r) {
	    int i,j;
	    for (i=0; i < 3; i++) {
		origin[i] = ori[i];
		force[i] = f[i];
		for (j=0; j < 3; j++) {
		    jacobian[i][j] = jm[i][j];
		}
	    }
	    radius = r;
	}

	// FOR_GHOST_EXTENSION:
	// Start the effect.  WARNING:  When re-starting an effect,
        // make sure to reset any state, such as past PHANToM position.
    	// Otherwise, the next call to calcEffectForce could 
	// generate unexpectedly large forces.
	/// Start the application of forces based on the field.
	virtual gstBoolean start() { 
	    if (!active) { printf("starting ForceFieldEffect\n"); }
	    active = TRUE; time = 0.0; return TRUE;
	}

	// FOR_GHOST_EXTENSION:
	/// Stop the application of force based on the field.
	virtual void stop() { 
	    if (active) { printf("stopping ForceFieldEffect\n"); }
	    active = FALSE;
	}

	// FOR_GHOST_EXTENSION:
	/// Calculate the force in parent reference frame of phantom.
	// When subclassing, the first 
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

	gstPoint origin;	//< Origin of the force field
	gstVector force;	//< Constant force to add to position-dependent forces
	double radius;		//< Distance from origin at which the field drops to zero
	double jacobian[3][3];	//< Describes increase in force away from origin in different directions
};

#endif
