//
// forcedevice_test_client.cpp - Program to test out the various functions
//	of a vrpn_ForceDevice server, inspired by Randy Heiland.  First,
//	it generates a box within which the user can feel around; the walls
//	have different characteristics.  It later generates force fields
//	and instant buzzing effects.
//
//	It is meant to test devices that have a button and tracker associated
//	with them, because it uses these devices to set coordinate systems and
//	locations for effects.
//

#include <math.h>                       // for fabs, sqrt
#include <stdio.h>                      // for printf, NULL
#include <stdlib.h>                     // for exit
#include <vrpn_Shared.h>                // for vrpn_gettimeofday
#include <vrpn_Button.h>                // for vrpn_Button_Remote, etc
#include <vrpn_ForceDevice.h>           // for vrpn_ForceDevice_Remote, etc
#include <vrpn_Tracker.h>               // for vrpn_TRACKERCB, etc

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Connection.h"            // for vrpn_Connection
#include "vrpn_Shared.h"                // for timeval, vrpn_TimevalDiff, etc
#include "vrpn_Types.h"                 // for vrpn_float32, vrpn_int32, etc

/*****************************************************************************
 *
   Classes and types and such
 *
 *****************************************************************************/

// What type of effect the application is presenting
typedef	enum { box, pointconstraint, lineconstraint, planeconstraint, forcefield, buzzing, geometry, quit } APP_STATE;

// Which Object ID for the cube geometry object
vrpn_int32  CUBE_ID = 0;

// For the display of the box, this holds a description of the parameters
// for each side.
class BoxSide {
public:
  BoxSide(double ox, double oy, double oz,
	  double nx, double ny, double nz,
	  double sMult, double fMult, double dMult)
  { d_oX = (float)ox; d_oY = (float)oy; d_oZ = (float)oz;
    d_nX = (float)nx; d_nY = (float)ny; d_nZ = (float)nz;
    d_sMult = (float)sMult;
    d_fMult = (float)fMult;
    d_dMult = (float)dMult;
  }

  float	d_oX;	  //< Offset from center in X direction
  float	d_oY;	  //< Offset from center in Y direction
  float	d_oZ;	  //< Offset from center in Z direction

  float	d_nX;	  //< Normal in X;
  float	d_nY;	  //< Normal in Y;
  float	d_nZ;	  //< Normal in Z;

  float	d_sMult;  //< (signed) How many multiples of change in spring constant to add
  float	d_fMult;  //< (signed) How many multiples of change in friction to add
  float	d_dMult;  //< (signed) How many multiples of change in damping to add
};

/*****************************************************************************
 *
   Global variables
 *
 *****************************************************************************/

// Force device client to use to control the server.
static	vrpn_ForceDevice_Remote *g_forceDevice;

// Standard stiffness of a surface, and factor to multiply/divide by when it varies
static	float g_kSpring = (float)0.5;	  // Unit is unknown, seems to be range is 0 < value <= 1.
static	float g_kSpringMult = 2;

// Standard damping of a surface, and factor to multiply/divide by when it varies
static	float g_kDamping = (float)0.001;	  // Unit is dynes*sec/cm.  Range is 0 <= value <= 0.005
static	float g_kDampingMult = 2;

// Standard values for static and dynamic friction
// Factors by which to multiple/divide static and dynamic friction when they vary
static  float g_sFric = (float)0.2;	  // Unit is fraction of normal force, range is 0 <= value <= 1
//static	float g_dFric = (float)0.2;	  // Unit is fraction of normal force, range is 0 <= value <= 1, dynamic <= static
static	float g_sFricMult = (float)2;
//static	float g_dFricMult = (float)2;

// State of the application (what should be generated now).
static	APP_STATE g_state = box;	  //< What mode are we in?
static	bool	  g_active = false;	  //< Is the current mode active?

// Current position of the device
static	float	g_xPos = (float)0.0;
static	float	g_yPos = (float)0.0;
static	float	g_zPos = (float)0.0;

