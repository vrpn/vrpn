#include <stdio.h>
#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_ForceDevice.h"
#include "quat.h"

#include "glwin.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>


typedef struct {
  float x;    // position
  float y;
  float z;
} pos;

typedef struct {
  float fx;
  float fy; 
  float fz;
} force;

typedef struct {
  float x;
  float y;
  float z;
  float vx;
  float vy;
  float vz;
  float ax;
  float ay;
  float az;
  float mass;
  float charge;
} state;

void handle_tracker(void *userdata, const vrpn_TRACKERCB t) {
  pos *p = (pos *) userdata;

  p->x = -t.pos[0];
  p->y = t.pos[1];
  p->z = -t.pos[2];
}

void handle_force(void *userdata, const vrpn_FORCECB fcb) {
  force *f = (force *) userdata;

  f->fx = -fcb.force[0];
  f->fy = fcb.force[1];
  f->fz = -fcb.force[2];
}


void handle_button(void *userdata, const vrpn_BUTTONCB b) {

  switch (b.button) {
    case 0:
      *(int *)userdata = b.state;
      if (b.state == 1) printf("Force field enabled\n");
      else printf("Force field disabled\n");
      break;
    default:
      printf("Unknown button %3d was pressed\n",b.button);
  }
  fflush(stdout);  
}

void init_graphics(void *myglwin)
{
  // allw the window to open and do its stuff once
  glwin_handle_events(myglwin);

  // setup OpenGL state...
  glShadeModel(GL_SMOOTH);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0, 1.0, 0.01, 10.0);
  gluLookAt( 0.0,  0.1, -5.0,     // eye pos
             0.0,  0.0,  0.0,     // look at this point
             0.0,  1.0,  0.0);    // up direction 
  glClearColor(0.0, 0.0, 0.0, 0.0);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);

  //
  // Setup cute lighting parms..
  //
  glEnable(GL_LIGHTING);
  GLfloat lpos[] = { 0.0,  0.0, -100.0,  1.0};
  GLfloat lamb[] = { 0.05, 0.05,   0.05, 1.0};
  GLfloat ldif[] = { 0.5,  0.5,    0.5,  1.0};
  GLfloat lspc[] = { 0.5,  0.5,    0.5,  1.0};

  glLightfv(GL_LIGHT0, GL_POSITION, lpos);
  glLightfv(GL_LIGHT0, GL_AMBIENT,  lamb);
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  ldif);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lspc);
  glLightf(GL_LIGHT0, GL_SHININESS, 400.0);
  glEnable(GL_LIGHT0);

  GLfloat mdif[] = {0.9,  0.0,  0.2, 1.0};
  GLfloat mspc[] = {0.2, 0.2, 0.2, 1.0};
  glMaterialfv(GL_FRONT, GL_DIFFUSE,  mdif);
  glMaterialfv(GL_FRONT, GL_SPECULAR, mspc);
  glEnable(GL_COLOR_MATERIAL);
}

void draw_axes() {
     //
     // Draw Coordinate Axes
     //
     glDisable(GL_LIGHTING);
     glBegin(GL_LINES);
     float i;

     glColor3f(0.0, 1.0, 0.0);
     for (i=-10; i<10; i += 0.50) {
       glVertex3f( -10.0,     0.0,        i);
       glVertex3f(  10.0,     0.0,        i);
       glVertex3f(     i,     0.0,    -10.0);
       glVertex3f(     i,     0.0,     10.0);
     }
     for (i=-10; i<10; i += 0.50) {
       glVertex3f( -10.0,     1.0,        i);
       glVertex3f(  10.0,     1.0,        i);
       glVertex3f(     i,     1.0,    -10.0);
       glVertex3f(     i,     1.0,     10.0);
     }
     glEnd();
} 

void draw_tracker_and_atom(force *p, GLUquadricObj *qobj,
                           state *atom, GLUquadricObj *qatom)
{
  float x, y, z;
  float atomx, atomy, atomz;
 

  // Convert from atom coordinates to scene coordinates
  atomx = atom->x;
  atomy = atom->y;
  atomz = atom->z;

  // Convert from haptic coordinates to our scene coordinates
  x=-(p->fx) + atomx;
  y=-(p->fy) + atomy;
  z=-(p->fz) + atomz;

  // First draw line from tracker to atom 
  glColor3f(1.0, 1.0, 1.0);
  glBegin(GL_LINES);
  glVertex3f(x, y, z); 
  glVertex3f(atomx, atomy, atomz);
  glEnd();


  glEnable(GL_LIGHTING);
  glColor3f(1.0, 0.0, 0.2);
  glPushMatrix();
    glTranslatef(x, y, z);
    gluSphere(qobj, 0.2, 32, 32);
  glPopMatrix();
  glColor3f(0.0, 1.0, 0.2);
  glPushMatrix();
    glTranslatef(atomx, atomy, atomz);
    gluSphere(qobj, 0.2, 32, 32);
  glPopMatrix();
}
void apply_bc(state *atom)
{
  const float floor_k = 20.0;
  const float maxdim = 1.0;
  const float nearclip = 3.99;
  const float farclip  = 5.0;

  if (atom->y < 0) atom->ay -= floor_k*atom->y/atom->mass;
  if (atom->y > maxdim) atom->ay -= floor_k*(atom->y-maxdim)/atom->mass;
 
  if (atom->x >  maxdim) atom->ax -= floor_k*(atom->x-maxdim)/atom->mass;
  if (atom->x < -maxdim) atom->ax -= floor_k*(atom->x+maxdim)/atom->mass;

  if (atom->z >  farclip) atom->az -= floor_k*(atom->z-farclip)/atom->mass;
  if (atom->z < -nearclip) atom->az -= floor_k*(atom->z+nearclip)/atom->mass;
}

