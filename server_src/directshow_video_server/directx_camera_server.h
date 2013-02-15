#ifndef	DIRECTX_CAMERA_SERVER_H
#define	DIRECTX_CAMERA_SERVER_H

#include "vrpn_Configure.h"

#ifdef	VRPN_USE_DIRECTSHOW


#include "vrpn_Shared.h"
#include <stdio.h>
#include <math.h>

// Horrible hack to get around missing file in Platform SDK
#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__

// Include files for DirectShow video input
#include <dshow.h>
#include <qedit.h>

#include  <vrpn_Imager.h>

// This code (and the code in the derived videofile server) is
// based on information the book "Programming Microsoft DirectShow
// for Digital Video and Television", Mark D. Pesce.  Microsoft Press.
// ISBN 0-7356-1821-6.  This book was required reading for me to
// understand how the filter graphs run, how to control replay rate,
// and how to reliably grab all of the samples from a stream.
// The chapter on using the Sample Grabber Filter
// (ch. 11) was particularly relevant.  Note that I did not break the seal
// on the enclosed disk, which would require me to agree to licensing terms
// that include me not providing anyone with copies of my modified versions
// of the sample code in source-code format.  Instead, I used the information
// gained in the reading of the book to write by hand my own version of this
// code.  What a sick world we live in when example code can't be freely
// shared.

// The Microsoft Platform SDK must be installed on your machine
// to make this work; this code makes use of the BaseClasses library and
// other parts of the library.

class directx_samplegrabber_callback; //< Forward declaration

class directx_camera_server {
public:
  /// Open the nth available camera
  directx_camera_server(int which, unsigned width = 320, unsigned height = 240);
  virtual ~directx_camera_server(void);
  virtual void close_device(void);

  /// Return the number of colors that the device has
  virtual unsigned  get_num_colors() const { return 3; };

  /// Read an image to a memory buffer.  Max < min means "whole range"
  virtual bool	read_image_to_memory(unsigned minX = 255, unsigned maxX = 0,
			     unsigned minY = 255, unsigned maxY = 0,
			     double exposure_millisecs = 250.0);

  /// Get pixels out of the memory buffer, RGB indexes the colors
  virtual bool	get_pixel_from_memory(unsigned X, unsigned Y, vrpn_uint8 &val, int RGB = 0) const;
  virtual bool	get_pixel_from_memory(unsigned X, unsigned Y, vrpn_uint16 &val, int RGB = 0) const;

  /// Store the memory image to a PPM file.
  virtual bool  write_memory_to_ppm_file(const char *filename, bool sixteen_bits = false) const;

  /// Send in-memory image over a vrpn connection
  virtual bool  send_vrpn_image(vrpn_Imager_Server* svr,vrpn_Connection* svrcon,double g_exposure,int svrchan, int num_chans = 1) const;

  /// Is the camera working properly?
  bool working(void) const { return _status; };

  // Tell what the range is for the image.
  virtual void read_range(int &minx, int &maxx, int &miny, int &maxy) const {
    minx = _minX; miny = _minY; maxx = _maxX; maxy = _maxY;
  }

  virtual unsigned  get_num_rows(void) const { 
    return _num_rows;
  }
  virtual unsigned  get_num_columns(void) const {
    return _num_columns;
  }

protected:
  /// Construct but do not open camera (used by derived classes)
  directx_camera_server();

  // Objects needed for DirectShow video input.
  IGraphBuilder *_pGraph;	      // Constructs a DirectShow filter graph
  ICaptureGraphBuilder2 *_pBuilder;   // Filter graph builder
  IMediaControl *_pMediaControl;      // Handles media streaming in the filter graph
  IMediaEvent   *_pEvent;	      // Handles filter graph events
  IBaseFilter *_pSampleGrabberFilter; // Grabs samples from the media stream
  ISampleGrabber *_pGrabber;	      // Interface for the sample grabber filter
  IAMStreamConfig *_pStreamConfig;    // Interface to set the video dimensions

  bool	    _status;			//< True is working, false is not
  unsigned  _num_rows, _num_columns;    //< Size of the memory buffer
  unsigned  _minX, _minY, _maxX, _maxY;	//< The region most recently read

  // Memory pointers used to get non-virtual memory
  unsigned char	*_buffer;   //< Buffer for what comes from camera,
  size_t    _buflen;	    //< Length of that buffer
  bool	    _started_graph; //< Did we start the filter graph running?
  unsigned  _mode;	    //< Mode 0 = running, Mode 1 = paused.

  long	    _stride;	    //< How many bytes to skip when going to next line (may be negative for upside-down images)

  // Pointer to the associated sample grabber callback object.
  directx_samplegrabber_callback  *_pCallback;

  virtual bool	open_and_find_parameters(const int which, unsigned width, unsigned height);
  virtual bool	read_one_frame(unsigned minX, unsigned maxX,
			unsigned minY, unsigned maxY,
			unsigned exposure_millisecs);
};

// This class is used to handle callbacks from the SampleGrabber filter.  It
// grabs each sample and holds onto it until the camera server that is associated
// with the object comes and gets it.  The callback method in this class is
// called in another thread, so its methods need to be guarded with semaphores.

class directx_samplegrabber_callback : public ISampleGrabberCB {
public:
  directx_samplegrabber_callback(void);
  ~directx_samplegrabber_callback(void);

  void shutdown(void) { _stayAlive = false; Sleep(100); }

  // Boolean flag telling whether there is a sample in the image
  // buffer ready for the application thread to consume.  Set to
  // true by the callback when there is an image there, and back
  // to false by the application thread when it reads the image.
  // XXX This should be done using a semaphore to avoid having
  // to poll in the application.
  bool	imageReady; //< true when there is an image ready to be processed

  // Boolean flag telling whether the app is done processing the image
  // buffer so that the callback thread can return it to the filter graph.
  // Set to true by the application when it finishes, and back
  // to false by the callback thread when it gets a new image.
  // XXX This should be done using a semaphore to avoid having
  // to poll in the callback thread.
  bool	imageDone; //< true when the app has finished processing an image

  // A pointer to the image sample that has been passed to the sample
  // grabber callback handler.
  IMediaSample  *imageSample;

  // These three methods must be defined because of the IUnknown parent class.
  // XXX The first two are a hack to pretend that we are doing reference counting;
  // this object must last longer than the sample grabber it is connected to in
  // order to avoid segmentations faults.
  STDMETHODIMP_(ULONG) AddRef(void) { return 1; }
  STDMETHODIMP_(ULONG) Release(void) { return 2; }
  STDMETHOD(QueryInterface)(REFIID interfaceRequested, void **handleToInterfaceRequested);

  // One of the following two methods must be defined do to the ISampleGraberCB
  // parent class; this is the way we hear from the grabber.  We implement the one that
  // gives us unbuffered access.  Be sure to turn off buffering in the SampleGrabber
  // that is associated with this callback handler.
  STDMETHODIMP BufferCB(double, BYTE *, long) { return E_NOTIMPL; }
  STDMETHOD(SampleCB)(double time, IMediaSample *sample);

protected:
  BITMAPINFOHEADER  _bitmapInfo;  //< Describes format of the bitmap
  bool	_stayAlive;		  //< Tells all threads to exit
};

#endif
#endif
