#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
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
//#define VERBOSE

#define MAX_FORCE (10.0)

// Initialize class.
#ifndef	VRPN_USE_HDAPI
gstType DynamicPlane::DynamicPlaneClassTypeId;
int DynamicPlaneTest = DynamicPlane::initClass();
int DynamicPlane::initClass() {
    DynamicPlaneClassTypeId = -1000;
    return 0;
}
#endif


#ifdef	VRPN_USE_HDAPI
DynamicPlane::DynamicPlane()
#else
DynamicPlane::DynamicPlane() : gstDynamic()
#endif
{
	// initialize transform to identity
	fixedPlane = new TexturePlane(0.0, 1.0, 0.0, 0.0);
#ifdef	VRPN_USE_HDAPI
	//XXX Probably going to need some linking code in here somewhere to keep track of objects
#else
	xform.identity();
	fixedPlane->setParent(this);
#endif
	buzzForce = new BuzzForceField();
	buzzForce->setPlane(vrpn_HapticPlane(0.0, 1.0, 0.0, 0.0));
	buzzForce->setSpring(fixedPlane->getSurfaceKspring());
	buzzForce->restrictToSurface(TRUE);
	buzzForce->adjustToTexturePlane(fixedPlane);

	plane = vrpn_HapticPlane(0.0, 1.0, 0.0, 0.0);
	lastPlane = vrpn_HapticPlane(plane);
#ifdef	VRPN_USE_HDAPI
	//XXX Probably going to need some linking code in here somewhere to keep track of objects
#else
	addChild(fixedPlane);
	addChild(buzzForce);
#endif
        _using_buzz = FALSE;
	_active = FALSE;
	_is_new_plane = FALSE;
	buzzForce->deactivate();
	fixedPlane->enableTexture();
	fixedPlane->setTextureSize(10.0);
	fixedPlane->setTextureAspectRatio(0.0);
	fixedPlane->setInEffect(FALSE);
	init_mutex(&xform_mutex);
}

void DynamicPlane::setParameters(double fd, double fs, double ks,
				 double bpa, double bpl, 
				 double buzzamp, double buzzfreq)
{
	fixedPlane->setParameters(fd, fs, ks, bpa, bpl);
	buzzForce->setParameters(buzzamp, buzzfreq, ks);
}


void DynamicPlane::setActive(vrpn_HapticBoolean active)
{
    _active = (active == TRUE);
    fixedPlane->setInEffect(active);
    if (_active && _using_buzz) {
      buzzForce->activate();
    } else {
      buzzForce->deactivate();
    }
}

void DynamicPlane::cleanUpAfterError() {
	_is_new_plane = FALSE;
	setActive(FALSE);
}

void DynamicPlane::enableBuzzing(vrpn_HapticBoolean enable)
{
    _using_buzz = (enable == TRUE);
    if (_active && _using_buzz) buzzForce->activate();
    else buzzForce->deactivate();
}
void DynamicPlane::enableTexture(vrpn_HapticBoolean enable)
{
    if (enable) fixedPlane->enableTexture();
    else fixedPlane->disableTexture();
}

void DynamicPlane::setSurfaceKspring(double ks)
{
    buzzForce->setSpring(ks);
    fixedPlane->setSurfaceKspring(ks);
}

void DynamicPlane::setBuzzAmplitude(double amp)
{
    buzzForce->setAmplitude(amp);
}

void DynamicPlane::setBuzzFrequency(double freq) {
    buzzForce->setFrequency(freq);}

// notes for stuff in updateDynamics() and update():
        // tell surface to add a translation which
        // is a function of the next phantom position
        // read in collisionDetect since this position
        // is the pivot (point for which old plane
        // was in effect before and new plane is in
        // effect after). Plane is translated in its
        // plane so that the pivot point projects to
        // the same place on the old and new planes.

        // alternative description:
        // order of events:
        // 0) collisionDetect:
        //          force from old plane specified and SCP_old computed
        // 1) plane message received, xform computed without translation
        // 2) phantom moves to position X from forces of old plane
        // 3) collisionDetect:
        //      force from new plane specified with condition
        //      SCP_old = SCP_new in plane coordinate system
        //      requiring a shift of the new plane
        //      (only at this stage is X known to plane so it couldn't have
        //       added this translation into xform previously)
        //
        // projection of position X is treated as the pivot point
        // so we translate the new plane so that the projection lies at
        // the same point in the plane as the projection did in the old
        // plane
        //
        // this way, if you were on a peak of a texture bump before the plane
        // changed, then you will still be on the peak but with a different
        // surface normal; the discontinuity is the same as that produced
        // by a non-textured surface and this is what we would expect for
        // a textured surface that was bent around a sharp edge
        // if we didn't do this, then you would get all kinds of strange
        // things happening at edges because all of a sudden you would be
        // at a different place on the texture
        //
        //