void run_dynamics(state *atom, force *f)
{
  const int N=1;
  const float dt = 0.0004;

  for (int i=0; i < N; i++) {
    atom->x += dt*atom->vx + .5*dt*dt*atom->ax;
    atom->y += dt*atom->vy + .5*dt*dt*atom->ay;
    atom->z += dt*atom->vz + .5*dt*dt*atom->az;

    atom->vx += 0.5*dt*(atom->ax);
    atom->vy += 0.5*dt*(atom->ay);
    atom->vz += 0.5*dt*(atom->az);

    atom->ax  =  -f->fx/atom->mass;
    atom->ay  =  -f->fy/atom->mass;
    atom->az  =  -f->fz/atom->mass;

    apply_bc(atom);

    atom->vx += 0.5*dt*(atom->ax);
    atom->vy += 0.5*dt*(atom->ay);
    atom->vz += 0.5*dt*(atom->az);
  } 
}

int main(int argc, char **argv) {       
  if (argc < 2) {
    printf("%s: Invalid parms\n", argv[0]);
    printf("usage: \n");
    printf("  %s Tracker0@host\n", argv[0]);

    return -1;
  }
  char *server = argv[1];
  pos *phan_position = new pos;
  pos *phan_offset   = new pos;
  force *phan_force  = new force;
  state *atom_state  = new state;

  int ff_enabled, ff_active;
  double kspr = 500.0;  // Units???

  vrpn_Tracker_Remote *tkr;
  vrpn_Button_Remote *btn;
  vrpn_ForceDevice_Remote *fdv;

  printf("Opening: %s ...\n",  server);

  tkr = new vrpn_Tracker_Remote(server);
  tkr->register_change_handler(phan_position, handle_tracker);

  btn = new vrpn_Button_Remote(server);
  btn->register_change_handler(&ff_enabled, handle_button);

  fdv = new vrpn_ForceDevice_Remote(server);
  fdv->register_force_change_handler(phan_force, handle_force);

  void * myglwin = glwin_create(700, 700);
  if (myglwin == NULL) {
    printf("Failed to open OpenGL window!!\n");
    return -1;
  }
  atom_state->x = 0.0; atom_state->y = 0.0; atom_state->z = 0.0;
  atom_state->vx = 0.0; atom_state->vy = 0.0; atom_state->vz = 0.0;
  atom_state->ax = 0.0; atom_state->ay = 0.0; atom_state->az = 0.0;
  atom_state->mass = .001;

  ff_active = 0;
 
  init_graphics(myglwin);
  GLUquadricObj *qobj  = gluNewQuadric();
  GLUquadricObj *qatom = gluNewQuadric();

  /* 
   * main interactive loop
   */
  while (1) {
    // Let the tracker do its thing
    tkr->mainloop();
    btn->mainloop();
    fdv->mainloop();

    run_dynamics(atom_state, phan_force);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (ff_enabled ) {
      if (!ff_active) {
	// Set up force field so initially the force is zero
	phan_offset->x = phan_position->x - .2*atom_state->x;
	phan_offset->y = phan_position->y - .2*atom_state->y;
	phan_offset->z = phan_position->z - .2*atom_state->z;
      }

      // Now turn the force field on
      // scene -> haptic: rotate 180 about the y axis
      fdv->setConstraintMode(vrpn_ForceDevice::POINT_CONSTRAINT);
      float cpos[3];
      cpos[0] = -(.2*atom_state->x + phan_offset->x);
      cpos[1] =  (.2*atom_state->y + phan_offset->y);
      cpos[2] = -(.2*atom_state->z + phan_offset->z);
      fdv->setConstraintPoint(cpos);
      fdv->setConstraintKSpring(kspr);
      fdv->enableConstraint(1); // enable force field


      ff_active = 1;
    }
    else if (ff_active) {
      fdv->enableConstraint(0); // disable force field
      ff_active = 0;
    }        

    draw_axes();

    draw_tracker_and_atom(phan_force, qobj, atom_state, qatom);

    glwin_swap_buffers(myglwin);
  }
}   /* main */
