/*	texture_plane.h 
	Plane primitive. This is a user defined geometry node which 
	extends the GHOST SDK. 
	This class represents a planar surface
  */

#ifndef TEXTURE_PLANE_H
#define TEXTURE_PLANE_H

#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER

/*
#include <gstPHANToM.h>
#include <gstShape.h>
*/ 

#define MIN_TEXTURE_WAVELENGTH (0.06) // [mm] reasonable value considering
								// resolution of phantom

#include "ghost.h"
#include "buzzForceField.h"

typedef void (*pos_force_dataCB)(const double time, 
    const gstPoint &pos, const gstVector &force, void *ud);


class TexturePlane:public gstShape
{
public:
	void init();
	//Constructor
	TexturePlane(const gstPlane *);

	TexturePlane(const gstPlane &);

	//Constructor
	TexturePlane(const TexturePlane *);

	//Constructor
	TexturePlane(const TexturePlane &);

	//Constructor
	TexturePlane(double a, double b, double c, double d);
	//destructor
	~TexturePlane(){}

	void setParent(gstDynamic *d) {dynamicParent = d;};	// tells plane that its 
			// coordinate system relative to the world coordinates has changed

	// use the DynamicPlane functions instead of these:
	void update(double a, double b, double c, double d);
	void setPlane(gstVector newNormal, gstPoint point);

	static int initClass() {
		//PlaneClassTypeId = gstUniqueId++ ;
		return 0;}

	void setInEffect(gstBoolean effect) {inEffect = effect;}

	void setNumRecCycles(int nrc) {
			if (nrc < 1) 
			   printf("Error: setRecCycles(cycles < 1): %d\n",nrc);
			else
			   numRecoveryCycles = nrc;}

	// Get type of this class.  No instance needed.
	static gstType	getClassTypeId() { return PlaneClassTypeId; }

	virtual gstType getTypeId() const { return PlaneClassTypeId;}

	// GHOST_IGNORE_IN_DOC:
	// Returns TRUE if subclass is of type.
	static gstBoolean	staticIsOfType(gstType type) {
		if (type == PlaneClassTypeId) return TRUE;
		else return (gstShape::staticIsOfType(type));}

    virtual gstBoolean isOftype(gstType type) const {
        if( type == PlaneClassTypeId) return TRUE;
		else return (gstShape::isOfType(type)); } 

	// FOR_GHOST_EXTENSION:
	// Used by system or for creating sub-classes only.
	//  Returns TRUE if line segment defined by startPt_WC and endPt_WC
	//  intersects this shape object.  If so, intersectionPt_WC is set to
	//  point of intersection and intersectionNormal_WC is set to surface
	//  normal at intersection point.
	virtual gstBoolean	intersection(	const gstPoint &startPt_WC,
		const gstPoint &endPt_WC,
		gstPoint &intersectionPt_WC,
		gstVector &intersectionPtNormal_WC,
		void **);

	// Used by system or for creating sub-classes only.
	// Returns TRUE if PHANToM is currently in contact with this object.
	// If so, the collision is added to the PHANToM's list through
	// gstPHANToM::getCollisionInfo() and gstPHANToM::collisionAdded().
	virtual gstBoolean	collisionDetect(gstPHANToM *PHANToM) ;

	// Used by system or for creating sub-classes only.
	// Returns TRUE if pt is inside of this object.
	//virtual int	checkIfPointIsInside_WC(const gstPoint &pt) = 0;

    void printSelf2() const { plane.printSelf2(); }

	// set frict. dyn, frict. stat, spring, texture amplitude
	// and texture wavelength
	void setParameters(double fd, double fs, double ks,
		double bpa, double bpl);

	// texture-related attributes:

	void enableTexture() {usingTexture = 1;};
	void disableTexture() {usingTexture = 0;};
	int textureEnabled() {return usingTexture;};
	// for setting texture by wavelength,amplitude:
	void setTextureWavelength(double wavelength);
	double getTextureWavelength() {return texWL;};
	void updateTextureWavelength();