void DynamicPlane::updateDynamics() {
	get_mutex(&xform_mutex);
	if (_is_new_plane) {
		planeEquationToTransform(lastPlane, plane, xform);
		// if we don't compute plane from xform then any
		// roundoff error in xform will be allowed to accumulate without
		// correction; by computing the actual plane represented by xform
		// it should cause updates to tend to reduce roundoff error to
		// zero if plane stays constant for a while or at least a small 
		// bounded amount if plane is always changing - this has been verified
		// in a test program
		setPlaneFromTransform(plane, xform);
		release_mutex(&xform_mutex);
#ifdef	VRPN_USE_HDAPI
		//XXX Probably going to need some transform code in here somewhere to keep track of motion
                //XXXX This xform definitely changes when new planes are sent!
#else
		setTransformMatrixDynamic(xform);
#endif
		_is_new_plane = FALSE;
        } else {
	        release_mutex(&xform_mutex);
        }

#ifdef	VRPN_USE_HDAPI
	//XXX Probably going to need some code in here somewhere to emulate GHOST behavior
#else
	gstDynamic::updateDynamics();
#endif
}

void DynamicPlane::update(double a,double b,double c,double d)
{
	//static int err_cnt = 0;
	
	// no need to do anything unless this plane is different from
	// the previous one
	get_mutex(&xform_mutex);
	if (plane.a() != a || plane.b() != b ||
		plane.c() != c || plane.d() != d){
		if (_is_new_plane) {
		//	err_cnt++;
		//	if (err_cnt > 3)
		//		printf("waited %d msec\n", err_cnt);
	                release_mutex(&xform_mutex);
			return;
		}
//		else err_cnt = 0;
		lastPlane = vrpn_HapticPlane(plane);
		fixedPlane->signalNewTransform();
		plane = vrpn_HapticPlane(a,b,c,d);
		_is_new_plane = TRUE;
	release_mutex(&xform_mutex);
#ifndef	VRPN_USE_HDAPI
		// XXX At some point, we may implement HDAPI dynamics objects.
		addToDynamicList();
#endif
	}
	else
	get_mutex(&xform_mutex);
}


// planeEquationToTransform:
// Given a plane that passes through the origin (0,0,0), and a transformation
// xform that transforms that plane to prev_plane, this function modifies
// xform so that it transforms the plane to next_plane. The incremental
// transformation composed with xform to achieve this consists of a rotation
// about the axis that is the intersection of prev_plane with next_plane.

