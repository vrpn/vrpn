/*	texture_plane.h 
	Plane primitive. This is a user defined geometry node which 
	extends the GHOST SDK. 
	This class represents a planar surface which can have bumps, and
	a dynamic plane with bumps that can jump up and down.

	XXX The port to HDAPI is not complete for these classes.  There is
	something in the dynamics callback that is not working (probably),
	and the Matrix class defined in ghost.h for the HDAPI implementation
	also may not work.
  */

#ifndef TEXTURE_PLANE_H
#define TEXTURE_PLANE_H

#include  "vrpn_Configure.h"
#include  "vrpn_ForceDevice.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

#ifndef	TRUE
#define TRUE 1
#endif
#ifndef	FALSE
#define FALSE 0
#endif

#define MIN_TEXTURE_WAVELENGTH (0.06) // [mm] reasonable value considering
				      // resolution of phantom

#include "ghost.h"
#include "buzzForceField.h"

typedef void (*pos_force_dataCB)(const double time, 
    const vrpn_HapticPosition &pos, const vrpn_HapticVector &force, void *ud);


class TexturePlane:public vrpn_HapticSurface
{
public:
	void init();

	//Constructor
	TexturePlane(const vrpn_HapticPlane *);
	TexturePlane(const vrpn_HapticPlane &);

	//Constructor
	TexturePlane(const TexturePlane *);
	TexturePlane(const TexturePlane &);

	//Constructor
	TexturePlane(double a, double b, double c, double d);
	//destructor
	~TexturePlane(){}

	// use the DynamicPlane functions instead of these:
	void update(double a, double b, double c, double d);
	void setPlane(vrpn_HapticVector newNormal, vrpn_HapticPosition point);

	static int initClass() {
		//PlaneClassTypeId = gstUniqueId++ ;
		return 0;}

	void setNumRecCycles(int nrc) {
          if (nrc < 1) {
            numRecoveryCycles = 1;
          } else {
            numRecoveryCycles = nrc;
          }
        }

	void setInEffect(vrpn_HapticBoolean effect) {inEffect = effect;}

	// FOR_GHOST_EXTENSION:
	// Used by system or for creating sub-classes only.
	//  Returns true if line segment defined by startPt_WC and endPt_WC
	//  intersects this shape object.  If so, intersectionPt_WC is set to
	//  point of intersection and intersectionNormal_WC is set to surface
	//  normal at intersection point.
	virtual vrpn_HapticBoolean	intersection(const vrpn_HapticPosition &startPt_WC,
		const vrpn_HapticPosition &endPt_WC,
		vrpn_HapticPosition &intersectionPt_WC,
		vrpn_HapticVector &intersectionPtNormal_WC,
		void **);

	// Used by system or for creating sub-classes only.
	// Returns true if pt is inside of this object.
	//virtual int	checkIfPointIsInside_WC(const vrpn_HapticPosition &pt) = 0;

#ifndef	VRPN_USE_HDAPI
	// Used by system or for creating sub-classes only.
	// Returns TRUE if PHANToM is currently in contact with this object.
	// If so, the collision is added to the PHANToM's list through
	// gstPHANToM::getCollisionInfo() and gstPHANToM::collisionAdded().
	// XXX We may need this.
	virtual vrpn_HapticBoolean	collisionDetect(gstPHANToM *PHANToM) ;

	void setParent(gstDynamic *d) {dynamicParent = d;};	// tells plane that its 
			// coordinate system relative to the world coordinates has changed

	// Get type of this class.  No instance needed.
	static gstType	getClassTypeId() { return PlaneClassTypeId; }

	virtual gstType getTypeId() const { return PlaneClassTypeId;}

	// GHOST_IGNORE_IN_DOC:
	// Returns TRUE if subclass is of type.
	static vrpn_HapticBoolean	staticIsOfType(gstType type) {
		if (type == PlaneClassTypeId) return TRUE;
		else return (gstShape::staticIsOfType(type));}

	virtual vrpn_HapticBoolean isOftype(gstType type) const {
	    if( type == PlaneClassTypeId) return TRUE;
		    else return (gstShape::isOfType(type)); } 

	void printSelf2() const { plane.printSelf2(); }
#endif
	// set frict. dyn, frict. stat, spring, texture amplitude
	// and texture wavelength
	void setParameters(double fd, double fs, double ks,
		double bpa, double bpl);

