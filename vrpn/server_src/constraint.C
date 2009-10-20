#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

#ifndef	VRPN_USE_HDAPI
#include <gstPHANToM.h>
#endif
#include "constraint.h"


#define MAX_FORCE 10.0	// we try not to exceed this
			// force [dyne], this gets pretty close
			// to the maximum PHANToM force
#define MAX_DIST 50.0	// maximum distance at which effect
			// force can be felt [mm]

vrpn_HapticVector ConstraintEffect::calcEffectForce(void *phantom_info) {

	vrpn_HapticPosition phantomPos;
	vrpn_HapticPosition lastPhantomPos;
	vrpn_HapticVector phantomVel;
	vrpn_HapticVector effectForce;
	double dt;
	int i;
#ifdef	VRPN_USE_HDAPI
	HDAPI_state *state = (HDAPI_state *)phantom_info;
	int rate = state->instant_rate;
	dt = 1.0 / rate;
	double vec[3];
	vec[0] = state->pose[3][0]; vec[1] = state->pose[3][1]; vec[2] = state->pose[3][2];
	phantomPos = vrpn_HapticPosition(vec);
	double lastvec[3];
	lastvec[0] = state->last_pose[3][0]; lastvec[1] = state->last_pose[3][1]; lastvec[2] = state->last_pose[3][2];
	lastPhantomPos = vrpn_HapticPosition(lastvec);
	double	velvec[3];
	velvec[0] = (vec[0] - lastvec[0]) / dt;
	velvec[1] = (vec[1] - lastvec[1]) / dt;
	velvec[2] = (vec[2] - lastvec[2]) / dt;
	phantomVel = vrpn_HapticVector(velvec);
#else
	gstPHANToM *phantom = (gstPHANToM *)phantom_info;
	phantom->getPosition_WC(phantomPos);
	phantom->getLastPosition_WC(lastPhantomPos);
	phantomVel = phantom->getVelocity();
	dt = phantom->getDeltaT();
#endif

	time += dt;

	if (active) {
		vrpn_HapticPosition forceVec = (fixedEnd - phantomPos);
#ifdef	VRPN_USE_HDAPI
		double dist = forceVec.magnitude();
#else
		double dist = forceVec.distToOrigin();
#endif
		effectForce = forceVec;
		for (i = 0; i < 3; i++) {
		    effectForce[i] *= kSpring;
		}
		// set effectForce to (0,0,0) if given our velocity
		// we may have already reached the fixedEnd
#ifdef	VRPN_USE_HDAPI
		if ((dist < phantomVel.magnitude()*dt) || (dist > MAX_DIST)) {
#else
		if ((dist < phantomVel.distToOrigin()*dt) || (dist > MAX_DIST)) {
#endif
                  return vrpn_HapticVector(0,0,0);
		} else if (dist*kSpring > MAX_FORCE){
		  for (i = 0; i < 3; i++) {
		    effectForce[i] *= MAX_FORCE/(dist*kSpring);
		  }
		  return effectForce;
		} else {
		  return effectForce;
		}
	} else {
		return vrpn_HapticVector(0,0,0);
	}
}

#endif