void DynamicPlane::planeEquationToTransform(vrpn_HapticPlane &prev_plane,
    vrpn_HapticPlane &next_plane, vrpn_HapticMatrix &xform_to_update) 
{
    // plane is [a,b,c,d] where ax + by +cz + d = 0

    vrpn_HapticVector next_normal = next_plane.normal();
    vrpn_HapticVector prev_normal = prev_plane.normal();
#ifdef	VRPN_USE_HDAPI
    double m = next_normal.magnitude();
#else
    double m = next_normal.distToOrigin();
#endif

    double distAlongNormal = -next_plane.d();

    vrpn_HapticVector axis = vrpn_HapticVector(0,0,0);
    double theta = 0;
    vrpn_HapticVector trans;

    trans = distAlongNormal*next_normal; // translation vector for xform

    // compute the rotation axis and angle
#ifdef	VRPN_USE_HDAPI
    vrpn_HapticVector crossprod = prev_normal.crossProduct(next_normal);
    double sintheta = crossprod.magnitude();
    double costheta = prev_normal.dotProduct(next_normal);
#else
    vrpn_HapticVector crossprod = prev_normal.cross(next_normal);
    double sintheta = crossprod.distToOrigin();
    double costheta = prev_normal.dot(next_normal);
#endif
    if (sintheta == 0) {	// exceptional case
        // pick an arbitrary vector perp to normal because we are either 
	// rotating by 0 or 180 degrees
        if (next_normal[0] != 0) {
            axis = vrpn_HapticVector(next_normal[1], -next_normal[0], 0);
#ifdef	VRPN_USE_HDAPI
            axis /= (axis.magnitude());
#else
            axis /= (axis.distToOrigin());
#endif
        } else {
            axis = vrpn_HapticVector(1,0,0);
	}
    } else {	// normal case
        axis = crossprod/sintheta;
    }
    theta = asin(sintheta);
    if (costheta < 0) theta = VRPN_PI - theta;

    // set the incremental rotation
#ifdef	VRPN_USE_HDAPI
    vrpn_HapticMatrix rot_incrxform;
    rot_incrxform.createRotation(axis, theta);
#else
    vrpn_HapticMatrix rot_incrxform;
    rot_incrxform.setRotate(axis,theta);
#endif

    // Set the rotation portion of the matrix based on the accumulation
    // of its previous value and the new rotation.
    double prod[3][3];
    int i,j;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            prod[i][j] = xform_to_update[i][0]*rot_incrxform[0][j] +
			 xform_to_update[i][1]*rot_incrxform[1][j] +
			 xform_to_update[i][2]*rot_incrxform[2][j];
        }
    }
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
          xform_to_update[i][j] = prod[i][j];
        }
    }

    // Set the translation for the new matrix.  XXX For HDAPI, I'm not sure
    // we are putting the translation into the proper portion of the
    // matrix.
#ifdef	VRPN_USE_HDAPI
    for (i = 0; i < 3; i++) {
      xform_to_update[3][i] = trans[i];
    }
#else
    xform_to_update.setTranslate(trans);
#endif

    return;
}

// XXX This is almost certainly broken in the HDAPI implementation.
// Don't trust it until you've verified that it works.  If you're lucky,
// the fix will require only swapping the matrix multiplication types.
void DynamicPlane::setPlaneFromTransform(vrpn_HapticPlane &pl, 
					 vrpn_HapticMatrix &xfm)
{
#ifdef	VRPN_USE_HDAPI
	vrpn_HapticVector normal(0,1,0);
	//normal = 
      xfm.multVecMatrix(normal, normal);
	vrpn_HapticPosition origin(0,0,0);
	//origin = 
      xfm.multMatrixVec(origin, origin);
        pl = vrpn_HapticPlane(normal[0], normal[1], normal[2], -origin.magnitude());
#else
	vrpn_HapticVector normal(0,1,0);
	normal = xfm.fromLocal(normal);
	vrpn_HapticPosition origin(0,0,0);
	origin = xfm.fromLocal(origin);
        pl = vrpn_HapticPlane(normal[0], normal[1], normal[2], -origin.distToOrigin());
#endif
	//printf("XXX %lf, %lf, %lf,  %lf\n", pl.a(), pl.b(), pl.c(), pl.d());
}

#ifndef	VRPN_USE_HDAPI
gstType TexturePlane::PlaneClassTypeId;
static int temp = TexturePlane::initClass();
#endif

void TexturePlane::init() {
	isNewPlane = FALSE;
	inEffect= FALSE;
	isInRecovery = FALSE;
	recoveryPlaneCount = 0;
	originalPlane = plane;
	lastDepth = 0;
	dIncrement = 0;
	numRecoveryCycles = 1;
	//  boundingRadius = 1000.0;
#ifndef	VRPN_USE_HDAPI
	invalidateCumTransf();
	dynamicParent = NULL;
#endif
	textureOrigin = vrpn_HapticPosition(0,0,0);

	usingTexture = 0;
	texWL = 10.0;
	texWN = 1/texWL;
	texAmp = 0;
	textureAspect = texAmp*2.0/texWL;

	fadeActive = FALSE;
	fadeOldKspring = 0;
	dSpring_dt = 0;
	dataCB = NULL;
	userdata = NULL;
	data_time = 0;
	safety_ineffect = TRUE;
	newCoordinates = FALSE;
	inContact = FALSE;
	init_mutex(&tex_param_mutex);
}

//constructors
TexturePlane::TexturePlane(const vrpn_HapticPlane & p)
{ 
	init();
	plane= vrpn_HapticPlane(p);
}

