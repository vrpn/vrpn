#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#include "buzzForceField.h"
#include "texture_plane.h"

// So we don't have #defines throughout the code that we forget to fix.
#if defined(sgi) || defined (__CYGWIN__) || defined(linux)
#define init_mutex(x) pthread_mutex_init(x,NULL);
#define get_mutex(x)  pthread_mutex_lock(x)
#define release_mutex(x) pthread_mutex_unlock(x);
#else
#define init_mutex(x) InitializeCriticalSection(x);
#define get_mutex(x)  EnterCriticalSection(x);
#define release_mutex(x) LeaveCriticalSection(x);
#endif

// macros for printing something out x times out of a thousand (i.e. x times per second)
#define DEBUG_BEGIN(x)	{static int pcnt = 0; if ((pcnt % 1000 >= 0)&&(pcnt%1000 < x)){
#define DEBUG_END	}pcnt++;}

// APPLICATION:
#ifdef	VRPN_USE_HDAPI
BuzzForceField::BuzzForceField() : 
#else
BuzzForceField::BuzzForceField() : gstForceField(),
#endif
	_t_buzz(0.0),
	_active(0),
	_surface_only(0),
	_plane(vrpn_HapticPlane(0.0, 1.0, 0.0, 0.0)),
	_spring(0.5),
	_amplitude(0.5),
	_frequency(60.0),
	_amp_needs_update(0),
	_freq_needs_update(0)
{
#ifndef	VRPN_USE_HDAPI
  boundBySphere(vrpn_HapticPosition(0,0,0), 1000);
#endif
	_tex_plane = NULL;
// MB: for SGI compilation with pthreads
	init_mutex(&_amp_freq_mutex);
}

void BuzzForceField::setAmplitude(double amp) {
	if (!_active){
		_amplitude = amp;
		return;
	}
	_new_amplitude = amp;
//	get_mutex(&_amp_freq_mutex);
	_amp_needs_update = true;
//	release_mutex(&_amp_freq_mutex);
}

void BuzzForceField::setFrequency(double freq) {
	if (!_active){
		_frequency = freq;
		return;
	}
	_new_frequency = freq;
//	get_mutex(&_amp_freq_mutex);
	_freq_needs_update = true;
//	release_mutex(&_amp_freq_mutex);
}

// SERVOLOOP:
#ifdef	VRPN_USE_HDAPI
vrpn_HapticVector BuzzForceField::calculateForceFieldForce(HDAPI_state	*state)
#else
vrpn_HapticVector BuzzForceField::calculateForceFieldForce(gstPHANToM *phantom)
#endif
{
	vrpn_HapticPosition phanPos;
	double force_mag;
	vrpn_HapticVector force_vec;
	double height_mod = 0;
	double phase;

	if (_active){
#ifdef	VRPN_USE_HDAPI
		int rate = state->instant_rate;
		double deltaT = 1.0 / rate;
		double vec[3];
		vec[0] = state->pose[3][0]; vec[1] = state->pose[3][1]; vec[2] = state->pose[3][2];
		//XXX We're not dealing with transforms here... make sure VRPN doesn't set them, or else deal with them!
		phanPos = vrpn_HapticPosition(vec);
#else
		double deltaT = phantom->getDeltaT();
		phantom->getPosition_WC(phanPos);
		phanPos = fromWorld(phanPos);
#endif
		_t_buzz += deltaT;
		phase = 2.0*_t_buzz*_frequency;
		if ( (phase - floor(phase)) < ((deltaT)*2.0*_frequency) ){
			// its okay to update freq, amp now 
//			get_mutex(&_amp_freq_mutex);
			if (_amp_needs_update){
				_amp_needs_update = false;
				_amplitude = _new_amplitude;
			}
			if (_freq_needs_update){
				_freq_needs_update = false;
				if (_new_frequency != 0)
					_t_buzz *= _frequency/_new_frequency;	// for continuity
				_frequency = _new_frequency;
			}
//			release_mutex(&_amp_freq_mutex);
		}
		// compute height above plane
		if (_tex_plane) {
			height_mod = _tex_plane->getTextureHeight(phanPos);
		}
#ifdef	VRPN_USE_HDAPI
		double ph_height = _plane.perpDistance(phanPos)-height_mod;
#else
		double ph_height = _plane.error(phanPos)-height_mod;
#endif
		if (ph_height > _amplitude) {
			return vrpn_HapticVector(0,0,0);
		}
		// force may be nonzero so compute height of vibrating plane to see
		double buzzplane_height = _amplitude*(sin(2.0*VRPN_PI*_frequency*_t_buzz));

		if (ph_height > 0) { // above static plane but not necess. above vibrating plane
			if (ph_height > buzzplane_height) // so force will be in +normal direction
				force_mag = 0;
			else
				force_mag = (buzzplane_height-ph_height)*_spring;
		}
		else if (ph_height > buzzplane_height){
			force_mag = (ph_height)*_spring;  // we need to cancel force of static plane so we
							  // produce the opposite of the collision force here
		}
		else {
			force_mag = buzzplane_height*_spring;
		}

		force_vec = _plane.normal();
		force_vec.normalize();
		force_vec *= force_mag;

		return force_vec;
	} else {
		return vrpn_HapticVector(0,0,0);
	}
}

#endif