	void setTextureAmplitude(double amplitude);
	double getTextureAmplitude() {return texAmp;};
	void updateTextureAmplitude();

	void setTextureWaveNumber(double wavenum);
	double getTextureWaveNumber() {return texWN;};

	// for setting texture by size, aspect ratio:
	// size = length of one wavelength
	void setTextureSize(double size);
	double getTextureSize() {return texWL;};
	void updateTextureSize();

	// aspectRatio = 2*amp/wavelength, for normal sine wave this is 1/(PI)
	void setTextureAspectRatio(double aspectRatio);
	double getTextureAspectRatio() {return 2.0*texAmp/texWL;};
	void updateTextureAspectRatio();

	// get a graph of what isotropic surface texture looks like
	void getTextureShape(int nsamples, float *surface);

	double getTextureHeight(gstPoint &position){
		gstPoint tex_pos = position - textureOrigin;
		return computeHeight(tex_pos[0], tex_pos[2]);
	}

	gstVector getTextureNormal(gstPoint &position){
		gstVector tex_pos = position - textureOrigin;
		return computeNormal(tex_pos[0], tex_pos[2]);
	}

	gstPlane getTangentPlane(gstPoint &position){
		gstVector tex_pos = position - textureOrigin;
		return computeTangentPlaneAt(tex_pos);
	}

	int isInContact() {return inContact;};

	// fading force effect
	void fadeAway(double fadingDuration) {
		if (!fadeActive)
			fadeOldKspring = getSurfaceKspring();
		fadeActive = TRUE;
		dSpring_dt = fadeOldKspring/fadingDuration;
	}
	int fadeDone(){
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
	gstBoolean usingAssumedTextureBasePlane(gstPlane &p);
	void updateTextureOrigin(double x, double z);

	// FOR USE IN TEXTURE COORDINATES ONLY!(i.e. phantomPos - textureOrigin):
	gstPlane computeTangentPlaneAt(gstPoint pnt);
	gstPoint projectPointOrthogonalToStaticPlane(gstPoint pnt);
	gstPoint computeSCPfromGradient(gstPoint currentPos);
	double computeHeight(double x, double z);
	double computeHeight(double r);
	gstVector computeNormal(double x, double z);

	// fade-related functions
	void incrementFade(double dT);

protected:
	gstPlane plane;
	gstBoolean inEffect;

	// variables used for recovery
	gstBoolean isNewPlane;	// set to true by update(), set to false
				// 	by collisionDetect()
	gstBoolean isInRecovery;// true after collisionDetect() discovers
				// 	there is a new plane and set to
				//	false when recoveryPlaneCount
				//	reaches numRecoveryCycles
	int recoveryPlaneCount;	// keeps track of how many intermediate
				// 	planes there have been 
	gstPlane originalPlane;	// keeps a copy of the plane set by
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
	gstVector textureOrigin;
	gstDynamic *dynamicParent;
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
#ifdef SGI
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
	static gstType PlaneClassTypeId;
	
};

class BuzzForceField;
// This class implements a haptic plane that will rotate and shift to match
// a specified plane equation allowing special effects to be defined in a
// local plane coordinate system
class DynamicPlane: public gstDynamic {
  public:
	DynamicPlane();
	~DynamicPlane() {};
	static gstType getClassTypeId() {return DynamicPlaneClassTypeId;};
	virtual gstType getTypeId() const {return DynamicPlaneClassTypeId;};
	virtual gstBoolean    isOfType(gstType type) const{    
		if (type == DynamicPlaneClassTypeId) return TRUE;
		else // Check if type is parent class.
			return (gstDynamic::staticIsOfType(type));}
  
	// Allows subclasses to check if they are subclass of type.
	static gstBoolean staticIsOfType(gstType type) {
		if (type == DynamicPlaneClassTypeId) return TRUE;
		else return (gstDynamic::staticIsOfType(type));}
	gstEvent getEvent() {
		gstEvent event;
		event.id = 0;
		return event;
	}

	void update(double a, double b, double c, double d); // set plane equation

	// mostly duplicates of child node functions so we can combine the
	// interfaces of multiple objects 