TexturePlane::TexturePlane(const vrpn_HapticPlane * p)
{
	init();
	plane = vrpn_HapticPlane(*p);
}

TexturePlane::TexturePlane(const TexturePlane *p)
{ 
	init();
	plane = vrpn_HapticPlane(p->plane);
}

TexturePlane::TexturePlane(const TexturePlane &p)
{ 
	init();
	plane = vrpn_HapticPlane(p.plane);
}


TexturePlane::TexturePlane(double a,double b, double c, double d) 
{  
	init();
	vrpn_HapticVector vec = vrpn_HapticVector(a,b,c);
	plane = vrpn_HapticPlane(vec,d);
}

void TexturePlane::setPlane(vrpn_HapticVector newNormal, vrpn_HapticPosition point) 
{ 
	plane.setPlane(newNormal,point);
	isNewPlane = TRUE;
}


void TexturePlane::setParameters(double fd, double fs, double ks,
								 double bpa, double bpl){
	setSurfaceFdynamic(fd);
	setSurfaceFstatic(fs);
	setSurfaceKspring(ks);
	setTextureAmplitude(bpa);
	setTextureWavelength(bpl);
}

void TexturePlane::update(double a, double b, double c, double d)
{
	plane = vrpn_HapticPlane(a,b,c,d);
	isNewPlane = TRUE;
}

void TexturePlane::setTextureWavelength(double wavelength){
	if (wavelength < MIN_TEXTURE_WAVELENGTH) return;
	get_mutex(&tex_param_mutex);
	new_texWN = 1.0/wavelength;
	texWN_needs_update = TRUE;
	release_mutex(&tex_param_mutex);
}

void TexturePlane::setTextureWaveNumber(double wavenum){
	if (wavenum == 0 || 1/wavenum < MIN_TEXTURE_WAVELENGTH) return;
	get_mutex(&tex_param_mutex);
	new_texWN = wavenum;
	texWN_needs_update = TRUE;
	release_mutex(&tex_param_mutex);
}

void TexturePlane::setTextureAmplitude(double amplitude){
	get_mutex(&tex_param_mutex);
	new_texAmp = amplitude;
	texAmp_needs_update = TRUE;
	release_mutex(&tex_param_mutex);
}

void TexturePlane::setTextureSize(double size){
	if (size < MIN_TEXTURE_WAVELENGTH) return;
	get_mutex(&tex_param_mutex);
	new_Size = size;
	texSize_needs_update = TRUE;
	release_mutex(&tex_param_mutex);
}

void TexturePlane::setTextureAspectRatio(double aspectRatio){
	get_mutex(&tex_param_mutex);
	new_textureAspect = aspectRatio;
	texAspect_needs_update = TRUE;
	release_mutex(&tex_param_mutex);
}

void TexturePlane::updateTextureWavelength(){
	get_mutex(&tex_param_mutex);
	if (texWN_needs_update){
		texWN_needs_update = FALSE;
		texWN = new_texWN;
		texWL = 1.0/texWN;
		textureAspect = 2.0*texAmp/texWL;
	}
	release_mutex(&tex_param_mutex);
}

void TexturePlane::updateTextureAmplitude(){
	get_mutex(&tex_param_mutex);
	if (texAmp_needs_update){
		texAmp_needs_update = FALSE;
		texAmp = new_texAmp;
		textureAspect = 2.0*texAmp/texWL;
	}
	release_mutex(&tex_param_mutex);
}

void TexturePlane::updateTextureSize() {
	get_mutex(&tex_param_mutex);
	if (texSize_needs_update){
		texSize_needs_update = FALSE;
		texWL = new_Size;
		texWN = 1.0/texWL;
		texAmp = textureAspect*texWL/2.0;
	}
	release_mutex(&tex_param_mutex);
}

void TexturePlane::updateTextureAspectRatio() {
	get_mutex(&tex_param_mutex);
	if (texAspect_needs_update){
		texAspect_needs_update = FALSE;
		textureAspect = new_textureAspect;
		texAmp = textureAspect*texWL/2.0;
	}
	release_mutex(&tex_param_mutex);
}