	// texture-related attributes:

	void enableTexture() {usingTexture = 1;};
	void disableTexture() {usingTexture = 0;};
	int textureEnabled() const {return usingTexture;};
	// for setting texture by wavelength,amplitude:
	void setTextureWavelength(double wavelength);
	double getTextureWavelength() const {return texWL;};
	void updateTextureWavelength();

	void setTextureAmplitude(double amplitude);
	double getTextureAmplitude() const {return texAmp;};
	void updateTextureAmplitude();

	void setTextureWaveNumber(double wavenum);
	double getTextureWaveNumber() const {return texWN;};

	// for setting texture by size, aspect ratio:
	// size = length of one wavelength
	void setTextureSize(double size);
	double getTextureSize() const {return texWL;};
	void updateTextureSize();

	// aspectRatio = 2*amp/wavelength, for normal sine wave this is 1/(PI)
	void setTextureAspectRatio(double aspectRatio);
	double getTextureAspectRatio() const {return 2.0*texAmp/texWL;};
	void updateTextureAspectRatio();

	// get a graph of what isotropic surface texture looks like
	void getTextureShape(int nsamples, float *surface) const;

	double getTextureHeight(vrpn_HapticPosition &position) const {
		vrpn_HapticVector tex_pos = position - textureOrigin;
		return computeHeight(tex_pos[0], tex_pos[2]);
	}

	vrpn_HapticVector getTextureNormal(vrpn_HapticPosition &position) const {
		vrpn_HapticVector tex_pos = position - textureOrigin;
		return computeNormal(tex_pos[0], tex_pos[2]);
	}

	vrpn_HapticPlane  getTangentPlane(vrpn_HapticPosition &position) const {
		vrpn_HapticVector tex_pos = position - textureOrigin;
		return computeTangentPlaneAt(tex_pos);
	}

	int isInContact() const {return inContact;};

	// fading force effect
	void fadeAway(double fadingDuration) {
	  if (!fadeActive) {
	    fadeOldKspring = getSurfaceKspring();
	  }
	  fadeActive = TRUE;
	  dSpring_dt = fadeOldKspring/fadingDuration;
	}
	int fadeDone() const {
		return !fadeActive;
	}
	void cancelFade() {
		if (!fadeActive) return;
		inEffect = FALSE;
		setSurfaceKspring(fadeOldKspring);
		fadeActive = FALSE;
	}

	// for data collection - data callback should not do anything other
	// than store the data to an external buffer, set to NULL to 
	// stop data collection:
	void setDataCB(pos_force_dataCB func, void *ud){
		dataCB = func; userdata = ud; data_time = 0;
	}
	void resetDataTime() {data_time = 0;};

	void signalNewTransform() {newCoordinates = TRUE;};

private:
	// texture-related functions
	vrpn_HapticBoolean usingAssumedTextureBasePlane(vrpn_HapticPlane &p);
	void updateTextureOrigin(double x, double z);

	// FOR USE IN TEXTURE COORDINATES ONLY!(i.e. phantomPos - textureOrigin):
	double computeHeight(double x, double z) const;
	double computeHeight(double r) const;
	vrpn_HapticPlane computeTangentPlaneAt(vrpn_HapticPosition pnt) const;
	vrpn_HapticPosition projectPointOrthogonalToStaticPlane(vrpn_HapticPosition pnt) const;
	vrpn_HapticPosition computeSCPfromGradient(vrpn_HapticPosition currentPos) const;
	vrpn_HapticVector computeNormal(double x, double z) const;

	// fade-related functions
	void incrementFade(double dT);

protected:
	vrpn_HapticPlane    plane;
	vrpn_HapticBoolean  inEffect;

	// variables used for recovery
	vrpn_HapticBoolean isNewPlane;	// set to true by update(), set to false
				// 	by collisionDetect()
	vrpn_HapticBoolean isInRecovery;// true after collisionDetect() discovers
				// 	there is a new plane and set to
				//	false when recoveryPlaneCount
				//	reaches numRecoveryCycles
	int recoveryPlaneCount;	// keeps track of how many intermediate
				// 	planes there have been 
	vrpn_HapticPlane originalPlane;	// keeps a copy of the plane set by
				// 	update() so that we can restore this
				//	plane precisely when recovery is complete
	double lastDepth;	// this is the depth below the surface
				// 	for the plane previous to the
				//	new plane - updated in collisionDetect()
	double dIncrement;	// value added to d parameter of plane
				//	equation with each call to
				//	collisionDetect() during a recovery
				//	- this value is computed when a new
				//	plane is encountered
	int numRecoveryCycles;	// number of recovery cycles