// Center position for the current effect.
static	float	g_xCenter = g_xPos;
static	float	g_yCenter = g_yPos;
static	float	g_zCenter = g_zPos;

// Box side descriptions.  They are in the following order: -X. -Y. -Z. +X. +Y, +Z.
// They are offset by 2cm from the center.  Their characteristics match those
// found in the button-press description message.
static	BoxSide	g_box[6] = {  BoxSide(-0.02, 0.00, 0.00,  1, 0, 0,  -1, 0, 0),
			      BoxSide( 0.00,-0.02, 0.00,  0, 1, 0,   0,-1, 0),
			      BoxSide( 0.00, 0.00,-0.02,  0, 0, 1,   0, 0,-1),
			      BoxSide( 0.02, 0.00, 0.00, -1, 0, 0,   1, 0, 0),
			      BoxSide( 0.00, 0.02, 0.00,  0,-1, 0,   0, 1, 0),
			      BoxSide( 0.00, 0.00, 0.02,  0, 0,-1,   0, 0, 1) };

// Stiffness of the point, line and plane constraint
static	float g_pointConstraintStiffness = (float)100.0;
static  float g_lineConstraintStiffness = (float) 300.0;
static  float g_planeConstraintStiffness = (float) 500.0;

// Strength and variability of the force field
static	float g_forceFieldStrength = (float)0.0;
static	float g_forceFieldVaries = (float)20.0;
static	float g_forceFieldRadius = (float)0.3;

/*****************************************************************************
 *
   Callback handlers
 *
 *****************************************************************************/

void    VRPN_CALLBACK handle_force_change(void *, const vrpn_FORCECB f)
{
  static vrpn_FORCECB lr;        // last report
  static int first_report_done = 0;

  if ((!first_report_done) ||
    ((f.force[0] != lr.force[0]) || (f.force[1] != lr.force[1])
      || (f.force[2] != lr.force[2]))) {
    printf("force is (%f,%f,%f)\n", f.force[0], f.force[1], f.force[2]);
  }

  first_report_done = 1;
  lr = f;
}


