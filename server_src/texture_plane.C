#include  "vrpn_Configure.h"
#ifdef	VRPN_USE_PHANTOM_SERVER
#include "texture_plane.h"

// macros for printing something out x times out of a thousand (i.e. x times per second)
#define DEBUG_BEGIN(x)	{static int pcnt = 0; if ((pcnt % 1000 >= 0)&&(pcnt%1000 < x)){
#define DEBUG_END	}pcnt++;}
//#define VERBOSE

#define MAX_FORCE (10.0)

// Initialize class.
gstType DynamicPlane::DynamicPlaneClassTypeId;

int DynamicPlaneTest = DynamicPlane::initClass();

int DynamicPlane::initClass() {
    DynamicPlaneClassTypeId = -1000;
    return 0;
}

DynamicPlane::DynamicPlane() : gstDynamic()
{
	// initialize transform to identity
	xform.identity();
	fixedPlane = new TexturePlane(0.0, 1.0, 0.0, 0.0);
	fixedPlane->setParent(this);
	buzzForce = new BuzzForceField();
	buzzForce->setPlane(gstPlane(0.0, 1.0, 0.0, 0.0));
	buzzForce->setSpring(fixedPlane->getSurfaceKspring());
	buzzForce->restrictToSurface(TRUE);
	buzzForce->adjustToTexturePlane(fixedPlane);

	plane = gstPlane(0.0, 1.0, 0.0, 0.0);
	lastPlane = gstPlane(plane);
	addChild(fixedPlane);
	addChild(buzzForce);
	_using_buzz = FALSE;
	_active = FALSE;
	_is_new_plane = FALSE;
	buzzForce->deactivate();
	fixedPlane->enableTexture();
	fixedPlane->setTextureSize(10.0);
	fixedPlane->setTextureAspectRatio(0.0);
	fixedPlane->setInEffect(FALSE);
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_init(&xform_mutex,NULL);
#else
	InitializeCriticalSection(&xform_mutex);
#endif
}

void DynamicPlane::setParameters(double fd, double fs, double ks,
								 double bpa, double bpl, 
								 double buzzamp, double buzzfreq){
	fixedPlane->setParameters(fd, fs, ks, bpa, bpl);
	buzzForce->setParameters(buzzamp, buzzfreq, ks);
}


void DynamicPlane::setActive(gstBoolean active) {_active = active;
    fixedPlane->setInEffect(active);
    if (_active && _using_buzz) buzzForce->activate();
    else buzzForce->deactivate();
}

void DynamicPlane::cleanUpAfterError(){
	_is_new_plane = FALSE;
	setActive(FALSE);
}

void DynamicPlane::enableBuzzing(gstBoolean enable) {
    _using_buzz = enable;
    if (_active && _using_buzz) buzzForce->activate();
    else buzzForce->deactivate();
}
void DynamicPlane::enableTexture(gstBoolean enable) {
    if (enable) fixedPlane->enableTexture();
    else fixedPlane->disableTexture();
}

void DynamicPlane::setSurfaceKspring(double ks) {
    buzzForce->setSpring(ks);
    fixedPlane->setSurfaceKspring(ks);
}

double DynamicPlane::getBuzzAmplitude() {
    return buzzForce->getAmplitude();}

void DynamicPlane::setBuzzAmplitude(double amp) {
    buzzForce->setAmplitude(amp);}

double DynamicPlane::getBuzzFrequency() {
    return buzzForce->getFrequency();}

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
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&xform_mutex);
#else
	EnterCriticalSection(&xform_mutex);
#endif
	if (_is_new_plane){
		planeEquationToTransform(lastPlane, plane, xform);
		// if we don't compute plane from xform then any
		// roundoff error in xform will be allowed to accumulate without
		// correction; by computing the actual plane represented by xform
		// it should cause updates to tend to reduce roundoff error to
		// zero if plane stays constant for a while or at least a small 
		// bounded amount if plane is always changing - this has been verified
		// in a test program
		setPlaneFromTransform(plane, xform);
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&xform_mutex);
#else
		LeaveCriticalSection(&xform_mutex);