	// end of variables for recovery

	// variables for transforming into texture coordinates
	// problem this addresses: provide a continuous 2D->2D map from a texture plane onto
	// the haptic plane where the map is a continuous function of the phantom
	// position which minimizes local distortion. With such a map we can add 
	// texture to a curved surface so it feels almost like the texture on a 
	// flat plane as long as the texture scale is much smaller
	// than the surface scale.

	// the parent node transforms the plane (0,1,0,0) into
	// the current plane by incremental rotations about edges formed by
	// consecutive planes - in this way we get a mapping that takes care of
	// preserving orientation in the plane with each new plane update
	// However, we are still left with a translation to maintain continuity
	// between texture coordinates on one plane and the next and this is
	// handled by updating a textureOrigin. textureOrigin is updated so that
	// the projection of the phantom position onto the plane texture coordinates
	// is preserved over plane updates
	vrpn_HapticVector textureOrigin;
#ifndef	VRPN_USE_HDAPI
	gstDynamic *dynamicParent;
#endif
	// end of texture variables for xform stuff
	// other texture variables
	int usingTexture;

	double new_texWN;
	int texWN_needs_update;		// update texWN/texWL preserving texAmp
	double texWN;	// wave number (waves per mm)
	double texWL;	// wave length	- inverse of wave number

	double new_Size;		// this is really the new value for texWL but
	int texSize_needs_update;// it is updated in such a way as to preserve
							// textureAspect
	double new_texAmp;
	int texAmp_needs_update;
	double texAmp;	// amplitude

	double new_textureAspect;
	int texAspect_needs_update;	// when this is updated it preserves wavelength
	double textureAspect; // ratio of amplitude to wave length

// MB: for SGI compilation with pthreads
#if defined(SGI) || defined (__CYGWIN__) || defined(linux)
        pthread_mutex_t  tex_param_mutex;
#else
	CRITICAL_SECTION tex_param_mutex;
#endif

	// variables for fading surface away (gradually reducing spring const)
	int fadeActive;
	double fadeOldKspring;
	double dSpring_dt;

	// variables for data callback
	pos_force_dataCB dataCB;
	void *userdata;
	double data_time;

	// other
	int inContact;
	int safety_ineffect;
	int newCoordinates;
	double t_elapsed;

private:
#ifndef	VRPN_USE_HDAPI
	static gstType PlaneClassTypeId;
#endif
};

class BuzzForceField;
// This class implements a haptic plane that will rotate and shift to match
// a specified plane equation allowing special effects to be defined in a
// local plane coordinate system
#ifdef	VRPN_USE_HDAPI
class DynamicPlane {
#else
class DynamicPlane: public gstDynamic {
#endif
  public:
	DynamicPlane();
	~DynamicPlane() {};
#ifndef	VRPN_USE_HDAPI
	static gstType              getClassTypeId() {return DynamicPlaneClassTypeId;};
	virtual gstType             getTypeId() const {return DynamicPlaneClassTypeId;};
	virtual vrpn_HapticBoolean  isOfType(gstType type) const {    
          if (type == DynamicPlaneClassTypeId) {
            return TRUE;
          } else {// Check if type is parent class.
            return (gstDynamic::staticIsOfType(type));
          }
        }
  
	// Allows subclasses to check if they are subclass of type.
	static vrpn_HapticBoolean staticIsOfType(gstType type) {
		if (type == DynamicPlaneClassTypeId) return TRUE;
		else return (gstDynamic::staticIsOfType(type));}
	gstEvent getEvent() const {
		gstEvent event;
		event.id = 0;
		return event;
	}
#endif

	void update(double a, double b, double c, double d); // set plane equation

	// mostly duplicates of child node functions so we can combine the
	// interfaces of multiple objects 

	void setActive(vrpn_HapticBoolean active);
	void cleanUpAfterError();

	void enableBuzzing(vrpn_HapticBoolean enable);
	void enableTexture(vrpn_HapticBoolean enable);

	int buzzingEnabled() const {return _using_buzz;};

	int textureEnabled() const {return fixedPlane->textureEnabled();};