void    VRPN_CALLBACK handle_tracker_change(void *, const vrpn_TRACKERCB t)
{
  // Record the current position of the device in global variables
  // so that the button routine can know where to store the center
  // position.
  g_xPos = (float)t.pos[0];
  g_yPos = (float)t.pos[1];
  g_zPos = (float)t.pos[2];

  // This is the workhorse function for the program.

  // If we are not active, then don't do anything.
  if (!g_active) { return; }

  // Depending on the
  // current state of the application, it generates the appropriate
  // effect.

  switch (g_state) {
    case box:
      // We let the user feel around a box whose sides have different
      // characteristics as described in the print statements in the
      // button-press code.
      {
	// Figure out which side of the box to use.  The index we are computing
	// should have 0 = negative X from center, 1 = negative Y from center,
	// 2 = negative Z, 3 = +X, 4 = +Y, 5 = +Z.  This is determined based
	// on which wall we are closest to touching.
	double  dx = g_xPos - g_xCenter;
	double  dy = g_yPos - g_yCenter;
	double  dz = g_zPos - g_zCenter;

	double	magx = fabs(dx);
	double	magy = fabs(dy);
	double	magz = fabs(dz);

	double	magmax = magx;
	if (magy > magmax) { magmax = magy; }
	if (magz > magmax) { magmax = magz; }

	int index;

	if (magx == magmax) {
	  if (dx < 0) {
	    index = 0;      // X largest mag, cursor in -X from center
	  } else {
	    index = 3;      // X largest mag, cursor in +X from center
	  }
	}
	if (magy == magmax) {
	  if (dy < 0) {
	    index = 1;      // Y largest mag, cursor in -Y from center
	  } else {
	    index = 4;      // Y largest mag, cursor in +Y from center
	  }
	}
	if (magz == magmax) {
	  if (dz < 0) {
	    index = 2;      // Z largest mag, cursor in -Z from center
	  } else {
	    index = 5;      // Z largest mag, cursor in +Z from center
	  }
	}

	// Find the A,B,C coefficients for the plane, which are just
	// the normal pulled from the box side descriptor
	float a = g_box[index].d_nX;
	float b = g_box[index].d_nY;
	float c = g_box[index].d_nZ;

	// Find a point on the plane, one of which is located at the
	// box wall's offset from the center location.  We're going to
	// need this to solve for D in the plane equation.
	float ppx = g_xCenter + g_box[index].d_oX;
	float ppy = g_yCenter + g_box[index].d_oY;
	float ppz = g_zCenter + g_box[index].d_oZ;

	// Find the value of D in the plane equation that makes it zero
	// at the point on the plane we just found above: this requires
	// D = - (Ax + By + Cz) for that point
	float d = - ( a*ppx + b*ppy + c*ppz );

	// Set plane parameters.  First, the plane equation and then the
	// surface parameters.  We raise the multiplier to the power of
	// the box-description multiplier and then apply this to the surface
	// parameter to vary each for different sides.  A box parameter of
	// 0 will therefore cause no change (multiply by one), a parameter of
	// -1 will reduce by the factor and a parameter of 1 will increase
	// by the parameter.  NOTE: Static and dynamic friction are set equal.
	g_forceDevice->set_plane(a, b, c, d);
	float	spring = g_kSpring, springmult = g_box[index].d_sMult;
	if (springmult ==  1) { spring *= g_kSpringMult; }
	if (springmult == -1) { spring /= g_kSpringMult; }
	g_forceDevice->setSurfaceKspring(spring);
	float	damping = g_kDamping, dampingmult = g_box[index].d_dMult;
	if (dampingmult ==  1) { damping *= g_kDampingMult; }
	if (dampingmult == -1) { damping /= g_kDampingMult; }
	g_forceDevice->setSurfaceKdamping(damping);
	float	friction = g_sFric, frictionmult = g_box[index].d_fMult;
	if (frictionmult ==  1) { friction *= g_sFricMult; }
	if (frictionmult == -1) { friction /= g_sFricMult; }
	g_forceDevice->setSurfaceFstatic(friction);
	g_forceDevice->setSurfaceFdynamic(friction);

	// texture and buzzing stuff:
	// this turns off buzzing and texture
	g_forceDevice->setSurfaceBuzzAmplitude(0.0);
	g_forceDevice->setSurfaceBuzzFrequency(60.0); // Hz
	g_forceDevice->setSurfaceTextureAmplitude(0.00); // meters!!!
	g_forceDevice->setSurfaceTextureWavelength((float)0.01); // meters!!!

	g_forceDevice->setRecoveryTime(10);	// recovery occurs over 10
					  // force update cycles

	// enable force device and send surface.
	g_forceDevice->startSurface();
      }
      break;

    case pointconstraint:
      // We produce a point constraint that pulls the user back towards the place where
      // they pressed the button.  The strength of this constraint is stored in a global. 
      {
	vrpn_float32  center[3] = { g_xCenter, g_yCenter, g_zCenter };
	g_forceDevice->setConstraintMode(vrpn_ForceDevice::POINT_CONSTRAINT);
	g_forceDevice->setConstraintKSpring(g_pointConstraintStiffness);
	g_forceDevice->setConstraintPoint(center);
	g_forceDevice->enableConstraint(1);
      }
      break;

    case lineconstraint:
      // We produce a point constraint that pulls the user back towards the place where
      // they pressed the button.  The strength of this constraint is stored in a global. 
      {
	vrpn_float32  center[3] = { g_xCenter, g_yCenter, g_zCenter };
	vrpn_float32  direction[3] = { 0, 0, 1};
	g_forceDevice->setConstraintMode(vrpn_ForceDevice::LINE_CONSTRAINT);
	g_forceDevice->setConstraintKSpring(g_lineConstraintStiffness);
	g_forceDevice->setConstraintLinePoint(center);
	g_forceDevice->setConstraintLineDirection( direction );
	g_forceDevice->enableConstraint(1);
      }
      break;

    case planeconstraint:
      // We produce a point constraint that pulls the user back towards the place where
      // they pressed the button.  The strength of this constraint is stored in a global. 
      {
	vrpn_float32  center[3] = { g_xCenter, g_yCenter, g_zCenter };
	vrpn_float32  direction[3] = { 0, 0, 1};
	g_forceDevice->setConstraintMode(vrpn_ForceDevice::PLANE_CONSTRAINT);
	g_forceDevice->setConstraintKSpring(g_planeConstraintStiffness);
	g_forceDevice->setConstraintPlanePoint(center);
	g_forceDevice->setConstraintPlaneNormal( direction );
	g_forceDevice->enableConstraint(1);
      }
      break;
   
    case forcefield:
      // Produce a force field pointing in +Z, starting at the center position.  As the
      // user moves more in the +Z direction, the field will get smaller; larger as they
      // move in -Z.
      {
	vrpn_float32  center[3] = { g_xCenter, g_yCenter, g_zCenter };
	vrpn_float32  force[3] = { 0, 0, g_forceFieldStrength };
	vrpn_float32  jacobian[3][3] = { { 0, 0, 0 }, {0, 0, 0}, {g_forceFieldVaries, 0, 0} };
	g_forceDevice->sendForceField(center, force, jacobian, g_forceFieldRadius);
      }
      break;

    case buzzing:
      {
	// Black magic dredged up from vrpn_Phantom.C
	vrpn_float32  amplitude;    //< 
	vrpn_float32  frequency;    //< Frequency of the buzzing
	vrpn_float32  duration;	    //< Duration of the effect
	vrpn_float32  direction[3]; //< Normalized?
	vrpn_float32  params[6];    //< In the order shown above
	vrpn_int32    effectID = 0; //< 0 means "instant buzzing effect"

	// We only send this every half second; the duration of the effect is
	// 1/4 second.
	{ static  struct timeval  last_sent = {0,0};
	  struct  timeval now;
	  vrpn_gettimeofday(&now, NULL);
	  if (vrpn_TimevalMsecs(vrpn_TimevalDiff(now, last_sent)) < 500) { break; }
	  last_sent = now;
	}

	// See how far we are from the center in meters.
	vrpn_float32  offset[3] = { g_xPos - g_xCenter, g_yPos - g_yCenter, g_zPos - g_zCenter };

	// Adjust the amplitude based on motion in X
	amplitude = (float)(0.5 + 0.5 * (offset[0] / 0.2));
	if (amplitude < 0) { amplitude = 0; }

	// Adjust the frequency based on motion in Y
	frequency = (float)(200 + 200 * (offset[1] / 0.2));
	if (frequency < 1) { frequency = 1; }

	// Duration is 1/4 second, send twice per second!
	duration = (float)0.25;

	// Normalize the direction from the center and put it into direction.
	// Set the direction to +Z if the probe is at the center.
	vrpn_float32  length = (float)sqrt(offset[0]*offset[0] + offset[1]*offset[1] + offset[2]*offset[2]);
	if (length == 0) {
	  direction[0] = direction[1] = 0; direction[2] = 1.0;
	} else {
	  direction[0] = offset[0] / length;
	  direction[1] = offset[1] / length;
	  direction[2] = offset[2] / length;
	}

	params[0] = amplitude;
	params[1] = frequency;
	params[2] = duration;
	params[3] = direction[0];
	params[4] = direction[1];
	params[5] = direction[2];
	g_forceDevice->setCustomEffect(effectID, params, 6);
	g_forceDevice->startEffect();
      }
      break;

    case geometry:
      {
	// Only do this creation once!
	static bool first_time = true;
	if (first_time) {
	  first_time = false;
	} else {
	  break;
	}

	const vrpn_float32  halfwidth = static_cast<vrpn_float32>(0.02);
	vrpn_float32  left = g_xCenter - halfwidth;
	vrpn_float32  right = g_xCenter + halfwidth;
	vrpn_float32  top = g_yCenter - halfwidth;
	vrpn_float32  bottom = g_yCenter + halfwidth;
	vrpn_float32  front = g_zCenter - halfwidth;
	vrpn_float32  back = g_zCenter + halfwidth;

	// Create the cube object, rooted in the world.
	g_forceDevice->useGhost();
	g_forceDevice->addObject(CUBE_ID);

	// Create a set of points at the cube corners.
	const vrpn_int32 LBF = 0, RBF = 1, LTF = 2, RTF = 3,
			 LBB = 4, RBB = 5, LTB = 6, RTB = 7;
	g_forceDevice->setObjectVertex(CUBE_ID, LBF, left, bottom, front);
	g_forceDevice->setObjectVertex(CUBE_ID, RBF, right, bottom, front);
	g_forceDevice->setObjectVertex(CUBE_ID, LTF, left, top, front);
	g_forceDevice->setObjectVertex(CUBE_ID, RTF, right, top, front);
	g_forceDevice->setObjectVertex(CUBE_ID, LBB, left, bottom, back);
	g_forceDevice->setObjectVertex(CUBE_ID, RBB, right, bottom, back);
	g_forceDevice->setObjectVertex(CUBE_ID, LTB, left, top, back);
	g_forceDevice->setObjectVertex(CUBE_ID, RTB, right, top, back);

	// Create a set of triangles that are the cube faces.
	// REMEMBER to use the right-hand rule for creating of the
	// triangles: the normal points out of that face.

	// Front
	g_forceDevice->setObjectTriangle(CUBE_ID, 0, LBF, LTF, RBF);
	g_forceDevice->setObjectTriangle(CUBE_ID, 1, RBF, LTF, RTF);

	// Back
	g_forceDevice->setObjectTriangle(CUBE_ID, 2, LBB, RBB, LTB);
	g_forceDevice->setObjectTriangle(CUBE_ID, 3, RBB, RTB, LTB);

	// Left
	g_forceDevice->setObjectTriangle(CUBE_ID, 4, LBF, LBB, LTF);
	g_forceDevice->setObjectTriangle(CUBE_ID, 5, LTF, LBB, LTB);

	// Right
	g_forceDevice->setObjectTriangle(CUBE_ID, 6, RBF, RTF, RBB);
	g_forceDevice->setObjectTriangle(CUBE_ID, 7, RTF, RTB, RBB);

	// Top
	g_forceDevice->setObjectTriangle(CUBE_ID, 8, LTF, LTB, RTF);
	g_forceDevice->setObjectTriangle(CUBE_ID, 9, RTF, LTB, RTB);

	// Bottom
	g_forceDevice->setObjectTriangle(CUBE_ID, 10, LBF, RBF, LBB);
	g_forceDevice->setObjectTriangle(CUBE_ID, 11, RBF, RBB, LBB);

	// Push all of the changes and make them active.
	g_forceDevice->updateObjectTrimeshChanges(CUBE_ID);
      }
      break;

    default:
      break;
  };
}

