/*
 * glwin.c -- simple code for opening an OpenGL display window, and doing
 *            BitBlt operations or whatever...
 *
 */
 
#include <stdio.h>
#include <stdlib.h>

#ifdef USEOPENGL

#if !defined(WIN32) && !defined(_MSC_VER)

#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "glwin.h"

typedef struct {
  int scrnum;
  Display *dpy;
  Window root;
  Window win;
  GLXContext ctx; 
  int width;
  int height;
} oglhandle;


void * glwin_create(int width, int height) {
  oglhandle * handle; 
  XSetWindowAttributes attr;
  unsigned long mask;
  XVisualInfo *visinfo;
  int attrib[] = { GLX_RGBA, GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, 
                   GLX_BLUE_SIZE, 1, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None };

  handle = (oglhandle *) malloc(sizeof(oglhandle));
  if (handle == NULL) {
    printf("failed to allocate handle\n");
    return NULL;
  }

  handle->width = width;
  handle->height = height;

  handle->dpy = XOpenDisplay(getenv("DISPLAY"));
  if (handle->dpy == NULL) {
    printf("failed to open Display!!\n");
    free(handle);
    return NULL;
  } 

  handle->scrnum = DefaultScreen( handle->dpy );
  handle->root = RootWindow( handle->dpy, handle->scrnum );

  visinfo = glXChooseVisual( handle->dpy, handle->scrnum, attrib );

  if (visinfo == NULL) {
    printf("Error: couldn't get an RGB, Double-buffered visual\n");
    free(handle);
    return NULL;
  }

  /* window attributes */
  attr.background_pixel = 0;
  attr.border_pixel = 0;
  attr.colormap = XCreateColormap(handle->dpy, handle->root, 
                                  visinfo->visual, AllocNone);

  attr.event_mask = StructureNotifyMask | ExposureMask;
  mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

  handle->win = XCreateWindow(handle->dpy, handle->root, 0, 0, width, height,
                              0, visinfo->depth, InputOutput,
                              visinfo->visual, mask, &attr );

  handle->ctx = glXCreateContext( handle->dpy, visinfo, NULL, True );

  glXMakeCurrent( handle->dpy, handle->win, handle->ctx );

  XStoreName(handle->dpy, handle->win, 
             "OpenGL Window");

  XMapWindow(handle->dpy, handle->win);

  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glwin_swap_buffers(handle);
  glClear(GL_COLOR_BUFFER_BIT);
  glwin_swap_buffers(handle);

  glwin_handle_events(handle);
  glwin_handle_events(handle);

  XFlush(handle->dpy);

  return handle;
}


void glwin_destroy(void * voidhandle) {
  oglhandle * handle = (oglhandle *) voidhandle;

  if (handle == NULL)
    return;

  XUnmapWindow(handle->dpy, handle->win);

  glXMakeCurrent(handle->dpy, None, NULL);

  XDestroyWindow(handle->dpy, handle->win);

  XCloseDisplay(handle->dpy); 
}
 
void glwin_swap_buffers(void * voidhandle) {
  oglhandle * handle = (oglhandle *) voidhandle;

  if (handle != NULL)
    glXSwapBuffers(handle->dpy, handle->win);
}

int glwin_handle_events(void * voidhandle) {
  oglhandle * handle = (oglhandle *) voidhandle;
  XEvent event;

  if (handle == NULL)
    return 0;

  XNextEvent(handle->dpy, &event);

  switch(event.type) {
    case ConfigureNotify:
     handle->width = event.xconfigure.width;
     handle->height = event.xconfigure.height;

     glShadeModel(GL_FLAT);
     glViewport(0, 0, handle->width, handle->height);
     glMatrixMode(GL_PROJECTION);
     glLoadIdentity();

     /* this is upside-down for what most code thinks, but it is */
     /* right side up for the ray tracer..                       */
     gluOrtho2D(0.0, handle->width, 0.0, handle->height);
   
     glMatrixMode(GL_MODELVIEW);
     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
     glPixelZoom(1.0, 1.0); /* for upside-down images that ray uses */

     return 1;
  }

  return 0;
} 