	void setParameters(double fd, double fs, double ks,
			   double bpa, double bpl, 
			   double buzzamp, double buzzfreq);

	double getSurfaceFstatic() const {
		    return fixedPlane->getSurfaceFstatic();}

	void setSurfaceFstatic(double fs) {
		fixedPlane->setSurfaceFstatic(fs);}

	double getSurfaceFdynamic() const {
		return fixedPlane->getSurfaceFdynamic();}

	void setSurfaceFdynamic(double fd) {
		    fixedPlane->setSurfaceFdynamic(fd);}

	double getSurfaceKspring() const {
		return fixedPlane->getSurfaceKspring();}

	void setSurfaceKspring(double ks);

	void setSurfaceKdamping(double kd) {
		fixedPlane->setSurfaceKdamping(kd);}
	double getSurfaceKdamping() const {
		return fixedPlane->getSurfaceKdamping();}

	double getBuzzAmplitude() const { return buzzForce->getAmplitude(); };

        void setBuzzAmplitude(double amp);

        double getBuzzFrequency() const { return buzzForce->getFrequency(); };

	void setBuzzFrequency(double freq);

	double getTextureAspectRatio() const {
		return fixedPlane->getTextureAspectRatio();}

	void setTextureAspectRatio(double ar) {
		fixedPlane->setTextureAspectRatio(ar);}

	void setTextureSize(double size){
		fixedPlane->setTextureSize(size);}
	double getTextureSize() const {return fixedPlane->getTextureSize();};

	void setTextureAmplitude(double amp){
		fixedPlane->setTextureAmplitude(amp);}
	double getTextureAmplitude() const {
		return fixedPlane->getTextureAmplitude();};

	double getTextureWavelength() const {
		return fixedPlane->getTextureWavelength();}

	void setTextureWavelength(double wl) {
		fixedPlane->setTextureWavelength(wl);}

	double getTextureWaveNumber() const {
		return fixedPlane->getTextureWaveNumber();}

	void setTextureWaveNumber(double wn) {
		fixedPlane->setTextureWaveNumber(wn);}

	void getTextureShape(int nsamples, float *surface) const {
		fixedPlane->getTextureShape(nsamples, surface);}

	bool getActive(void) const { return (_active != 0); }

	// TODO: setTextureShape()

	void fadeAway(double fadingDuration) 
		    { fixedPlane->fadeAway(fadingDuration);}
	int fadeDone() const { return fixedPlane->fadeDone();}
	void cancelFade() {fixedPlane->cancelFade();}

	void setDataCB(pos_force_dataCB func, void *ud){
		    fixedPlane->setDataCB(func, ud);
	}
	void resetDataTime() {fixedPlane->resetDataTime();};

	// DynamicPlane-specific:
	void planeEquationToTransform(vrpn_HapticPlane &prev_plane,
		vrpn_HapticPlane &next_plane, vrpn_HapticMatrix &xform_to_update);
	void planeEquationToTransform(const vrpn_HapticPlane &prev_plane, 
		const vrpn_HapticPlane &next_plane, const vrpn_HapticVector &p_project, 
		vrpn_HapticMatrix &xform);
	void setPlaneFromTransform(vrpn_HapticPlane &pl, vrpn_HapticMatrix &xfm);


#ifdef	VRPN_USE_HDAPI
	const vrpn_HapticPlane &getPlane(void) const { return plane; }
#endif

  private:
	void updateTransform(const vrpn_HapticMatrix &xfm) {
		xform = xfm;};
	vrpn_HapticMatrix getTransform() const {
		return xform;};

#ifdef	VRPN_USE_HDAPI
  public:
	virtual void updateDynamics();
#else
  gstInternal public:
	static int initClass();
	virtual void updateDynamics();
#endif

  private:
	vrpn_HapticMatrix xform;
// MB: for SGI compilation with pthreads
#if defined(SGI) || defined (__CYGWIN__) || defined(linux)
        pthread_mutex_t  xform_mutex;
#else
	CRITICAL_SECTION xform_mutex;
#endif

#ifndef	VRPN_USE_HDAPI
	static gstType DynamicPlaneClassTypeId;
#endif
	TexturePlane *fixedPlane;
	BuzzForceField *buzzForce;
	vrpn_HapticPlane plane;
	vrpn_HapticPlane lastPlane;

	bool _using_buzz;
	bool _active;
	bool _is_new_plane;
};

#endif
#endif
