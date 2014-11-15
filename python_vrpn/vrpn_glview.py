#!/usr/bin/python

#
# Small program to examine positional data from a VRPN tracker
#
# Requirements:
#    apt-get install python-qt python-qt-gl
#

import sys
import math
import time
import optparse

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4.QtOpenGL import *

from OpenGL.GL import *
from OpenGL.GLU import *

sys.path.append(".")

import vrpn_Tracker

VERBOSE = False

#
class GLCamera:
    def __init__(self):
        self.angle = 0
        self.radius = 10
        self.height = 2
        pass

    def gluLookAt(self):
        x = self.radius * math.sin(self.angle)
        y = self.height
        z = self.radius * math.cos(self.angle)
        gluLookAt(x, y, z,
                  0.0, 0.0, 0.0,
                  0.0, 1.0, 0.0)
        pass

    pass

#
def convert_to_glmatrix(pos, quat):
    m = [1,0,0,0,
         0,1,0,0,
         0,0,1,0,
         0,0,0,1]

    p4_2 = quat[0]*2;  p5_2 = quat[1]*2;  p6_2 = quat[2]*2;
    xx = p4_2*quat[0]; xy = p4_2*quat[1]; xz = p4_2*quat[2];
    yy = p5_2*quat[1]; yz = p5_2*quat[2];
    zz = p6_2*quat[2];

    sx = quat[3]*p4_2; sy = quat[3]*p5_2; sz = quat[3]*p6_2;

    m[0] =1.0-(yy+zz);  m[4] =    (xy-sz);  m[8] =    (xz+sy);
    m[1] =    (xy+sz);  m[5] =1.0-(xx+zz);  m[9] =    (yz-sx);
    m[2] =    (xz-sy);  m[6] =    (yz+sx);  m[10]=1.0-(xx+yy);
    m[3] =0          ;  m[7] =0          ;  m[11]=0          ;

    m[12] = pos[0]
    m[13] = pos[1]
    m[14] = pos[2]
    m[15] = 1.0
    return m

#
def draw_grid(scale):
    glLineWidth(1)
    #glColor4f(0.9, 0.9, 0.9, 0.9)
    glBegin(GL_LINES)
    s = scale
    for i in range(-5,6):
        glVertex3f(i, 0,  s)
        glVertex3f(i, 0, -s)
        pass
    for i in range(-5,6):
        glVertex3f( s, 0, i)
        glVertex3f(-s, 0, i)
        pass
    glEnd()
    pass

#
def draw_axes(scale):
    s = scale
    glLineWidth(2)
    glBegin(GL_LINES)
    glColor3f(1,0,0) # x - r
    glVertex3f(0,0,0)
    glVertex3f(s,0,0)
    glColor3f(0,1,0) # y - g
    glVertex3f(0,0,0)
    glVertex3f(0,s,0)
    glColor3f(0.2,0.2,1) # z - b
    glVertex3f(0,0,0)
    glVertex3f(0,0,s)
    glEnd()
    pass

#
class ViewWidget3D(QGLWidget):
    def __init__(self, parent):
        QGLWidget.__init__(self, parent)
        self.setFocusPolicy(Qt.StrongFocus)
        self.lastmousepos = None
        self.camera = GLCamera()
        self.quadric = gluNewQuadric()
        self.mutex = QMutex()
        self.points = {}
        pass

    def initializeGL(self):
        QGLWidget.initializeGL(self)
        glClearColor(0,0,0,1)
        pass

    def resizeGL(self, w, h):
        glViewport(0, 0, w, h)
        pass

    def paintGL(self):
        QGLWidget.paintGL(self)

        # erase previous frame
        glClear(GL_COLOR_BUFFER_BIT)

        # setup matrices
        glMatrixMode(GL_PROJECTION)
        glLoadIdentity()
        gluPerspective(45, self.width()/float(self.height()), 1, 100)

        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        self.camera.gluLookAt()

        # grid
        glPushMatrix()
        glColor4f(1,1,1,1)
        draw_grid(5)
        glPopMatrix()

        # axes
        glPushMatrix()
        draw_axes(1)
        glPopMatrix()

        # points
        radius = 0.05
        self.mutex.lock()
        glColor4f(1,1,1,1)
        for id in self.points:
            p = self.points[id]
            try: pos, quat = p[0:3], p[3:]
            except: pass
            m = convert_to_glmatrix(pos, quat)
            glPushMatrix()
            glMultMatrixf(m)
            gluSphere(self.quadric, radius, 4,4)
            self.renderText(radius, radius, radius, "%d"%(id,))
            draw_axes(0.1)
            glPopMatrix()
            pass
        self.mutex.unlock()
        pass

    def mouseMoveEvent(self, event):
        if self.lastmousepos == None:
            self.lastmousepos = QVector2D(event.x(), event.y())
            return
        newpos = QVector2D(event.x(), event.y())
        dt = newpos - self.lastmousepos
        self.lastmousepos = newpos

        self.camera.angle -= dt.x() / self.width() * 2
        self.camera.height -= dt.y() / self.height() * 2

        self.update()
        pass
    def mousePressEvent(self, event):
        pass
    def mouseReleaseEvent(self, event):
        self.lastmousepos = None
        pass
    def wheelEvent(self, event):
        if(event.delta() > 0):
            self.camera.radius *= 0.9
        else:
            self.camera.radius *= 1.1
            pass
        self.update()
        pass
    def keyReleaseEvent(self, event):
        if event.text() == 'q':
            pass
        self.update()
        pass
    def handler(self, userdata, t):
        if VERBOSE:
            print userdata, t
            pass
        self.mutex.lock()
        try:
            self.points[t[0]] = t[1:]
            pass
        except:
            print "unknown data: ", t
            pass
        self.mutex.unlock()
        self.update()
        pass
    pass

#
class mainthread(QThread):
    def __init__(self, tracker, widget):
        QThread.__init__(self)
        self.tracker = tracker
        self.widget = widget
        pass
    def run(self):
        while True:
            self.tracker.mainloop()
            time.sleep(0)
            pass
        pass
    pass

#
def main(argv):
    global VERBOSE

    # parse command line
    parser = optparse.OptionParser()
    parser.add_option("-v", "--verbose", dest="verbose", default=False, action="store_true")
    options, args = parser.parse_args()

    if len(args) < 1:
        print "no tracker specified (Example: Tracker0@localhost)"
        return
    print "using tracker: ", args[0]

    if options.verbose:
        print "verbose mode"
        VERBOSE = True
        pass
    # create QT widgets
    app = QApplication(argv)
    mw = QMainWindow()
    widget = ViewWidget3D(mw)
    mw.setCentralWidget(widget)
    mw.resize(800,600)
    mw.show()

    # create VRPN tracker and register change handlers
    vrpn_Tracker.register_tracker_change_handler(widget.handler)
    tracker = vrpn_Tracker.vrpn_Tracker_Remote(argv[1])
    tracker.register_change_handler(None, vrpn_Tracker.get_tracker_change_handler())

    # start main thread
    mt = mainthread(tracker,widget)
    mt.start()

    # start main app
    app.exec_()
    pass

main(sys.argv)