void	VRPN_CALLBACK handle_button_change(void *, const vrpn_BUTTONCB b)
{
  // When the button is pressed, start things going for the state
  // we are in and tell that the forces are active.
  // When the button is released, switch to a new state and set things
  // as they need to be set for that state.  Also, tell that the
  // forces are inactive.
  if (b.state) {

    // Forces becoming active in the current state
    g_active = true;

    // Set the location at which the button was pressed based on
    // the last tracker position so that effects which depend on
    // it can know.
    g_xCenter = g_xPos;
    g_yCenter = g_yPos;
    g_zCenter = g_zPos;

    // Set things up to generate forces in the state we are in
    switch (g_state) {
      case box:
	printf("  -X wall will have low spring constant\n");
	printf("  +X wall will have high spring constant\n");
	printf("  -Y wall will have low friction\n");
	printf("  +Y wall will have high friction\n");
	printf("  -Z wall will have low damping\n");
	printf("  +Z wall will have high damping\n");
	printf("\n");
	printf("Release button to stop feeling inside box\n");
	break;

      case pointconstraint:
	printf("  You will be pulled back towards where you pressed the button\n");
	printf("\n");
	printf("Release button to stop point constraint\n");
	break;

      case lineconstraint: 
	printf("  You will be able to pull along a line in the z direction\n");
	printf("\n");
	printf("Release button to stop line constraint\n");
	break;
   
      case planeconstraint: 
	printf("  You will be able to pull along the x-y plane\n");
	printf("\n");
	printf("Release button to stop plane constraint\n");
	break;

      case forcefield:
	printf("  You will be pushed in Z while you are within the field\n");
	printf("  The push is in +Z when you move in +X, in -Z when you move in -X\n");
	printf("\n");
	printf("Release button to stop force field\n");
	break;

      case buzzing:
	printf("  Amplitude will increase as you move in +X, decrease as you move in -X\n");
	printf("  Frequency will increase as you move in +Y, decrease as you move in -Y\n");
	printf("  Direction will be towards where the button was pressed\n");
	printf("\n");
	printf("Release button to stop buzzing\n");
	break;

      case geometry:
	printf("  A cube 4cm on a side will be created, centered at your start location.\n");
	printf("\n");
	printf("Release button to stop feeling the cube\n");
	break;

      default:
	break;
    };
  } else {

    // Not doing any forces until button pressed again.
    g_active = false;

    // Move from the state we are in to the next state.
    switch (g_state) {
      case box:
	g_forceDevice->stopSurface();
	printf("\n");
	printf("Press button to start point constraint\n");
	g_state = pointconstraint;
	break;

      case pointconstraint:
	g_forceDevice->enableConstraint(0);
	printf("\n");
	printf("Press button to line constraint\n");
	g_state = lineconstraint;
	break;

      case lineconstraint:
	g_forceDevice->enableConstraint(0);
	printf("\n");
	printf("Press button to start plane constraint\n");
	g_state = planeconstraint;
	break;

      case planeconstraint:
	g_forceDevice->enableConstraint(0);
	printf("\n");
	printf("Press button to start force field\n");
	g_state = forcefield;
	break;

      case forcefield:
	g_forceDevice->stopForceField();
	printf("\n");
	printf("Press button to start buzzing\n");
	g_state = buzzing;
	break;

   
      case buzzing:
	g_forceDevice->stopEffect();
	printf("\n");
	printf("Press button to start geometry cube!\n");
	g_state = geometry;
	break;

      case geometry:
	g_forceDevice->removeObject(CUBE_ID);
	printf("\n");
	printf("Qutting program!\n");
	g_state = quit;
	break;

      default:
	g_state = quit;
	break;
    };
  }
}

