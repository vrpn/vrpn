#include "GHOST.H"
#include "constraint.h"

#define MAX_FORCE 10.0	// we try not to exceed this
						// force [dyne], this gets pretty close
						// to the maximum PHANToM force
#define MAX_DIST 50.0	// maximum distance at which effect
						// force can be felt [mm]

gstVector ConstraintEffect::calcEffectForce(void *PHANToM){

	gstPHANToM *phantom = (gstPHANToM *)PHANToM;
	gstPoint phantomPos;
	gstPoint lastPhantomPos;
	gstVector phantomVel;
	gstVector phantomForce;
	gstVector effectForce;
	double dt;
	int i;
	phantom->getPosition_WC(phantomPos);
	phantomForce = phantom->getReactionForce_WC();
	phantom->getLastPosition_WC(lastPhantomPos);
	phantomVel = phantom->getVelocity();
	dt = phantom->getDeltaT();

	time += dt;

	if (active) {
		gstPoint forceVec = (fixedEnd - phantomPos);
		double dist = forceVec.distToOrigin();
		effectForce = forceVec;
		for (i = 0; i < 3; i++)
		    effectForce[i] *= kSpring;
		// set effectForce to (0,0,0) if given our velocity
		// we may have already reached the fixedEnd
		if ((dist < phantomVel.distToOrigin()*dt) || (dist > MAX_DIST))
		    return gstVector(0,0,0);
		else if (dist*kSpring > MAX_FORCE){
			for (i = 0; i < 3; i++)
				effectForce[i] *= MAX_FORCE/(dist*kSpring);
			return effectForce;
		}
		else
		    return effectForce;
	}
	else {
		return gstVector(0,0,0);
	}

}

