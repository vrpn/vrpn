#include "buzzForceField.h"
#include "texture_plane.h"

// macros for printing something out x times out of a thousand (i.e. x times per second)
#define DEBUG_BEGIN(x)	{static int pcnt = 0; if ((pcnt % 1000 >= 0)&&(pcnt%1000 < x)){
#define DEBUG_END	}pcnt++;}

// APPLICATION:
BuzzForceField::BuzzForceField() : gstForceField(),
	_t_buzz(0.0),
	_active(0),
	_surface_only(0),
	_plane(gstPlane(0.0, 1.0, 0.0, 0.0)),
	_spring(0.5),
	_amplitude(0.5),
	_frequency(60.0),
	_amp_needs_update(0),
	_freq_needs_update(0)
{
	boundBySphere(gstPoint(0,0,0), 1000);
	_tex_plane = NULL;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_init(&_amp_freq_mutex,NULL);
#else
	InitializeCriticalSection(&_amp_freq_mutex);
#endif
}

void BuzzForceField::setAmplitude(double amp) {
	if (!_active){
		_amplitude = amp;
		return;
	}
	_new_amplitude = amp;
//	EnterCriticalSection(&_amp_freq_mutex);
	_amp_needs_update = TRUE;
//	LeaveCriticalSection(&_amp_freq_mutex);
}

void BuzzForceField::setFrequency(double freq) {
	if (!_active){
		_frequency = freq;
		return;
	}
	_new_frequency = freq;
//	EnterCriticalSection(&_amp_freq_mutex);
	_freq_needs_update = TRUE;
//	LeaveCriticalSection(&_amp_freq_mutex);
}

// SERVOLOOP:
gstVector BuzzForceField::calculateForceFieldForce(gstPHANToM *phantom){
	gstPoint phanPos;
	double force_mag;
	gstVector force_vec;
	double height_mod = 0;
	double phase;

	if (_active){
		_t_buzz += phantom->getDeltaT();
		phase = 2.0*_t_buzz*_frequency;
		if ( (phase - floor(phase)) < ((phantom->getDeltaT())*2.0*_frequency) ){
			// its okay to update freq, amp now 
//			EnterCriticalSection(&_amp_freq_mutex);
			if (_amp_needs_update){
				_amp_needs_update = FALSE;
				_amplitude = _new_amplitude;
			}
			if (_freq_needs_update){
				_freq_needs_update = FALSE;
				if (_new_frequency != 0)
					_t_buzz *= _frequency/_new_frequency;	// for continuity
				_frequency = _new_frequency;
			}
//			LeaveCriticalSection(&_amp_freq_mutex);
		}
		phantom->getPosition_WC(phanPos);
		phanPos = fromWorld(phanPos);
		// compute height above plane
		if (_tex_plane)
			height_mod = _tex_plane->getTextureHeight(phanPos);
		double ph_height = _plane.error(phanPos)-height_mod;
		
		if (ph_height > _amplitude) {
			return gstVector(0,0,0);
		}
		// force may be nonzero so compute height of vibrating plane to see
		double buzzplane_height = _amplitude*(sin(2.0*M_PI*_frequency*_t_buzz));

		if (ph_height > 0) { // above static plane but not necess. above vibrating plane
			if (ph_height > buzzplane_height) // so force will be in +normal direction
				force_mag = 0;
			else
				force_mag = (buzzplane_height-ph_height)*_spring;
		}
		else if (ph_height > buzzplane_height){
			force_mag = (ph_height)*_spring; // we need to cancel force of static plane so we
											// produce the opposite of the collision force here
		}
		else {
			force_mag = buzzplane_height*_spring;
		}


		force_vec = _plane.normal();
		force_vec.normalize();
		force_vec *= force_mag;

		return force_vec;
	}
	else
		return gstVector(0,0,0);
}