#endif
		setTransformMatrixDynamic(xform);
		_is_new_plane = FALSE;
	}
	else
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&xform_mutex);
#else
		LeaveCriticalSection(&xform_mutex);
#endif
	gstDynamic::updateDynamics();
}

void DynamicPlane::update(double a,double b,double c,double d)
{
	//static int err_cnt = 0;
	
	// no need to do anything unless this plane is different from
	// the previous one
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&xform_mutex);
#else
	EnterCriticalSection(&xform_mutex);
#endif
	if (plane.a() != a || plane.b() != b ||
		plane.c() != c || plane.d() != d){
		if (_is_new_plane) {
		//	err_cnt++;
		//	if (err_cnt > 3)
		//		printf("waited %d msec\n", err_cnt);
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&xform_mutex);
#else
			LeaveCriticalSection(&xform_mutex);
#endif
			return;
		}
//		else err_cnt = 0;
		lastPlane = gstPlane(plane);
		fixedPlane->signalNewTransform();
		plane = gstPlane(a,b,c,d);
		_is_new_plane = TRUE;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&xform_mutex);
#else
		LeaveCriticalSection(&xform_mutex);
#endif
		addToDynamicList();
	}
	else
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&xform_mutex);
#else
		LeaveCriticalSection(&xform_mutex);
#endif
}


// planeEquationToTransform:
// Given a plane that passes through the origin (0,0,0), and a transformation
// xform that transforms that plane to prev_plane, this function modifies
// xform so that it transforms the plane to next_plane. The incremental
// transformation composed with xform to achieve this consists of a rotation
// about the axis that is the intersection of prev_plane with next_plane.

void DynamicPlane::planeEquationToTransform(gstPlane &prev_plane,
    gstPlane &next_plane, gstTransformMatrix &xform_to_update) 
{
    // plane is [a,b,c,d] where ax + by +cz + d = 0


    gstVector next_normal = next_plane.normal();
    gstVector prev_normal = prev_plane.normal();
    double m = next_normal.distToOrigin();

    double distAlongNormal = -next_plane.d();

    gstVector axis = gstVector(0,0,0);
    double theta = 0;
    gstVector trans;

    trans = distAlongNormal*next_normal; // translation vector for xform

    // compute the rotation axis and angle
    gstVector crossprod = prev_normal.cross(next_normal);
    double sintheta = crossprod.distToOrigin();
    double costheta = prev_normal.dot(next_normal);
    if (sintheta == 0){	// exceptional case
        // pick an arbitrary vector perp to normal because we are either 
	// rotating by 0 or 180 degrees
        if (next_normal[0] != 0){
            axis = gstVector(next_normal[1], -next_normal[0], 0);
            axis /= (axis.distToOrigin());
        } else
            axis = gstVector(1,0,0);
    } else	// normal case
        axis = crossprod/sintheta;
    theta = asin(sintheta);
    if (costheta < 0) theta = M_PI - theta;

    // set the incremental rotation
    gstTransformMatrix rot_incrxform;
    rot_incrxform.setRotate(axis,theta);
    double rot_cumulative[3][3], prod[3][3], rot_incr[3][3];


	xform_to_update.getRotationMatrix(rot_cumulative);
    rot_incrxform.getRotationMatrix(rot_incr);
    int i,j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++){
            prod[i][j] = rot_cumulative[i][0]*rot_incr[0][j] +
			 rot_cumulative[i][1]*rot_incr[1][j] +
			 rot_cumulative[i][2]*rot_incr[2][j];
        }

	xform_to_update.setRotationMatrix(prod);
	xform_to_update.setTranslate(trans);

	return;
}

void DynamicPlane::setPlaneFromTransform(gstPlane &pl, 
					gstTransformMatrix &xfm){
    gstVector normal(0,1,0);
	normal = xfm.fromLocal(normal);
	gstPoint origin(0,0,0);
	origin = xfm.fromLocal(origin);
	pl = gstPlane(normal[0], normal[1], normal[2], -origin.distToOrigin());
}

