#include "GHOST.H"
#include "forcefield.h"

#define FF_MAX_FORCE 10.0	// we try not to exceed this
			// force [dyne], this gets pretty close
			// to the maximum PHANToM force

gstVector ForceFieldEffect::calcEffectForce(void *PHANToM){

	gstPHANToM *phantom = (gstPHANToM *)PHANToM;
	gstPoint phantomPos;
	gstPoint lastPhantomPos;
	gstVector phantomVel;
	gstVector phantomForce;
	gstVector effectForce = gstVector(0,0,0);
	double forceMag;
	double dt;
	int i,j;
	phantom->getPosition_WC(phantomPos);
/*	phantomForce = phantom->getReactionForce_WC();
	phantom->getLastPosition_WC(lastPhantomPos);
	phantomVel = phantom->getVelocity();
	dt = phantom->getDeltaT();

	time += dt;
*/
	if (active) {
		gstVector dR = (phantomPos - origin);
		if (dR.norm() > radius) return gstVector(0,0,0);

		effectForce = force;
		for (i = 0; i < 3; i++)
		    for (j = 0; j < 3; j++)
			effectForce[i] += dR[j]*jacobian[i][j];
		
		// clamp to FF_MAX_FORCE if it is exceeded
		forceMag = effectForce.norm();
		if (forceMag > FF_MAX_FORCE){
			for (i = 0; i < 3; i++)
				effectForce[i] *= FF_MAX_FORCE/(forceMag);
			return effectForce;
		}
		else
		    return effectForce;
	}
	else {
		return gstVector(0,0,0);
	}
}

