#ifndef INSTANT_BUZZ_EFFECT
#define INSTANT_BUZZ_EFFECT

#ifdef	VRPN_USE_PHANTOM_SERVER
#include <gstBasic.h>
#include <math.h>
#include <gstEffect.h>
#include <gstErrorHandler.h>

const double PI=3.1415926535897932384626433832795;

// Buzz effect for PHANToM.  This effect
// vibrates the PHANToM end point along the y-axis 
// with a given frequency, amplitude, and duration.
class InstantBuzzEffect : public gstEffect 
{

public:

    // Constructor.
    InstantBuzzEffect() : gstEffect() {
	frequency = 40.0;
	duration = 2.0;
	amplitude = 1;
	x = 0;
	y = 0;
	z = 1;
	if (QueryPerformanceFrequency(&currentPerformanceFrequency) != TRUE) {
	    fprintf(stderr, "unable to get performance counter frequency\n");
	    currentPerformanceFrequency.QuadPart = 0;
	}
	debut.QuadPart = 0;
    }

    // Destructor.
    ~InstantBuzzEffect() {}

    // Set duration of effect [seconds].
    void	setDuration(double durInSec) {
	if (durInSec <= 0) {
	    gstErrorHandler(GST_BOUNDS_ERROR,
		"InstantBuzzEffect:setDuration Invalid Duration <= 0",durInSec);
	    return;
	}
	duration = durInSec; 
    }

    // Get duration of effect [seconds].
    double 	getDuration() { return duration; }

    // Set "buzz" frequency [Hz].
    void	setFrequency(double newFreq) {
	if (newFreq <= 0) {
	    gstErrorHandler(GST_BOUNDS_ERROR,
	       "InstantBuzzEffect:setFrequency Invalid Frequency <= 0",newFreq);
	    return;
	}
	frequency = newFreq; 
    }

    // Get "buzz" frequency [Hz].
    double getFrequency() { return frequency; }

    // Set amplitude of effect [millimeters].
    void setAmplitude(double newAmp) {
	if (newAmp < 0) {
	    gstErrorHandler(GST_BOUNDS_ERROR,
		"InstantBuzzEffect:setAmplitude Invalid Amplitude < 0",newAmp);
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
    virtual gstBoolean start() { 
	LARGE_INTEGER counter;
    /*if (!active) { 
	    printf("starting Buzz Effect\n"); 
	}*/
	if (QueryPerformanceCounter(&counter) != TRUE){
	    fprintf(stderr, "unable to get perfo counter\n");
	    return FALSE;
	}
	debut = counter;
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
    virtual gstVector	calcEffectForce(void *phantom);
    virtual gstVector	calcEffectForce(void *phantom, gstVector &torques) {
	torques.init(0.0, 0.0, 0.0);
	return calcEffectForce(phantom);
    }

protected:
    double		frequency, amplitude, duration;
    double		x,y,z;
    LARGE_INTEGER	currentPerformanceFrequency, debut;
};

#endif // VRPN_USE_PHANTOM_SERVER
#endif // INSTANT_BUZZ_EFFECT