	void setActive(gstBoolean active);
	void cleanUpAfterError();

	void enableBuzzing(gstBoolean enable);
	void enableTexture(gstBoolean enable);

	int buzzingEnabled() {return _using_buzz;};

	int textureEnabled() {return fixedPlane->textureEnabled();};

	void setParameters(double fd, double fs, double ks,
								 double bpa, double bpl, 
								 double buzzamp, double buzzfreq);

    double getSurfaceFstatic() {
		return fixedPlane->getSurfaceFstatic();}

	void setSurfaceFstatic(double fs) {
		fixedPlane->setSurfaceFstatic(fs);}

	double getSurfaceFdynamic() {
		return fixedPlane->getSurfaceFdynamic();}

    void setSurfaceFdynamic(double fd) {
		fixedPlane->setSurfaceFdynamic(fd);}

	double getSurfaceKspring() {
		return fixedPlane->getSurfaceKspring();}

	void setSurfaceKspring(double ks);

	void setSurfaceKdamping(double kd) {
		fixedPlane->setSurfaceKdamping(kd);}
	double getSurfaceKdamping() {
		return fixedPlane->getSurfaceKdamping();}

	double getBuzzAmplitude();

    void setBuzzAmplitude(double amp);

	double getBuzzFrequency();

	void setBuzzFrequency(double freq);

	double getTextureAspectRatio() {
		return fixedPlane->getTextureAspectRatio();}

    void setTextureAspectRatio(double ar) {
		fixedPlane->setTextureAspectRatio(ar);}

	void setTextureSize(double size){
		fixedPlane->setTextureSize(size);}
	double getTextureSize() {return fixedPlane->getTextureSize();};

	void setTextureAmplitude(double amp){
		fixedPlane->setTextureAmplitude(amp);}
	double getTextureAmplitude() {
		return fixedPlane->getTextureAmplitude();};

	double getTextureWavelength() {
		return fixedPlane->getTextureWavelength();}

    void setTextureWavelength(double wl) {
		fixedPlane->setTextureWavelength(wl);}

	double getTextureWaveNumber() {
		return fixedPlane->getTextureWaveNumber();}

	void setTextureWaveNumber(double wn) {
		fixedPlane->setTextureWaveNumber(wn);}

    void getTextureShape(int nsamples, float *surface) {
        fixedPlane->getTextureShape(nsamples, surface);}

	// TODO: setTextureShape()

    void fadeAway(double fadingDuration) 
		{ fixedPlane->fadeAway(fadingDuration);}
    int fadeDone(){ return fixedPlane->fadeDone();}
    void cancelFade() {fixedPlane->cancelFade();}

    void setDataCB(pos_force_dataCB func, void *ud){
		fixedPlane->setDataCB(func, ud);
    }
    void resetDataTime() {fixedPlane->resetDataTime();};

	// DynamicPlane-specific:
	void planeEquationToTransform(gstPlane &prev_plane,
		gstPlane &next_plane, gstTransformMatrix &xform_to_update);
	void setPlaneFromTransform(gstPlane &pl, gstTransformMatrix &xfm);

	void planeEquationToTransform(const gstPlane &prev_plane, 
		const gstPlane &next_plane, const gstVector &p_project, 
		gstTransformMatrix &xform);

  private:
	void updateTransform(const gstTransformMatrix &xfm) {
		xform = xfm;};
	gstTransformMatrix getTransform() {
		return xform;};

  gstInternal public:
	static int initClass();
	virtual void updateDynamics();

  private:
	gstTransformMatrix xform;
// MB: for SGI compilation with pthreads
#ifdef SGI
        pthread_mutex_t  xform_mutex;
#else
	CRITICAL_SECTION xform_mutex;
#endif

	static gstType DynamicPlaneClassTypeId;
	TexturePlane *fixedPlane;
	BuzzForceField *buzzForce;
	gstPlane plane;
	gstPlane lastPlane;

	int _using_buzz;
	int _active;
	int _is_new_plane;
};

#endif
#endif
