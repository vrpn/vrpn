/*
# instantBuzzEffect : instantaneous buzz "custom" effect for Phantom server
# written by Sebastien MARAUX, ONDIM SA (France)
# maraux@ondim.fr
*/

#include "InstantBuzzEffect.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#ifdef	VRPN_USE_HDAPI
#include <HD/hd.h>
#else
#include <gstPHANToM.h>
#endif
#include <windows.h>
#include <math.h>
#include "InstantBuzzEffect.h"

#ifdef	VRPN_USE_HDAPI
vrpn_HapticVector  InstantBuzzEffect::calcEffectForce(void) {
#else
gstVector InstantBuzzEffect::calcEffectForce(void *phantom) {
#endif
    // No force if inactive
    if (!active) {
      return vrpn_HapticVector(0,0,0);
    }

    LARGE_INTEGER counter;
	
    if (currentPerformanceFrequency.HighPart == 0 && currentPerformanceFrequency.LowPart == 0) {
      return vrpn_HapticVector(0,0,0);
    }
    
    if (QueryPerformanceCounter(&counter) != TRUE){
	fprintf(stderr, "unable to get perfo counter\n");
      return vrpn_HapticVector(0,0,0);
    }

    double elapsedSec =  (counter.QuadPart - debut.QuadPart) / (double) currentPerformanceFrequency.QuadPart;
    if (elapsedSec < getDuration()) {
	double currentPart = (counter.QuadPart*getFrequency()/currentPerformanceFrequency.QuadPart);
	currentPart = (currentPart-(long)currentPart)*2*PI;
	double currentForce = sin(currentPart);
	double force = currentForce*getAmplitude();
	return vrpn_HapticVector( x*force, y*force, z*force );
    }

      return vrpn_HapticVector(0,0,0);
}

#endif // VRPN_USE_PHANTOM_SERVER
