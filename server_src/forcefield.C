#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#include "gstPHANToM.h"
#include "forcefield.h"

/// Do not to exceed this force (in dynes).
// It is pretty close to the maximum PHANToM force.
const double FF_MAX_FORCE = 10.0;

gstVector ForceFieldEffect::calcEffectForce(void *PHANToM) {

	gstPHANToM *phantom = (gstPHANToM *)PHANToM;
	gstPoint phantomPos;
	gstVector effectForce = gstVector(0,0,0);
	double forceMag;
	int i,j;

	if (active) {
		// Find out the vector from the current position to the origin
		// of the defined force field.
		phantom->getPosition_WC(phantomPos);
		gstVector dR = (phantomPos - origin);

		// If the Phantom has been moved too far from the origin,
		// drop the force to zero.

		if (dR.norm() > radius) {
		    return gstVector(0,0,0);
		}

		// Compute the force, which is the constant force plus
		// a force that varies as the Phantom position deviates
		// from the origin of the force field.  The Jacobian
		// determines how the force changes in different directions
		// away from the origin (generalizing spring forces of different
		// magnitudes that constrain the Phantom to the point of the
		// origin, to a line containing the origin, or to a plane
		// containing the origin).  Tom Hudson worked out the math
		// on this.

		effectForce = force;
		for (i = 0; i < 3; i++) {
		    for (j = 0; j < 3; j++) {
			effectForce[i] += dR[j]*jacobian[i][j];
		    }
		}
		
		// Clamp to FF_MAX_FORCE if it is exceeded, leaving the
		// direction of the force unchanged.

		forceMag = effectForce.norm();
		if (forceMag > FF_MAX_FORCE) {
		    for (i = 0; i < 3; i++) {
			effectForce[i] *= FF_MAX_FORCE/(forceMag);
		    }
		}
	}

	// Return the force, which will be zero by default if the
	// force field is not active.
	return effectForce;
}

#endif
