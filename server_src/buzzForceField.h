#ifndef BUZZFORCEFIELD_H
#define BUZZFORCEFIELD_H

/*
	BuzzForceField :
		This is a buzzing effect that may be tied to a particular surface to make
		the surface feel like it is vibrating (buzzing is in direction of
		surface normal and amplitude gets clipped down to zero as you 
		come off the surface. For accurate simulation of a buzzing surface, the
		plane should be set to the plane approximation for the surface and the
		spring constant should match that of the surface if the amplitude is to
		represent the simulated buzzing amplitude.
		Strictly speaking, the simulated vibrating plane is shifted by amplitude
		outwards along normal so force is clipped in the thin layer above
		the surface at heights between 0 and 2*amplitude. (amplitude is typically
		on the order of a millimeter)
 */


#include <gstForceField.h>
#include <gstPlane.h>

class TexturePlane;

class BuzzForceField : public gstForceField {
	// gstForceField override:
	gstVector calculateForceFieldForce(gstPHANToM *phantom);
public:
	BuzzForceField();
	void activate() {_active = TRUE;};
	void deactivate() {_active = FALSE;};
	void restrictToSurface(gstBoolean r) {_surface_only = r;};
	void setSurface(gstPlane &p, double spr) {_plane = p; _spring = spr;};
	void setPlane(gstPlane &p) {_plane = p;};
	void adjustToTexturePlane(TexturePlane *p) {
		_tex_plane = p;
	}
	void setParameters(double buzzamp, double buzzfreq, double kspr){
		_amplitude = buzzamp; _frequency = buzzfreq; _spring = kspr;
	}
	void setSpring(double spr) {_spring = spr;};

	// changes in amplitude or frequency are only done at zero-crossings
	// to avoid 0th-order discontinuity - not done for spring because
	// discontinuity is unavoidable if surface stiffness changes - might
	// want a recovery time though
	void setAmplitude(double amp);
	void setFrequency(double freq);

	int active() {return _active;};
	int restrictedToSurface() {return _surface_only;};
	gstPlane &getPlane() {return _plane;};
	double getSpring() {return _spring;};
	double getAmplitude() {return _amplitude;};
	double getFrequency() {return _frequency;};

private:
	double _t_buzz;	// time used in force calculation
	int _active;		// is force active
	int _surface_only;	// is buzzing limited to the surface of a plane
	gstPlane _plane;		// what surface is buzzing
	double _spring;			// force/mm

	double _amplitude;		// mm
	double _new_amplitude;
	int _amp_needs_update;

	double _frequency;		// Hz
	double _new_frequency;
	int _freq_needs_update;

	CRITICAL_SECTION _amp_freq_mutex;// mutex for accessing 
							// _amp_needs_update or _freq_needs_update

	TexturePlane *_tex_plane;	// we use this to modulate height of _plane by
								// the height of the texture
};

#endif

