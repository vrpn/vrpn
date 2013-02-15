/*
# instantBuzzEffect : instantaneous buzz "custom" effect for Phantom server
# written by Sebastien MARAUX, ONDIM SA (France)
# maraux@ondim.fr
*/

#ifndef INSTANT_BUZZ_EFFECT
#define INSTANT_BUZZ_EFFECT

#ifndef	TRUE
#define TRUE 1
#endif
#ifndef	FALSE
#define FALSE 0
#endif

#include "vrpn_Shared.h"

#include <math.h>
#include <stdio.h>
#include "vrpn_Configure.h"
#include "vrpn_ForceDevice.h"

#ifdef	VRPN_USE_PHANTOM_SERVER
#include "ghost.h"

const double PI=3.1415926535897932384626433832795;

// Buzz effect for PHANToM.  This effect
// vibrates the PHANToM end point along the y-axis 
// with a given frequency, amplitude, and duration.
#ifdef	VRPN_USE_HDAPI
class InstantBuzzEffect
#else
class InstantBuzzEffect : public gstEffect 
#endif
{

public:

    // Constructor.
#ifdef	VRPN_USE_HDAPI
  InstantBuzzEffect() {
#else
  InstantBuzzEffect() : gstEffect() {
#endif
	frequency = 40.0;
	duration = 2.0;
	amplitude = 1;
	x = 0;
	y = 0;
	z = 1;
#ifdef	_WIN32
	if (QueryPerformanceFrequency(&currentPerformanceFrequency) != TRUE) {
	    fprintf(stderr, "unable to get performance counter frequency\n");
	    currentPerformanceFrequency.QuadPart = 0;
	}
	debut.QuadPart = 0;
#endif
#ifdef	VRPN_USE_HDAPI
	active = FALSE;	  // XXX Should this be true?
	time = 0;
#endif
    }

    // Destructor.
    ~InstantBuzzEffect() {}

    // Set duration of effect [seconds].
    void	setDuration(double durInSec) {
	if (durInSec <= 0) {
#ifdef	VRPN_USE_HDAPI
	  fprintf(stderr,
		"InstantBuzzEffect:setDuration Invalid Duration <= 0 (%lf)\n",durInSec);
#else
	  gstErrorHandler(GST_BOUNDS_ERROR,
		"InstantBuzzEffect:setDuration Invalid Duration <= 0",durInSec);
#endif
	    return;
	}
	duration = durInSec; 
    }

    // Get duration of effect [seconds].
    double 	getDuration() { return duration; }

    // Set "buzz" frequency [Hz].
    void	setFrequency(double newFreq) {
	if (newFreq <= 0) {
#ifdef	VRPN_USE_HDAPI
	  fprintf(stderr,
		"InstantBuzzEffect:setFrequency Invalid Frequency <= 0 (%lf)\n",newFreq);
#else
	    gstErrorHandler(GST_BOUNDS_ERROR,
	       "InstantBuzzEffect:setFrequency Invalid Frequency <= 0",newFreq);
#endif
	    return;
	}
	frequency = newFreq; 
    }

    // Get "buzz" frequency [Hz].
    double getFrequency() { return frequency; }

    // Set amplitude of effect [millimeters].
    void setAmplitude(double newAmp) {
	if (newAmp < 0) {
#ifdef	VRPN_USE_HDAPI
	  fprintf(stderr,
		"InstantBuzzEffect:setAmplitude Invalid Amplitude <= 0 (%lf)\n",newAmp);
#else
	    gstErrorHandler(GST_BOUNDS_ERROR,
		"InstantBuzzEffect:setAmplitude Invalid Amplitude < 0",newAmp);
#endif
	    return;
	}
	amplitude = newAmp;
    }

    // Get amplitude of effect [millimeters].
    double getAmplitude() { return amplitude; }

    // Set Direction of effect.
    void setDirection(double px, double py, double pz) {
	double sqnorm = x*x + y*y + z*z;
	if (sqnorm == 1 || sqnorm == 0) {
		x=px;
		y=py;
		z=pz;
	}
	else { // normalize vector to avoid unexpected amplitude.

		double norm = sqrt(sqnorm);
		x = px/norm;
		y = py/norm;
		z = pz/norm;
	}
    }

    // Get Direction of effect.
    void getDirection(double *px, double *py, double *pz) {
	*px=x;
	*py=y;
	*pz=z;
    }

    // FOR_GHOST_EXTENSION:
    // Start the effect.  WARNING:  When re-starting an effect,
    // make sure to reset any state, such as past PHANToM position.
    // Otherwise, the next call to calcEffectForce could 
    // generate unexpectedly large forces.
    /// Start the application of forces based on the field.
    virtual vrpn_HapticBoolean start() { 
	/*if (!active) { 
	    printf("starting Buzz Effect\n"); 
	}*/
#ifdef _WIN32
	LARGE_INTEGER counter;
        if (QueryPerformanceCounter(&counter) != TRUE){
	    fprintf(stderr, "unable to get perfo counter\n");
	    return FALSE;
	}
	debut = counter;
#endif
	active = TRUE;
	time = 0.0;
	return TRUE;
    }

    // FOR_GHOST_EXTENSION:
    /// Stop the application of force based on the field.
    virtual void stop() { 
    /*if (active) { 
	    printf("stopping Buzz Effect\n"); 
	}*/
      active = FALSE;
    }

    // FOR_GHOST_EXTENSION:
    // Caculate the force.  Force is returned in parent 
    // reference frame of phantom. When subclassing, the first 
    // parameter should be cast to gstPHANToM to retrieve 
    // any information about the state of the PHANToM that 
    // is needed to calculate the forces.  deltaT should 
    // be used to update the time.  Also, if the effect is
    // not active, the zero vector should be returned.
    // ACTUNG!
    // WARNING!: Never call PHANToM->setForce or
    //			PHANToM->setForce_WC from this function.
    //			It will cause an infinite recursion.
#ifdef	VRPN_USE_HDAPI
    virtual vrpn_HapticVector calcEffectForce(void);
#else
    virtual gstVector	calcEffectForce(void *phantom);
    virtual gstVector	calcEffectForce(void *phantom, gstVector &torques) {
	torques.init(0.0, 0.0, 0.0);
	return calcEffectForce(phantom);
    }
#endif

protected:
    double		frequency, amplitude, duration;
    double		x,y,z;
#ifdef _WIN32
    LARGE_INTEGER	currentPerformanceFrequency, debut;
#endif
#ifdef	VRPN_USE_HDAPI
    bool		active;
    double		time;
#endif
};

#endif // VRPN_USE_PHANTOM_SERVER
#endif // INSTANT_BUZZ_EFFECT

