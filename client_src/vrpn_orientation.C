///////////////////////////////////////////////////////////////////////////////////////////////
//
// Name:        vrpn_orientation.C
//
// Authors:     Gustaf Hendeby, Dmitry Mastykin
//
// Description: VRPN client demo for Trivisio Colibri device (based on freeglut3).
//              Modified by Russ Taylor to work for other trackers as well.
//
///////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#ifdef __APPLE__
#include <GLUT/glut.h>
#include <OpenGL/OpenGL.h>
#else
#include <GL/glut.h>                    // for glutCreateWindow, etc   // IWYU pragma: keep
#include <GL/gl.h>                      // for glClear, glClearColor, etc
#endif
#include <cstring>
#include <vector>
#include <cstdlib>

#include <vrpn_Tracker.h>        // for vrpn_TRACKERACCCB, etc
#include <quat/quat.h>

int width = 300;
int height = 300;
bool withNum = true;
bool withCube = true;
int sensors=0;
std::vector<vrpn_TRACKERCB> dataset;
vrpn_Tracker_Remote* tkr = NULL;

void display(void)
{
	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3d(0, 0, 0);
	for (int i=0; i<sensors; ++i) {
		glPushMatrix();
		glLoadIdentity();
		glColor3f(0, 0, 0);
		glTranslated(20*i, 0, 0);
		glRasterPos2f(-2.5, 6.5);
		const char* name = "Sensor#";
		for (size_t j = 0; j<8; ++j)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, name[j]);
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, '0'+i);
		glPopMatrix();

		vrpn_TRACKERCB* data = &dataset[i];

		glPushMatrix();
		glTranslated(0., -20*i, 0);
		if(withCube) {
			glPushMatrix();
            {
                qogl_matrix_type matrix;
                q_to_ogl_matrix(matrix, data->quat);
                glMultMatrixd(matrix);
            }

			float s = 3.;
			float w = 3.0 * s;
			float d = 3.0 * s;
			float h = 1.4 * s;

			glTranslated(-0.5*w, -0.5*d, -0.5*h);
			// X-surface
			glColor3d(.5, 0, 0);
			glBegin(GL_POLYGON);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(0, 0, h);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(0, d, h);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(0, d, 0);
			glNormal3d(-1, 0, 0);
			glEnd();

			glColor3d(1, 0, 0);
			glBegin(GL_POLYGON);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(w, 0, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(w, 0, h);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(w, d, h);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(w, d, 0);
			glNormal3d(1, 0, 0);
			glEnd();

			// Y-surface
			glColor3d(0, .5, 0);
			glBegin(GL_POLYGON);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(w, 0, 0);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(w, 0, h);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(0, 0, h);
			glNormal3d(0, -1, 0);
			glEnd();

			glColor3d(0, 1, 0);
			glBegin(GL_POLYGON);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, d, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(w, d, 0);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(w, d, h);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(0, d, h);
			glNormal3d(0, 1, 0);
			glEnd();

			// Z-surface
			glColor3d(0, 0, .5);
			glBegin(GL_POLYGON);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, 0);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(0, d, 0);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(w, d, 0);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(w, 0, 0);
			glNormal3d(0, 0, -1);
			glEnd();

			glColor3d(0, 0, 1);
			glBegin(GL_POLYGON);
			glTexCoord2f(0.0f, 0.0f);
			glVertex3d(0, 0, h);
			glTexCoord2f(1.0f, 0.0f);
			glVertex3d(w, 0, h);
			glTexCoord2f(1.0f, 1.0f);
			glVertex3d(w, d, h);
			glTexCoord2f(0.0f, 1.0f);
			glVertex3d(0, d, h);
			glNormal3d(0, 0, 1);
			glEnd();

			glPopMatrix();
		}
		if(withNum) {
			q_vec_type eul;
			q_to_euler(eul, data->quat);

			glPushMatrix();
			glLoadIdentity();
			glTranslated(20*i, 0, 0);
			std::string e1, e2, e3;
			{
				std::stringstream foo;
				foo.setf(std::ios::fixed, std::ios::floatfield);
				foo.setf(std::ios::right, std::ios::adjustfield);
				foo.precision(1);
				foo.width(6);
				foo << "  Yaw: ";
				foo.width(7);
				float tmp = std::fmod(-eul[0], 2*Q_PI);
				if (tmp<0)
					tmp += 2*Q_PI;
				foo << tmp*180/Q_PI;
				e1 = foo.str();
			}
			{
				std::stringstream foo;
				foo.setf(std::ios::fixed, std::ios::floatfield);
				foo.setf(std::ios::right, std::ios::adjustfield);
				foo.precision(1);
				foo.width(6);
				foo << "Pitch: ";
				foo.width(7);
				foo << -eul[1]*180/Q_PI;
				e2 = foo.str();
			}
			{
				std::stringstream foo;
				foo.setf(std::ios::fixed, std::ios::floatfield);
				foo.setf(std::ios::right, std::ios::adjustfield);
				foo.precision(1);
				foo.width(6);
				foo << " Roll: ";
				foo.width(7);
				foo <<  eul[2]*180/Q_PI;
				e3 = foo.str();
			}
			glColor3f(0, 0, 0);
			glRasterPos2f(-10, -6.8);
			for (size_t j = 0; j<e1.length(); ++j)
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, e1[j]);
			glRasterPos2f(-10, -8.3);
			for (size_t j = 0; j<e2.length(); ++j)
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, e2[j]);
			glRasterPos2f(-10, -9.8);
			for (size_t j = 0; j<e3.length(); ++j)
				glutBitmapCharacter(GLUT_BITMAP_9_BY_15, e3[j]);

			glPopMatrix();
		}
		glPopMatrix();
	}
	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y)
{
	switch(key) {
	case 'n':
		withNum = !withNum;
		break;
	case 'c':
		withCube = !withCube;
		break;
	case 'q':
		delete tkr;
		std::exit(0);
	}
}

void idle(void)
{
	tkr->mainloop();
    vrpn_SleepMsecs(20);
    glutPostRedisplay();
}

void VRPN_CALLBACK
handle_tracker_pos_quat(void *userdata, const vrpn_TRACKERCB t)
{
    // Make sure we have a count value for this sensor
    if (sensors <= t.sensor) {
        sensors = t.sensor + 1;
		dataset.resize(sensors);
	}

	dataset[t.sensor] = t;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " TRACKER_NAME@HOST" << std::endl;
        return -1;
    }

	std::cout << std::endl << std::endl;
	std::cout << "***************************************" << std::endl
			  << "*  Attempting to connect to Tracker " << argv[1] << std::endl
			  << "***************************************" << std::endl
			  << std::endl;
	
	tkr = new vrpn_Tracker_Remote(argv[1]);
	if (tkr == NULL) {
		fprintf(stderr, "Error opening %s\n", argv[1]);
		return -1;
	}
	// Set up the tracker callback handlers
	tkr->register_change_handler(NULL, handle_tracker_pos_quat);
    do {
        tkr->mainloop();
        vrpn_SleepMsecs(1);
    } while (sensors == 0);

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(width*sensors, height);
	glutCreateWindow("Tracker Multi-Orientation");

	glViewport(0, 0, width*sensors, height);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glOrtho(-10.0, -10.+sensors*20.0, -12.0, 8.0, -100, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(-10, 0, 0, 0, 0, 0, 0, 0, 1);

	glClearColor(1, 1, 1, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLineWidth(10);

	glutDisplayFunc(display);
	glutIdleFunc(idle);
	glutKeyboardFunc(keyboard);

	glutMainLoop();

	return 0;
}