int main(int argc, char *argv[])
{
  vrpn_Tracker_Remote *tracker;
  vrpn_Button_Remote *button;

  if (argc != 2) {
    printf("Usage: %s device_name\n",argv[0]);
    printf("   Example: %s Phantom@localhost\n",argv[0]);
    exit(-1);
  }

  char *device_name = argv[1];
  printf("Connecting to %s:\n",device_name);

  /* initialize the force device */
  g_forceDevice = new vrpn_ForceDevice_Remote(device_name);
  g_forceDevice->register_force_change_handler(NULL, handle_force_change);

  /* initialize the tracker */
  tracker = new vrpn_Tracker_Remote(device_name);
  tracker->register_change_handler(NULL, handle_tracker_change);

  /* initialize the button */
  button = new vrpn_Button_Remote(device_name);
  button->register_change_handler(NULL, handle_button_change);

  // Wait until we get connected to the server.
  while (!g_forceDevice->connectionPtr()->connected()) {
      g_forceDevice->mainloop();
  }

  printf("Press forceDevice's first button to begin feeling inside a box\n");

  // main loop
  while ( g_state != quit )
  {
    // Let tracker receive position information from remote tracker
    tracker->mainloop();

    // Let button receive button status from remote button
    button->mainloop();

    // Let the forceDevice send its planes to remote force device
    g_forceDevice->mainloop();
  }

  delete tracker;
  delete button;
  delete g_forceDevice;

  return 0;
}   /* main */

