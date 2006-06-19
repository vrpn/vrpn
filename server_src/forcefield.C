#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#ifndef	VRPN_USE_HDAPI
#include "gstPHANToM.h"
#endif
#include "forcefield.h"

/// Do not to exceed this force (in dynes).
// It is pretty close to the maximum PHANToM force.
const double FF_MAX_FORCE = 10.0;

vrpn_HapticVector ForceFieldEffect::calcEffectForce(void *phantom_info) {

	vrpn_HapticPosition phantomPos;
	vrpn_HapticVector effectForce = vrpn_HapticVector(0,0,0);
	double forceMag;
	int i,j;

	if (active) {
		// Find out the vector from the current position to the origin
		// of the defined force field.
#ifdef	VRPN_USE_HDAPI
		HDAPI_state *state = (HDAPI_state *)phantom_info;
		double vec[3];
		vec[0] = state->pose[3][0]; vec[1] = state->pose[3][1]; vec[2] = state->pose[3][2];
		phantomPos = vrpn_HapticPosition(vec);
#else
		gstPHANToM *phantom = (gstPHANToM *)phantom_info;
		phantom->getPosition_WC(phantomPos);
#endif
		vrpn_HapticVector dR = (phantomPos - origin);

		// If the Phantom has been moved too far from the origin,
		// drop the force to zero.

#ifdef	VRPN_USE_HDAPI
		if (dR.magnitude() > radius) {
#else
		if (dR.norm() > radius) {
#endif
		    return vrpn_HapticVector(0,0,0);
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

#ifdef	VRPN_USE_HDAPI
		forceMag = effectForce.magnitude();
#else
		forceMag = effectForce.norm();
#endif
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