gstType TexturePlane::PlaneClassTypeId;
static int temp = TexturePlane::initClass();

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
	invalidateCumTransf();

	textureOrigin = gstPoint(0,0,0);
	dynamicParent = NULL;

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
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_init(&tex_param_mutex,NULL);
#else
	InitializeCriticalSection(&tex_param_mutex);
#endif
}

//constructors
TexturePlane::TexturePlane(const gstPlane & p)
{ 
	init();
	plane= gstPlane(p);
}

TexturePlane::TexturePlane(const gstPlane * p)
{
	init();
	plane = gstPlane(p);
}

TexturePlane::TexturePlane(const TexturePlane *p)
{ 
	init();
	plane = gstPlane(p->plane);
}

TexturePlane::TexturePlane(const TexturePlane &p)
{ 
	init();
	plane = gstPlane(p.plane);
}


TexturePlane::TexturePlane(double a,double b, double c, double d) 
{  
	init();
	gstVector vec = gstVector(a,b,c);
	plane = gstPlane(vec,d);
}

void TexturePlane::setPlane(gstVector newNormal, gstPoint point) 
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
	plane = gstPlane(a,b,c,d);
	isNewPlane = TRUE;
}

void TexturePlane::setTextureWavelength(double wavelength){
	if (wavelength < MIN_TEXTURE_WAVELENGTH) return;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	new_texWN = 1.0/wavelength;
	texWN_needs_update = TRUE;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

void TexturePlane::setTextureWaveNumber(double wavenum){
	if (wavenum == 0 || 1/wavenum < MIN_TEXTURE_WAVELENGTH) return;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	new_texWN = wavenum;
	texWN_needs_update = TRUE;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

void TexturePlane::setTextureAmplitude(double amplitude){
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	new_texAmp = amplitude;
	texAmp_needs_update = TRUE;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

void TexturePlane::setTextureSize(double size){
	if (size < MIN_TEXTURE_WAVELENGTH) return;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	new_Size = size;
	texSize_needs_update = TRUE;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

void TexturePlane::setTextureAspectRatio(double aspectRatio){
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	new_textureAspect = aspectRatio;
	texAspect_needs_update = TRUE;
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

void TexturePlane::updateTextureWavelength(){
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	if (texWN_needs_update){
		texWN_needs_update = FALSE;
		texWN = new_texWN;
		texWL = 1.0/texWN;
		textureAspect = 2.0*texAmp/texWL;
	}
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

void TexturePlane::updateTextureAmplitude(){
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	if (texAmp_needs_update){
		texAmp_needs_update = FALSE;
		texAmp = new_texAmp;
		textureAspect = 2.0*texAmp/texWL;
	}
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

void TexturePlane::updateTextureSize() {
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	if (texSize_needs_update){
		texSize_needs_update = FALSE;
		texWL = new_Size;
		texWN = 1.0/texWL;
		texAmp = textureAspect*texWL/2.0;
	}
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

void TexturePlane::updateTextureAspectRatio() {
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_lock(&tex_param_mutex);
#else
	EnterCriticalSection(&tex_param_mutex);
#endif
	if (texAspect_needs_update){
		texAspect_needs_update = FALSE;
		textureAspect = new_textureAspect;
		texAmp = textureAspect*texWL/2.0;
	}
// MB: for SGI compilation with pthreads
#ifdef SGI
	pthread_mutex_unlock(&tex_param_mutex);
#else
	LeaveCriticalSection(&tex_param_mutex);
#endif
}

gstBoolean TexturePlane::intersection(const gstPoint &startPt_WC,
			       const gstPoint &endPt_WC,
			       gstPoint &intersectionPt_WC,
			       gstVector &intersectionPtNormal_WC,
			       void **)
{   
	// XXX - this function doesn't do the right thing if there
	// are any other shapes in the scene
	return FALSE;
}


gstBoolean TexturePlane::collisionDetect(gstPHANToM *PHANToM)
{ 
	gstPoint phantomPos, lastPhantPos, intersectionPoint;
	
	double depth = 0;
	gstPoint SCP;
	gstVector SCPnormal;
	double deltaT;
	static double deltaDist;
	gstPoint diff;
	deltaT = PHANToM->getDeltaT();

	if (fadeActive)
		incrementFade(deltaT);

	gstVector phantomForce = PHANToM->getReactionForce_WC();
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

	if(!isTouchableByPHANToM() || _resetPHANToMContacts) {
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
			gstPoint posInPreviousCoordinates;
			PHANToM->getPosition_WC(posInPreviousCoordinates);
			posInPreviousCoordinates = dynamicParent->fromWorldLast(posInPreviousCoordinates);
			posInPreviousCoordinates = fromParent(posInPreviousCoordinates);

			if (posInPreviousCoordinates != phantomPos){

//				if (t_elapsed > 0.0){
//					printf("big t_elapsed: %f\n", t_elapsed);
//				}
				gstPoint currProj = plane.projectPoint(phantomPos);
				gstPoint prevProj = 
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
		gstPoint texturePos = phantomPos - textureOrigin;

		SCP = computeSCPfromGradient(texturePos);
		gstPlane texPlane;
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

gstBoolean TexturePlane::usingAssumedTextureBasePlane(gstPlane &p){
	return !(p.a() != 0 || p.b() != 1 || p.c() != 0 || p.d() != 0);
}

// given a point, compute the tangent to the
// point at the orthogonal projection intersection point
gstPlane TexturePlane::computeTangentPlaneAt(gstPoint pnt)
{
	// compute normal at pnt[0],pnt[2]
	gstVector pNormal = computeNormal(pnt[0],pnt[2]);
	// assume pnt is in the tangent plane
	gstPoint proj = projectPointOrthogonalToStaticPlane(pnt);
	gstPlane tangPlane = gstPlane(pNormal, proj);
	return tangPlane;
}


gstPoint TexturePlane::projectPointOrthogonalToStaticPlane(gstPoint pnt)
{
	double y = computeHeight(pnt[0], pnt[2]);
	gstPoint projection = pnt;
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
gstPoint TexturePlane::computeSCPfromGradient(gstPoint currentPos)
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

	gstPoint newSCP;

	// get height of surface at contact point
	scp_h = computeHeight(pos_r);

	// return current position if we are not touching the surface
	if (scp_h < pos_y) 
		newSCP = currentPos;
	else {
		gstVector normal = computeNormal(pos_x, pos_z);
		normal = normal/normal.norm();

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
double TexturePlane::computeHeight(double x, double z)
{
	double r_sq = x*x + z*z;
	double r = sqrt(r_sq);

	double height = computeHeight(r);
	return height;
}

// for texture: compute height as a function of radius from texture origin
double TexturePlane::computeHeight(double r)
{
	double k = 2*M_PI*texWN;
	return texAmp*cos(k*r);
}

// for texture:
gstVector TexturePlane::computeNormal(double x, double z)
{
	double k = 2*M_PI*texWN;
	double r_sq = x*x + z*z;
	double r = sqrt(r_sq);

	gstVector normal;

	if (r != 0){
		normal.setx(texAmp*x*k*sin(k*r)/r);
		normal.sety(1.0);					
		normal.setz(texAmp*z*k*sin(k*r)/r);
	}
	else{
		normal.setx(0);
		normal.sety(1.0);
		normal.setz(0);
	}
	return normal;
	
}

// returns what a single wavelength looks like (height in mm as a function
// of distance from texture origin)
void TexturePlane::getTextureShape(int nsamples, float *surface)
{
	
	float radius = 0;
	float incr = texWL/(float)(nsamples-1);
	int i;

	if (texWN == 0) {
		surface[0] = computeHeight(0);
		for (i = 1; i < nsamples; i++){
			surface[i] = surface[0];
		}
	}
	else {
		for (i = 0; i < nsamples; i++){
			// (radius, height)
			surface[i] = computeHeight(radius);
			radius += incr;
		}
	}
}

#endif