vrpn_HapticBoolean TexturePlane::intersection(const vrpn_HapticPosition &startPt_WC,
			       const vrpn_HapticPosition &endPt_WC,
			       vrpn_HapticPosition &intersectionPt_WC,
			       vrpn_HapticVector &intersectionPtNormal_WC,
			       void **)
{
	// XXX - this function doesn't do the right thing if there
	// are any other shapes in the scene
	return FALSE;
}


#ifndef	VRPN_USE_HDAPI
vrpn_HapticBoolean TexturePlane::collisionDetect(gstPHANToM *PHANToM)
{ 
	vrpn_HapticPosition phantomPos, lastPhantPos, intersectionPoint;
	
	double depth = 0;
	vrpn_HapticPosition SCP;
	vrpn_HapticVector SCPnormal;
	double deltaT;
	static double deltaDist;
	vrpn_HapticPosition diff;
	deltaT = PHANToM->getDeltaT();

	if (fadeActive)
		incrementFade(deltaT);

	vrpn_HapticVector phantomForce = PHANToM->getReactionForce_WC();
	PHANToM->getLastPosition_WC(lastPhantPos);
	PHANToM->getPosition_WC(phantomPos);
	diff = lastPhantPos - phantomPos;
	deltaDist = diff.distToOrigin();

	if (dataCB){
		dataCB(data_time, lastPhantPos, phantomForce, userdata);
		data_time += deltaT;
	}

	//if the plane node is not in effect
	if(inEffect == FALSE){
		safety_ineffect = TRUE;
		return FALSE;
	}

	inContact = getStateForPHANToM(PHANToM);

#ifdef VRPN_USE_GHOST_31
	if(!_touchableByPHANToM || _resetPHANToMContacts) {
#else // Ghost 4.0 (and the default case)
	if(!isTouchableByPHANToM() || _resetPHANToMContacts) {
#endif
		_resetPHANToMContacts = FALSE;
		
		inContact = FALSE;
		(void) updateStateForPHANToM(PHANToM,inContact);
		//printf("in if incontact is false\n");

		return inContact;
	}

	phantomPos = fromWorld(phantomPos);

	//project the current phantomPosition onto the plane to get the SCP
	
	// if we don't have the constant plane assumed by texture computing
	// functions
	if (!usingTexture || !usingAssumedTextureBasePlane(plane) || 
			!dynamicParent){
		depth = -(phantomPos[0]*plane.a() + phantomPos[1]*plane.b() +
			phantomPos[2]*plane.c() + plane.d());
		SCP = plane.projectPoint(phantomPos);
		//set the SCPnormal to be the normal of the plane
		SCPnormal = plane.normal();
		texAmp = 0;	// extra precaution in case plane happens to 
					// become (0,1,0,0) we don't want to suddenly add
					// texture
	} else {

		// adjust things so plane update for parent node preserves 
		// projection of phantom position in our coordinate system
		// 
		if (dynamicParent){
			if (newCoordinates){
				t_elapsed = 0;
				newCoordinates = FALSE;
			} else {
				t_elapsed += deltaT;
			}
			vrpn_HapticPosition posInPreviousCoordinates;
			PHANToM->getPosition_WC(posInPreviousCoordinates);
			posInPreviousCoordinates = dynamicParent->fromWorldLast(posInPreviousCoordinates);
			posInPreviousCoordinates = fromParent(posInPreviousCoordinates);

			if (posInPreviousCoordinates != phantomPos){

//				if (t_elapsed > 0.0){
//					printf("big t_elapsed: %f\n", t_elapsed);
//				}
				vrpn_HapticPosition currProj = plane.projectPoint(phantomPos);
				vrpn_HapticPosition prevProj = 
					plane.projectPoint(posInPreviousCoordinates); 
					// this is the projection of the
					// current position onto the
					// previous plane (as defined by
					// the parent transformation)
				if (!dynamicParent->dynamicMoveThisServoLoop())
					printf("error: dynamicParent move not detected\n");

				textureOrigin[0] = textureOrigin[0] - prevProj[0] + currProj[0];
				textureOrigin[1] = 0;
				textureOrigin[2] = textureOrigin[2] - prevProj[2] + currProj[2];
			}
		}

		// move textureOrigin from its current location towards current
		// position in steps of length texWL until it is within texWL of
		// the current position
		updateTextureOrigin(phantomPos[0], phantomPos[2]);

		// translate to texture coordinates - note: this is a translation in the plane of
		// the phantom position
		vrpn_HapticPosition texturePos = phantomPos - textureOrigin;

		SCP = computeSCPfromGradient(texturePos);
		vrpn_HapticPlane texPlane;
		texPlane = computeTangentPlaneAt(texturePos);
		depth = -(texturePos.x()*texPlane.a() + texturePos.y()*texPlane.b() +
			texturePos.z()*texPlane.c() + texPlane.d());

		// go back to untranslated (non-texture) plane coordinates
		SCP = SCP + textureOrigin;
		SCPnormal = texPlane.normal();

		// if (|texturePos| is close to 0.25*texWL or 0.75*texWL then we
		//  are near a zero crossing so its relatively safe to
		//  update texture shape
		double old_wl = texWL;
		double radius = sqrt(texturePos[0]*texturePos[0] + texturePos[2]*texturePos[2]);// this is between 0 and texWL
		if (fabs(radius - 0.25*texWL) < deltaDist ||
			fabs(radius - 0.75*texWL) < deltaDist){
			if (texWN_needs_update) updateTextureWavelength();
			if (texAmp_needs_update) updateTextureAmplitude();
			if (texSize_needs_update) updateTextureSize();
			if (texAspect_needs_update) updateTextureAspectRatio();

			if (old_wl != texWL && old_wl != 0){	// adjust to maintain phase continuity
				textureOrigin[0] = -texWL*texturePos[0]/old_wl + phantomPos[0];
				textureOrigin[2] = -texWL*texturePos[2]/old_wl + phantomPos[2];
			}
		}
	}

	if (depth <= 0) {	// positive depth is below surface
	    inContact = FALSE;
		lastDepth = 0;
		safety_ineffect = FALSE;
	    (void) updateStateForPHANToM(PHANToM,inContact);
	    return inContact;
	}
	// if we suddenly get a new plane that causes the depth in the
	// plane to be large then we don't want to let the phantom force
	// get large
	else if (safety_ineffect) {
		inContact = FALSE;
		lastDepth = 0;
		(void) updateStateForPHANToM(PHANToM,inContact);
		return inContact;
	}
	else if ((depth - lastDepth)*getSurfaceKspring() > MAX_FORCE){
		inContact = FALSE;
		lastDepth = 0;
		fprintf(stderr, "Warning: exceeded max force change\n");
		fprintf(stderr, "  move out of surface to re-enable forces\n");
		safety_ineffect = TRUE;
		(void) updateStateForPHANToM(PHANToM,inContact);
		return inContact;
	}
	else {	// In contact
	    inContact = TRUE;
	    (void)updateStateForPHANToM(PHANToM, inContact);
	    addCollision(PHANToM,SCP,SCPnormal);  
	}	

	// this value is important for safety checks
	lastDepth = depth;

	return inContact;
}
#endif

void TexturePlane::incrementFade(double dT){
	double curr_spr = getSurfaceKspring();
	double next_spr = curr_spr - dT*dSpring_dt;

	if (next_spr > 0)
		setSurfaceKspring(next_spr);
	else{
		inEffect = FALSE;
		fadeActive = FALSE;
		setSurfaceKspring(fadeOldKspring);
	}
}

vrpn_HapticBoolean TexturePlane::usingAssumedTextureBasePlane(vrpn_HapticPlane &p){
	return !(p.a() != 0 || p.b() != 1 || p.c() != 0 || p.d() != 0);
}

// given a point, compute the tangent to the
// point at the orthogonal projection intersection point
vrpn_HapticPlane TexturePlane::computeTangentPlaneAt(vrpn_HapticPosition pnt) const
{
	// compute normal at pnt[0],pnt[2]
	vrpn_HapticVector pNormal = computeNormal(pnt[0],pnt[2]);
	// assume pnt is in the tangent plane
	vrpn_HapticPosition proj = projectPointOrthogonalToStaticPlane(pnt);
	vrpn_HapticPlane tangPlane = vrpn_HapticPlane(pNormal, proj);
	return tangPlane;
}


vrpn_HapticPosition TexturePlane::projectPointOrthogonalToStaticPlane(vrpn_HapticPosition pnt) const
{
	double y = computeHeight(pnt[0], pnt[2]);
	vrpn_HapticPosition projection = pnt;
	projection[1] = y;
	return projection;
}

// computes an scp such that the vector from currentPos to
// the scp gives a force in the direction of the normal but
// with magnitude such that the component in the direction
// perpendicular to the base plane is equal to the depth
// below the surface (where the point on the surface is
// found by projecting the current position along the base
// plane normal to the texture plane:
/*
           2<- 1/--------\   <- bump on plane
              /            \
------------/  c             \---------- <- base plane

  c = current position
  1 = projection to textured surface along normal to base plane
  2 = projection along normal for textured surface with height
		from base plane equal to that for 1 (this is the scp returned)

  */
/*
	IMPORTANT: This function assumes that the plane equation is
	y = 0 for simplicity of computation
	the position is expected to be in a coordinate system such that
	the plane has this equation. 
	Therefore, to simulate other planes, the position in world coordinates
	must be transformed correctly

  */
vrpn_HapticPosition TexturePlane::computeSCPfromGradient(vrpn_HapticPosition currentPos) const
{
	double pos_x, pos_y, pos_z, pos_r; // position in local coordinates
	double scp_h;	// scp in local coordinates

	// first, translate position into texture coordinates
	pos_x = (currentPos[0]);
	pos_y = currentPos[1];
	pos_z = (currentPos[2]);

	pos_r = sqrt(pos_x*pos_x + pos_z*pos_z);

	// the first time we contact
	if (!inContact){
		return currentPos;
	}

	vrpn_HapticPosition newSCP;

	// get height of surface at contact point
	scp_h = computeHeight(pos_r);

	// return current position if we are not touching the surface
	if (scp_h < pos_y) 
		newSCP = currentPos;
	else {
		vrpn_HapticVector normal = computeNormal(pos_x, pos_z);
		normal.normalize();

		double delta_y = scp_h - pos_y;

		// compute scp in untranslated coordinates
		newSCP[0] = pos_x + normal[0]*(delta_y/normal[1]);
		newSCP[1] = scp_h;
		newSCP[2] = pos_z + normal[2]*(delta_y/normal[1]);
	}

	return newSCP;
}

// for texture:
void TexturePlane::updateTextureOrigin(double x, double z)
{
	if (texWN == 0) return;
	double r_x = (x - textureOrigin[0]);
	double r_z = (z - textureOrigin[2]);
	double r_sq = r_x*r_x + r_z*r_z;
	double r = sqrt(r_sq);
	r_x /= r;
	r_z /= r;
	// (r_x, r_z) is the direction along which we want to move origin
	double num_radii = r/texWL;
	num_radii = floor(num_radii);
	if (num_radii >= 1){
		textureOrigin[0] += num_radii*texWL*r_x;
		textureOrigin[2] += num_radii*texWL*r_z;
	}
}

// for texture:
double TexturePlane::computeHeight(double x, double z) const
{
	double r_sq = x*x + z*z;
	double r = sqrt(r_sq);

	double height = computeHeight(r);
	return height;
}

// for texture: compute height as a function of radius from texture origin
double TexturePlane::computeHeight(double r) const
{
	double k = 2*VRPN_PI*texWN;
	return texAmp*cos(k*r);
}

// for texture:
vrpn_HapticVector TexturePlane::computeNormal(double x, double z) const
{
	double k = 2*VRPN_PI*texWN;
	double r_sq = x*x + z*z;
	double r = sqrt(r_sq);

	vrpn_HapticVector normal;

	if (r != 0){
		normal = vrpn_HapticVector(texAmp*x*k*sin(k*r)/r, 1.0, texAmp*z*k*sin(k*r)/r);
	}
	else{
		normal = vrpn_HapticVector(0, 1.0, 0);
	}
	return normal;
	
}

// returns what a single wavelength looks like (height in mm as a function
// of distance from texture origin)
void TexturePlane::getTextureShape(int nsamples, float *surface) const
{
	
	double	radius = 0;
	double	incr = texWL/(float)(nsamples-1);
	int i;

	if (texWN == 0) {
		surface[0] = (float)computeHeight(0);
		for (i = 1; i < nsamples; i++){
			surface[i] = surface[0];
		}
	}
	else {
		for (i = 0; i < nsamples; i++){
			// (radius, height)
			surface[i] = (float)computeHeight(radius);
			radius += incr;
		}
	}
}

#endif