void glwin_draw_image(void * voidhandle, int xsize, int ysize, 
                      unsigned char * img) {
  glRasterPos2i(0, 0);
  glDrawPixels(xsize, ysize, GL_RGB, GL_UNSIGNED_BYTE, img);
  glwin_swap_buffers(voidhandle);
}


#else


/* 
 *  WIN32 Version 
 */

#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "glwin.h"

typedef struct {
  HWND hWnd;
  HDC hDC;
  HGLRC hRC;
  long scrwidth;
  long scrheight;
  int width;
  int height;
} oglhandle;

static char szAppName[] = "OpenGL";
static char szAppTitle[]="OpenGL Window";

                                /* XXX GET RID OF THIS!!!!! */
static oglhandle * globhandle;  /* XXX GET RID OF THIS!!!!! */
                                /* XXX GET RID OF THIS!!!!! */

/*
 * declaration of myWindowProc()
 */
LONG WINAPI myWindowProc( HWND, UINT, WPARAM, LPARAM );

static int OpenWin32Connection(oglhandle * handle) {
  WNDCLASS  wc;
  HINSTANCE hInstance = GetModuleHandle(NULL);

  /* Clear (important!) and then fill in the window class structure. */
  memset(&wc, 0, sizeof(WNDCLASS));
  wc.style         = CS_OWNDC;
  wc.lpfnWndProc   = (WNDPROC) myWindowProc;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
  wc.hCursor       = LoadCursor(hInstance, IDC_ARROW);
  wc.hbrBackground = NULL; /* Default color */
  wc.lpszMenuName  = NULL;
  wc.lpszClassName = szAppName;

  if(!RegisterClass(&wc)) {
    printf("Cannot register window class.\n");
    return -1;
  }

  handle->scrwidth  = GetSystemMetrics(SM_CXSCREEN);
  handle->scrheight = GetSystemMetrics(SM_CYSCREEN); 

  return 0;
}

static HGLRC SetupOpenGL(HWND hWnd) {
  int nMyPixelFormatID;
  HDC hDC;
  HGLRC hRC;
  static PIXELFORMATDESCRIPTOR pfd = {
        sizeof (PIXELFORMATDESCRIPTOR), /* struct size      */
        1,                              /* Version number   */
        PFD_DRAW_TO_WINDOW      /* Flags, draw to a window, */
          | PFD_SUPPORT_OPENGL, /* use OpenGL               */
        PFD_TYPE_RGBA,          /* RGBA pixel values        */
        24,                     /* 24-bit color             */
        0, 0, 0,                /* RGB bits & shift sizes.  */
        0, 0, 0,                /* Don't care about them    */
        0, 0,                   /* No alpha buffer info     */
        0, 0, 0, 0, 0,          /* No accumulation buffer   */
        32,                     /* 32-bit depth buffer      */
        0,                      /* No stencil buffer        */
        0,                      /* No auxiliary buffers     */
        PFD_MAIN_PLANE,         /* Layer type               */
        0,                      /* Reserved (must be 0)     */
        0,                      /* No layer mask            */
        0,                      /* No visible mask          */
        0                       /* No damage mask           */
  };

  hDC = GetDC(hWnd);
  nMyPixelFormatID = ChoosePixelFormat(hDC, &pfd);

  /* 
   * catch errors here.
   * If nMyPixelFormat is zero, then there's
   * something wrong... most likely the window's
   * style bits are incorrect (in CreateWindow() )
   * or OpenGL isn't installed on this machine
   *
   */

  if (nMyPixelFormatID == 0) {
    printf("Error selecting OpenGL Pixel Format!!\n");
    return NULL;
  }

  SetPixelFormat( hDC, nMyPixelFormatID, &pfd );

  hRC = wglCreateContext(hDC);
  ReleaseDC(hWnd, hDC);

  return hRC;
}

