#include "InstantBuzzEffect.h"
#include <gstPHANToM.h>
#include <windows.h>
#include <math.h>

gstVector InstantBuzzEffect::calcEffectForce(void *phantom) {
    // No force if inactive
    if (!active) {
        return gstVector(0,0,0);
    }

    LARGE_INTEGER counter;
	
    if (currentPerformanceFrequency.HighPart == 0 && currentPerformanceFrequency.LowPart == 0) {
	return gstVector(0,0,0);
    }
    
    if (QueryPerformanceCounter(&counter) != TRUE){
	fprintf(stderr, "unable to get perfo counter\n");
	return gstVector(0,0,0);
    }

    double elapsedSec =  (counter.QuadPart - debut.QuadPart) / (double) currentPerformanceFrequency.QuadPart;
    if (elapsedSec < getDuration()) {
	double currentPart = (counter.QuadPart*getFrequency()/currentPerformanceFrequency.QuadPart);
	currentPart = (currentPart-(long)currentPart)*2*PI;
	double currentForce = sin(currentPart);
	double force = currentForce*getAmplitude();
	return gstVector(x*force, y*force, z*force);
    }

    return gstVector(0,0,0);
}

