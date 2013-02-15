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

#include <math.h>

#ifdef	VRPN_USE_HDAPI
vrpn_HapticVector  InstantBuzzEffect::calcEffectForce(void) {
#else
gstVector InstantBuzzEffect::calcEffectForce(void *phantom) {
#endif
    // No force if inactive
    if (!active) {
      return vrpn_HapticVector(0,0,0);
    }

// XXX Needs to be implemented on non-Windows platforms.  Use
// the gettimeofday() function to calculate timing on those platforms.
// We might want to switch to vrpn_gettimeofday() on all platforms, since
// that is now implemented on Windows.
#ifdef	_WIN32
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
#endif

      return vrpn_HapticVector(0,0,0);
}

#endif // VRPN_USE_PHANTOM_SERVER
