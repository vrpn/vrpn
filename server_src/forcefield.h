#ifndef FORCEFIELD_H
#define FORCEFIELD_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

#include "ghost.h"
#include <math.h>

/// force field effect for PHANToM

#ifdef	VRPN_USE_HDAPI
class  ForceFieldEffect
#else
class  ForceFieldEffect:public gstEffect 
#endif
{
  public:

	/// Constructor.
#ifdef	VRPN_USE_HDAPI
    ForceFieldEffect(): active(false), time(0) {}
#else
    ForceFieldEffect():gstEffect() {}
#endif

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
	virtual vrpn_HapticBoolean start() { 
//	    if (!active) { printf("starting ForceFieldEffect\n"); }
	    active = true; time = 0.0; return true;
	}

	// FOR_GHOST_EXTENSION:
	/// Stop the application of force based on the field.
	virtual void stop() { 
//	    if (active) { printf("stopping ForceFieldEffect\n"); }
	    active = false;
	}

#ifdef	VRPN_USE_HDAPI
	virtual vrpn_HapticBoolean isActive() const { return active; }
#endif

	// FOR_GHOST_EXTENSION:
	/// Calculate the force in parent reference frame of phantom.
	// When subclassing, the first 
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
	vrpn_HapticPosition origin;	//< Origin of the force field
	vrpn_HapticVector force;	//< Constant force to add to position-dependent forces
	double radius;			//< Distance from origin at which the field drops to zero
	double jacobian[3][3];		//< Describes increase in force away from origin in different directions
};

#endif
#endif
