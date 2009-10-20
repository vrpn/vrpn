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
  double qx, qy, qz, qw;  // quaternion orientation
} posquat;

typedef struct {
  int b[100];
} buttons;

void handle_tracker(void *userdata, const vrpn_TRACKERCB t) {
  posquat *pq = (posquat *) userdata;

  pq->x = t.pos[0];
  pq->y = t.pos[1];
  pq->z = t.pos[2];
  pq->qx=t.quat[Q_X];
  pq->qy=t.quat[Q_Y];
  pq->qz=t.quat[Q_Z];
  pq->qw=t.quat[Q_W];
}

void handle_button(void *userdata, const vrpn_BUTTONCB b) {
  buttons *bt = (buttons *) userdata;

  if (b.button < 100) 
    bt->b[b.button] = b.state;
  printf("\nButton %3d is in state: %d                      \n", 
         b.button, b.state);
  fflush(stdout);  
}

void handle_force(void *userdata, const vrpn_FORCECB f) {
#if 0
  static vrpn_FORCECB lr;        // last report
  static int first_report_done = 0;

  if ((!first_report_done) || ((f.force[0] != lr.force[0]) || 
      (f.force[1] != lr.force[1]) || (f.force[2] != lr.force[2])))
    printf("\nForce is (%11f, %11f, %11f)                  \n", 
           f.force[0], f.force[1], f.force[2]);

  first_report_done = 1;
  lr = f;
#endif
}

void init_graphics(void *myglwin) {
  glwin_handle_events(myglwin); // let window to open and handle events once
  glwin_handle_events(myglwin); // let window to open and handle events once

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
  glEnable(GL_LEQUAL);

  GLfloat lpos[] = { 0.0,  0.0, -100.0,  1.0};
  GLfloat lamb[] = { 0.05, 0.05,  0.05, 1.0};
  GLfloat ldif[] = { 0.5,  0.5,   0.5,  1.0};
  GLfloat lspc[] = { 0.5,  0.5,   0.5,  1.0};

  glLightfv(GL_LIGHT0, GL_POSITION, lpos);
  glLightfv(GL_LIGHT0, GL_AMBIENT,  lamb);
  glLightfv(GL_LIGHT0, GL_DIFFUSE,  ldif);
  glLightfv(GL_LIGHT0, GL_SPECULAR, lspc);
  glLightf(GL_LIGHT0, GL_SHININESS, 400.0);
  glEnable(GL_LIGHT0);

  GLfloat mdif[] = {0.9,  0.0,  0.2, 1.0};
  GLfloat mspc[] = {0.2, 0.2, 0.2, 1.0};
  glMaterialfv(GL_FRONT, GL_SPECULAR, mspc);
  glMaterialfv(GL_FRONT, GL_DIFFUSE,  mdif);
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
     glEnd();
} 

void draw_tracker(posquat *pq, GLUquadricObj *qobj1, GLUquadricObj *qobj2)
{
  float x, y, z;
  
  // Convert from haptic coordinates to our scene coordinates
  x=-10.0*(pq->x);
  y=10.0*(pq->y);
  z=-10.0*(pq->z);
  double q[4] = {-(pq->qx), (pq->qy), -(pq->qz), pq->qw};
  // convert the orientation from quaternion to OpenGL matrix
  double rotation[16];
  q_to_ogl_matrix(rotation,q);


  // Draw the tracker itself
  //

  // First draw line to object
  glColor3f(1.0, 1.0, 1.0);
  glBegin(GL_LINES);
  glVertex3f(0.0,    0.0,    0.0   );
  glVertex3f(x, y, z); 
  glEnd();


  glEnable(GL_LIGHTING);
  glColor3f(1.0, 0.0, 0.2);
  glPushMatrix();
    glTranslatef(x, y, z);
    glMultMatrixd(rotation);
    glTranslatef(-1.5,  0.0,  0.0);
    gluSphere(qobj1, 0.2, 32, 32);
    glTranslatef(3.0,0.0,0.0);
    gluSphere(qobj2, 0.2, 32, 32);
  glPopMatrix();
}

int main(int argc, char **argv) {       
  char * server;
  posquat *tposquat = new posquat;
  buttons  bpos;

  vrpn_Tracker_Remote *tkr;
  vrpn_Button_Remote *btn;
  vrpn_ForceDevice_Remote *fdv;

  if (argc < 2) {
    printf("%s: Invalid parms\n", argv[0]);
    printf("usage: \n");
    printf("  %s Tracker0@host\n", argv[0]);

    return -1;
  }
  server = argv[1];


  printf("Opening: %s ...\n",  server);
  tkr = new vrpn_Tracker_Remote(server);
  tkr->register_change_handler(tposquat, handle_tracker);

  btn = new vrpn_Button_Remote(server);
  btn->register_change_handler(&bpos, handle_button);

  fdv = new vrpn_ForceDevice_Remote(server);
  fdv->register_force_change_handler(NULL, handle_force);

  void * myglwin = glwin_create(700, 700);
  if (myglwin == NULL) {
    printf("Failed to open OpenGL window!!\n");
    return -1;
  }

  init_graphics(myglwin);
  GLUquadricObj *qobj1 = gluNewQuadric();
  GLUquadricObj *qobj2 = gluNewQuadric();

  /* 
   * main interactive loop
   */
   while (1) {
     // Let the tracker do its thing
     tkr->mainloop();
     btn->mainloop();
     fdv->mainloop();

     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

     draw_axes();

     draw_tracker(tposquat, qobj1, qobj2);

     glwin_swap_buffers(myglwin);
   }
}   /* main */