static int myCreateWindow(oglhandle * handle, 
                          int xpos, int ypos, int xs, int ys) {
  /* Create a main window for this application instance. */
  handle->hWnd = 
        CreateWindow(
              szAppName,          /* app name */
              szAppTitle,         /* Text for window title bar */
              WS_OVERLAPPEDWINDOW /* Window style */
               | WS_CLIPCHILDREN
               | WS_CLIPSIBLINGS, /* NEED THESE for OpenGL calls to work! */
              xpos, ypos,
              xs, ys,
              NULL,                  /* no parent window                */
              NULL,                  /* Use the window class menu.      */
              GetModuleHandle(NULL), /* This instance owns this window  */
              handle                 /* We don't use any extra data     */
        );

  if (!handle->hWnd) {
    printf("Couldn't Open Window!!\n");
    return -1;
  }

  handle->hDC = GetDC(handle->hWnd);
  wglMakeCurrent(handle->hDC, handle->hRC);

  /* Make the window visible & update its client area */
  ShowWindow( handle->hWnd, SW_SHOW);  /* Show the window         */
  UpdateWindow( handle->hWnd );        /* Sends WM_PAINT message  */

  return 0;
}


LONG WINAPI myWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  PAINTSTRUCT   ps; /* Paint structure. */

  switch(msg) {
    case WM_CREATE:
      globhandle->hRC = SetupOpenGL(hwnd);
      return 0;

    case WM_SIZE:
      // hDC = GetDC(hwnd);
      wglMakeCurrent(globhandle->hDC, globhandle->hRC);

      glShadeModel(GL_FLAT);
      glViewport(0, 0, (GLsizei) LOWORD(lParam), (GLsizei) HIWORD (lParam));
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();

      /* this is upside-down for what most code thinks, but it is */
      /* right side up for the ray tracer..                       */
      gluOrtho2D(0.0, (GLsizei) LOWORD(lParam), 0.0, (GLsizei) HIWORD (lParam));
  
      glMatrixMode(GL_MODELVIEW);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glPixelZoom(1.0, 1.0); /* for upside-down images that ray uses */
      return 0;

    case WM_CLOSE:
      PostQuitMessage(0);
      return 0;

    case WM_PAINT:
      BeginPaint(hwnd, &ps);
      EndPaint(hwnd, &ps);
      return 0;

    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }

  return 0;
}


void * glwin_create(int width, int height) {
  oglhandle * handle;
  int rc;

  handle = (oglhandle *) malloc(sizeof(oglhandle));
  if (handle == NULL)
    return NULL;
  globhandle = handle;

  rc = OpenWin32Connection(handle);
  if (rc != 0) {
    printf("OpenWin32Connection() returned an error!\n");
    free(handle);
    return NULL;
  } 

  handle->width = width;
  handle->height = height;
  
  rc = myCreateWindow(handle, 0, 0, width, height); 
  if (rc != 0) {
    printf("CreateWindow() returned an error!\n");
    free(handle);
    return NULL;
  } 


  return handle;
}

void glwin_destroy(void * voidhandle) {
  oglhandle * handle = (oglhandle *) voidhandle;

  wglDeleteContext(handle->hRC);
  PostQuitMessage( 0 );

  /* glwin_handle_events(handle); */
}

void glwin_swap_buffers(void * voidhandle) {
  oglhandle * handle = (oglhandle *) voidhandle;
  glFlush();
}

int glwin_handle_events(void * voidhandle) {
  oglhandle * handle = (oglhandle *) voidhandle;
  MSG msg;

  if (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    return 1;
  }

  return 0;
}

void glwin_draw_image(void * voidhandle, int xsize, int ysize, unsigned char * img) {
  oglhandle * handle = (oglhandle *) voidhandle;
  glRasterPos2i(0, 0);
  glDrawPixels(xsize, ysize, GL_RGB, GL_UNSIGNED_BYTE, img);
  glwin_swap_buffers(voidhandle);
  return;
}


#endif




#else

void * glwin_create(int width, int height) {
  return NULL;
}

void glwin_destroy(void * voidhandle) {
  return;
}

void glwin_swap_buffers(void * voidhandle) {
  return;
}

int glwin_handle_events(void * voidhandle) {
  return 0;
}

void glwin_draw_image(void * voidhandle, int xsize, int ysize, unsigned char * img) {
  return;
}

#endif

