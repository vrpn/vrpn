#include <vrpn_Configure.h>

#include "directx_camera_server.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <vrpn_BaseClass.h>

// Libraries we need to link with to use what we're using.
#if defined(_MSC_VER) && !defined(VRPN_USING_CMAKE)
#pragma comment(lib,"strmiids.lib")
#endif

//#define	DEBUG

//-----------------------------------------------------------------------
// Helper functions for editing the filter graph:

static HRESULT GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin)
{
    IEnumPins  *pEnum;
    IPin       *pPin;
    pFilter->EnumPins(&pEnum);
    while(pEnum->Next(1, &pPin, 0) == S_OK)
    {
        PIN_DIRECTION PinDirThis;
        pPin->QueryDirection(&PinDirThis);
        if (PinDir == PinDirThis)
        {
            pEnum->Release();
            *ppPin = pPin;
            return S_OK;
        }
        pPin->Release();
    }
    pEnum->Release();
    return E_FAIL;  
}

static HRESULT ConnectTwoFilters(IGraphBuilder *pGraph, IBaseFilter *pFirst, IBaseFilter *pSecond)
{
    IPin *pOut = NULL, *pIn = NULL;
    HRESULT hr = GetPin(pFirst, PINDIR_OUTPUT, &pOut);
    if (FAILED(hr)) return hr;
    hr = GetPin(pSecond, PINDIR_INPUT, &pIn);
    if (FAILED(hr)) 
    {
        pOut->Release();
        return E_FAIL;
     }
    hr = pGraph->Connect(pOut, pIn);
    pIn->Release();
    pOut->Release();
    return hr;
}


//-----------------------------------------------------------------------

/** The first time we are called, start the filter graph running in continuous
    mode and grab the first image that comes out.  Later times, grab each new
    image as it comes.  The "mode" parameter tells what mode we are in:
      Mode 0 = Run()
      Mode 1 = Pause()
    */
bool  directx_camera_server::read_one_frame(unsigned minX, unsigned maxX,
			      unsigned minY, unsigned maxY,
			      unsigned exposure_millisecs)
{
  HRESULT hr;

  if (!_status) { return false; };

  // If we have not yet started the media graph running, set up the callback
  // handler for the sample grabber filter so that we will hear about each frame
  // as it comes in.  Then set the filter graph running.
  if (!_started_graph) {
    // Set the grabber do not do one-shot mode because that would cause
    // it to stop the filter graph after a single frame is captured.
    _pGrabber->SetOneShot(FALSE);

    // Set the grabber to not do buffering mode, because we've not implemented
    // the handler for buffered callbacks.
    _pGrabber->SetBufferSamples(FALSE);

    // Run the graph and wait until it captures a frame into its buffer
    switch (_mode) {
      case 0: // Case 0 = run
	hr = _pMediaControl->Run();
	break;
      case 1: // Case 1 = paused
	hr = _pMediaControl->Pause();
	break;
      default:
	fprintf(stderr, "directx_camera_server::read_one_frame(): Unknown mode (%d)\n", _mode);
	_status = false;
	return false;
    }
    if ( (hr != S_OK) && (hr != S_FALSE) ){
      fprintf(stderr,"directx_camera_server::read_one_frame(): Can't run filter graph\n");
      _status = false;
      return false;
    }

    _started_graph = true;
  }

  //XXX Should the app set the timeout period?

  // Wait until there is a sample ready in the callback handler.  If there is,
  // copy it into our buffer and then tell it we are done processing the sample.
  // If it takes too long, time out.
  const int TIMEOUT_MSECS = 250;
  BYTE	*imageLocation;
  if (!_pCallback->imageReady) {
    for (int i = 0; i < TIMEOUT_MSECS; i++) {
      vrpn_SleepMsecs(1);
      if (_pCallback->imageReady) { break; }	// Break out of the wait if its ready
    }
    if (!_pCallback->imageReady) {
#ifdef DEBUG
      fprintf(stderr,"directx_camera_server::read_one_frame(): Timeout when reading image\n");
#endif
      return false;
    }
  }

  // If we are in mode 2, then we pause the graph after we captured one image.
  if (_mode == 2) {
    _pMediaControl->Pause();
    _mode = 1;
  }

  if (_pCallback->imageReady) {
    _pCallback->imageReady = false;
    if (FAILED(_pCallback->imageSample->GetPointer(&imageLocation))) {
      fprintf(stderr,"directx_camera_server::read_one_frame(): Can't get buffer\n");
      _status = false;
      _pCallback->imageDone = true;
      return false;
    }
    // Step through each line of the video and copy it into the buffer.  We
    // do one line at a time here because there can be padding at the end of
    // each line on some video formats.
    for (DWORD iRow = 0; iRow < _num_rows; iRow++) {
      memcpy(_buffer+_num_columns*3*iRow, imageLocation+_stride*iRow, _num_columns*3);
    }
    _pCallback->imageDone = true;
  }

  // Capture timing information and print out how many frames per second
  // are being received.

  { static struct timeval last_print_time;
    struct timeval now;
    static bool first_time = true;
    static int frame_count = 0;

    if (first_time) {
      vrpn_gettimeofday(&last_print_time, NULL);
      first_time = false;
    } else {
      static	unsigned  last_r = 10000;
      frame_count++;
      vrpn_gettimeofday(&now, NULL);
      double timesecs = 0.001 * vrpn_TimevalMsecs(vrpn_TimevalDiff(now, last_print_time));
      if (timesecs >= 5) {
	double frames_per_sec = frame_count / timesecs;
	frame_count = 0;
	printf("Received frames per second = %lg\n", frames_per_sec);
	last_print_time = now;
      }
    }
  }

  return true;
}

//---------------------------------------------------------------------
// Open the camera specified and determine its available features and parameters.

bool directx_camera_server::open_and_find_parameters(const int which, unsigned width, unsigned height)
{
  HRESULT hr;

  //-------------------------------------------------------------------
  // Create COM and DirectX objects needed to access a video stream.

  // Initialize COM.  This must have a matching uninitialize somewhere before
  // the object is destroyed.
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before CoInitialize\n");
#endif
  CoInitialize(NULL);

  // Create the filter graph manager
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before CoCreateInstance FilterGraph\n");
#endif
  CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
		      IID_IGraphBuilder, (void **)&_pGraph);
  if (_pGraph == NULL) {
    fprintf(stderr, "directx_camera_server::open_and_find_parameters(): Can't create graph manager\n");
    return false;
  }
  _pGraph->QueryInterface(IID_IMediaControl, (void **)&_pMediaControl);
  _pGraph->QueryInterface(IID_IMediaEvent, (void **)&_pEvent);

  // Create the Capture Graph Builder.
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before CoCreateInstance CaptureGraphBuilder2\n");
#endif
  CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, 
      IID_ICaptureGraphBuilder2, (void **)&_pBuilder);
  if (_pBuilder == NULL) {
    fprintf(stderr, "directx_camera_server::open_and_find_parameters(): Can't create graph builder\n");
    return false;
  }

  // Associate the graph with the builder.
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before SetFilterGraph\n");
#endif
  _pBuilder->SetFiltergraph(_pGraph);

  //-------------------------------------------------------------------
  // Go find a video device to use: in this case, we are using the Nth
  // one we find, where the number N is the "which" parameter.

  // Create the system device enumerator.
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before CoCreateInstance SystemDeviceEnum\n");
#endif
  ICreateDevEnum *pDevEnum = NULL;
  CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, 
      IID_ICreateDevEnum, (void **)&pDevEnum);
  if (pDevEnum == NULL) {
    fprintf(stderr, "directx_camera_server::open_and_find_parameters(): Can't create device enumerator\n");
    return false;
  }

  // Create an enumerator for video capture devices.
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before CreateClassEnumerator\n");
#endif
  IEnumMoniker *pClassEnum = NULL;
  pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
  if (pClassEnum == NULL) {
    fprintf(stderr, "directx_camera_server::open_and_find_parameters(): Can't create video enumerator (no cameras?)\n");
    pDevEnum->Release();
    return false;
  }

#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before Loop over enumerators\n");
#endif
  ULONG cFetched;
  IMoniker *pMoniker = NULL;
  IBaseFilter *pSrc = NULL;
  // Skip (which - 1) cameras
  int i;
  for (i = 0; i < which-1 ; i++) {
    if (pClassEnum->Next(1, &pMoniker, &cFetched) != S_OK) {
      fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't open camera (not enough cameras)\n");
      pMoniker->Release();
      return false;
    }
  }
  // Take the next camera and bind it
  if (pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK) {
    // Bind the first moniker to a filter object.
    pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
    pMoniker->Release();
  } else {
    fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't open camera (not enough cameras)\n");
    pMoniker->Release();
    return false;
  }

  pClassEnum->Release();
  pDevEnum->Release();

  //-------------------------------------------------------------------
  // Construct the sample grabber callback handler that will be used
  // to receive image data from the sample grabber.
  if ( (_pCallback = new directx_samplegrabber_callback()) == NULL) {
    fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't create sample grabber callback handler (out of memory?)\n");
    return false;
  }

  //-------------------------------------------------------------------
  // Construct the sample grabber that will be used to snatch images from
  // the video stream as they go by.  Set its media type and callback.

  // Create the Sample Grabber.
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before CoCreateInstance SampleGrabber\n");
#endif
  CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
      IID_IBaseFilter, reinterpret_cast<void**>(&_pSampleGrabberFilter));
  if (_pSampleGrabberFilter == NULL) {
    fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't get SampleGrabber filter (not DirectX 8.1+?)\n");
    return false;
  }
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before QueryInterface\n");
#endif
  _pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber,
      reinterpret_cast<void**>(&_pGrabber));

  // Set the media type to video
#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before SetMediaType\n");
#endif
  AM_MEDIA_TYPE mt;
  // Ask for video media producers that produce 8-bit RGB
  ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
  mt.majortype = MEDIATYPE_Video;	  // Ask for video media producers
  mt.subtype = MEDIASUBTYPE_RGB24;	  // Ask for 8 bit RGB
  _pGrabber->SetMediaType(&mt);

  //-------------------------------------------------------------------
  // Ask for the video resolution that has been passed in.
  // This code is based on
  // intuiting that we need to use the SetFormat call on the IAMStreamConfig
  // interface; this interface is described in the help pages.
  // If the width and height are specified as 0, then they are not set
  // in the header, letting them use whatever is the default.
  if ( (width != 0) && (height != 0) ) {
    _pBuilder->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSrc,
			      IID_IAMStreamConfig, (void **)&_pStreamConfig);
    if (_pStreamConfig == NULL) {
      fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't get StreamConfig interface\n");
      return false;
    }

    ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
    mt.majortype = MEDIATYPE_Video;	  // Ask for video media producers
    mt.subtype = MEDIASUBTYPE_RGB24;	  // Ask for 8 bit RGB
    mt.pbFormat = (BYTE*)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
    VIDEOINFOHEADER *pVideoHeader = (VIDEOINFOHEADER*)mt.pbFormat;
    ZeroMemory(pVideoHeader, sizeof(VIDEOINFOHEADER));
    pVideoHeader->bmiHeader.biBitCount = 24;
    pVideoHeader->bmiHeader.biWidth = width;
    pVideoHeader->bmiHeader.biHeight = height;
    pVideoHeader->bmiHeader.biPlanes = 1;
    pVideoHeader->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pVideoHeader->bmiHeader.biSizeImage = DIBSIZE(pVideoHeader->bmiHeader);

    // Set the format type and size.
    mt.formattype = FORMAT_VideoInfo;
    mt.cbFormat = sizeof(VIDEOINFOHEADER);

    // Set the sample size.
    mt.bFixedSizeSamples = TRUE;
    mt.lSampleSize = DIBSIZE(pVideoHeader->bmiHeader);

    // Make the call to actually set the video type to what we want.
    if (_pStreamConfig->SetFormat(&mt) != S_OK) {
      fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't set resolution to %dx%d\n",
	pVideoHeader->bmiHeader.biWidth, pVideoHeader->bmiHeader.biHeight);
      return false;
    }

    // Clean up the pbFormat header memory we allocated above.
    CoTaskMemFree(mt.pbFormat);
  }

  //-------------------------------------------------------------------
  // Create a NULL renderer that will be used to discard the video frames
  // on the output pin of the sample grabber

#ifdef	DEBUG
  printf("directx_camera_server::open_and_find_parameters(): Before CoCreateInstance NullRenderer\n");
#endif
  IBaseFilter *pNull = NULL;
  CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
      IID_IBaseFilter, reinterpret_cast<void**>(&pNull));

  //-------------------------------------------------------------------
  // Build the filter graph.  First add the filters and then connect them.

  // pSrc is the capture filter for the video device we found above.
  _pGraph->AddFilter(pSrc, L"Video Capture");

  // Add the sample grabber filter
  _pGraph->AddFilter(_pSampleGrabberFilter, L"SampleGrabber");

  // Add the null renderer filter
  _pGraph->AddFilter(pNull, L"NullRenderer");

  // Connect the output of the video reader to the sample grabber input
  ConnectTwoFilters(_pGraph, pSrc, _pSampleGrabberFilter);

  // Connect the output of the sample grabber to the NULL renderer input
  ConnectTwoFilters(_pGraph, _pSampleGrabberFilter, pNull);

  //-------------------------------------------------------------------
  // XXX See if this is a video tuner card by querying for that interface.
  // Set it to read the video channel if it is one.
  IAMTVTuner  *pTuner = NULL;
  hr = _pBuilder->FindInterface(NULL, NULL, pSrc, IID_IAMTVTuner, (void**)&pTuner);
  if (pTuner != NULL) {
#ifdef	DEBUG
    printf("directx_camera_server::open_and_find_parameters(): Found a TV Tuner!\n");
#endif

    //XXX Put code here.
    // Set the first input pin to use the cable as input
    hr = pTuner->put_InputType(0, TunerInputCable);
    if (FAILED(hr)) {
      fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't set input to cable\n");
    }

    // Set the channel on the video to be baseband (is this channel zero?)
    hr = pTuner->put_Channel(0, -1, -1);
    if (FAILED(hr)) {
      fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't set channel\n");
    }

    pTuner->Release();
  }
  

  //-------------------------------------------------------------------
  // Find _num_rows and _num_columns in the video stream.
  _pGrabber->GetConnectedMediaType(&mt);
  VIDEOINFOHEADER *pVih;
  if (mt.formattype == FORMAT_VideoInfo) {
      pVih = reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat);
  } else {
    fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Can't get video header type\n");
    return false;
  }

  // Number of rows and columns.  This is different if we are using a target
  // rectangle (rcTarget) than if we are not.
  if (IsRectEmpty(&pVih->rcTarget)) {
    _num_columns = pVih->bmiHeader.biWidth;
    _num_rows = pVih->bmiHeader.biHeight;
  } else {
    _num_columns = pVih->rcTarget.right;
    _num_rows = pVih->bmiHeader.biHeight;
    printf("XXX directx_camera_server::open_and_find_parameters(): Warning: may not work correctly with target rectangle\n");
  }
  _minX = 0;
  _maxX = _num_columns - 1;
  _minY = 0;
  _maxY = _num_rows - 1;
#ifdef DEBUG
  printf("Got %dx%d video\n", _num_columns, _num_rows);
#endif

  // Make sure that the image is not compressed and that we have 8 bits
  // per pixel.
  if (pVih->bmiHeader.biCompression != BI_RGB) {
    fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Compression not RGB\n");
    switch (pVih->bmiHeader.biCompression) {
      case BI_RLE8:
	fprintf(stderr,"  (It is BI_RLE8)\n");
	break;
      case BI_RLE4:
	fprintf(stderr,"  (It is BI_RLE4)\n");
      case BI_BITFIELDS:
	fprintf(stderr,"  (It is BI_BITFIELDS)\n");
	break;
      default:
	fprintf(stderr,"  (Unknown compression type)\n");
    }
    return false;
  }
  int BytesPerPixel = pVih->bmiHeader.biBitCount / 8;
  if (BytesPerPixel != 3) {
    fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Not 3 bytes per pixel (%d)\n",
      pVih->bmiHeader.biBitCount);
    return false;
  }

  // A negative height indicates that the images are stored non-inverted in Y
  // Not sure what to do with images that have negative height -- need to
  // read the book some more to find out.
  if (_num_rows < 0) {
    fprintf(stderr,"directx_camera_server::open_and_find_parameters(): Num Rows is negative (internal error)\n");
    return false;
  }

  // Find the stride to take when moving from one row of video to the
  // next.  This is rounded up to the nearest DWORD.
  _stride = (_num_columns * BytesPerPixel + 3) & ~3;

  // Set the callback, where '0' means 'use the SampleCB callback'
  _pGrabber->SetCallback(_pCallback, 0);

  //-------------------------------------------------------------------
  // Release resources that won't be used later and return
  pSrc->Release();
  pNull->Release();
  return true;
}

/// Construct but do not open a camera
directx_camera_server::directx_camera_server() :
  _pGraph(NULL),
  _pBuilder(NULL),
  _pMediaControl(NULL),
  _pEvent(NULL),
  _pSampleGrabberFilter(NULL),
  _pGrabber(NULL),
  _pStreamConfig(NULL),
  _pCallback(NULL),
  _started_graph(false),
  _mode(0),
  _buffer(NULL)
{
  // No image in memory yet.
  _minX = _maxX = _minY = _maxY = 0;
}

/// Open nth available camera with specified resolution.
directx_camera_server::directx_camera_server(int which, unsigned width, unsigned height) :
  _pGraph(NULL),
  _pBuilder(NULL),
  _pMediaControl(NULL),
  _pEvent(NULL),
  _pSampleGrabberFilter(NULL),
  _pGrabber(NULL),
  _pStreamConfig(NULL),
  _pCallback(NULL),
  _started_graph(false),
  _mode(0),
  _buffer(NULL)
{
  //---------------------------------------------------------------------
  if (!open_and_find_parameters(which, width, height)) {
    fprintf(stderr, "directx_camera_server::directx_camera_server(): Cannot open camera\n");
    _status = false;
    return;
  }

  //---------------------------------------------------------------------
  // Allocate a buffer that is large enough to read the maximum-sized
  // image with no binning.
  _buflen = (unsigned)(_num_rows * _num_columns * 3);	// Expect B,G,R; 8-bits each.
  if ( (_buffer = new unsigned char[_buflen]) == NULL) {
    fprintf(stderr,"directx_camera_server::directx_camera_server(): Out of memory for buffer\n");
    _status = false;
    return;
  }
  // No image in memory yet.
  _minX = _maxX = _minY = _maxY = 0;

  _status = true;
}

//---------------------------------------------------------------------
// Close the camera and the system.  Free up memory.

void  directx_camera_server::close_device(void)
{
  // Clean up.
  if (_pGrabber) { _pGrabber->Release(); };
  if (_pSampleGrabberFilter) { _pSampleGrabberFilter->Release(); };
  if (_pStreamConfig) { _pStreamConfig->Release(); };
  if (_pEvent) { _pEvent->Release(); };
  if (_pMediaControl) { _pMediaControl->Release(); };
  if (_pBuilder) { _pBuilder->Release(); };
  if (_pGraph) { _pGraph->Release(); };
  CoUninitialize();
}
  
directx_camera_server::~directx_camera_server(void)
{
  // Get the callback device to immediately return all samples
  // it has queued up, then shut down the filter graph.
  _pCallback->shutdown();
  close_device();

  if (_buffer != NULL) { delete [] _buffer; }

  // Delete the callback object, so that it can clean up and
  // make sure all of its threads exit.
  delete _pCallback;
}

bool  directx_camera_server::read_image_to_memory(unsigned minX, unsigned maxX, unsigned minY, unsigned maxY,
					 double exposure_millisecs)
{
  if (!_status) { return false; };

  //---------------------------------------------------------------------
  // In case we fail, clear these
  _minX = _minY = _maxX = _maxY = 0;

  //---------------------------------------------------------------------
  // If the maxes are greater than the mins, set them to the size of
  // the image.
  if (maxX < minX) {
    minX = 0; maxX = _num_columns - 1;
  }
  if (maxY < minY) {
    minY = 0; maxY = _num_rows - 1;
  }

  //---------------------------------------------------------------------
  // Clip collection range to the size of the sensor on the camera.
  if (minX < 0) { minX = 0; };
  if (minY < 0) { minY = 0; };
  if (maxX >= _num_columns) { maxX = _num_columns - 1; };
  if (maxY >= _num_rows) { maxY = _num_rows - 1; };

  //---------------------------------------------------------------------
  // Store image size for later use
  _minX = minX; _minY = minY; _maxX = maxX; _maxY = maxY;

  //---------------------------------------------------------------------
  // Set up and read one frame, if we can.
  if (!read_one_frame(_minX, _maxX, _minY, _maxY, (int)exposure_millisecs)) {
    return false;
  }

  return true;
}

bool  directx_camera_server::write_memory_to_ppm_file(const char *filename, bool sixteen_bits) const
{
  // XXX This will depend on what kind of pixels we have!

  if (!_status) { return false; };

  //---------------------------------------------------------------------
  // Make sure the region is non-zero (so we've read an image)
  if ( (_maxX <= _minX) || (_maxY <= _minY) ) {
    fprintf(stderr,"directx_camera_server::write_memory_to_ppm_file(): No image in memory\n");
    return false;
  }

  //---------------------------------------------------------------------
  // If we've collected only a subset of the image, this version of the code
  // won't work.
  if ( (_minX != 0) || (_maxX != _num_columns-1) || (_minY != 0) || (_maxY != _num_rows-1) ) {
    fprintf(stderr, "directx_camera_server::write_memory_to_ppm_file(): Can't write subset (not implemented)\n");
    return false;
  }

  //---------------------------------------------------------------------
  // Swap the red and blue values for each triplet in the buffer, since
  // the DirectShow order is BGR and the PPM order is RGB.
  unsigned  char *bufrgb = new unsigned char[_buflen];
  if (bufrgb == NULL) {
    fprintf(stderr,"directx_camera_server::write_memory_to_ppm_file(): Out of memory\n");
    return false;
  }
  unsigned  X,Y;
  unsigned  cols = (_maxX - _minX) + 1;
  for (Y = _minY; Y < _maxY; Y++) {
    for (X = _minX; X < _maxX; X++) {
      bufrgb[ (Y*cols + X) * 3 + 0 ] = _buffer[ (Y*cols + X) * 3 + 2 ];
      bufrgb[ (Y*cols + X) * 3 + 1 ] = _buffer[ (Y*cols + X) * 3 + 1 ];
      bufrgb[ (Y*cols + X) * 3 + 2 ] = _buffer[ (Y*cols + X) * 3 + 0 ];
    }
  }

  //---------------------------------------------------------------------
  // Write out an uncompressed RGB PPM file.  We ignore the "16bits" request
  // because we don't have 16 bits to put in there!
  if (sixteen_bits) {
    fprintf(stderr, "directx_camera_server::write_memory_to_ppm_file(): 16 bits requested, ignoring\n");
  }
  FILE *of = fopen(filename, "wb");
  if (of == NULL) {
    fprintf(stderr, "directx_camera_server::write_memory_to_ppm_file(): Can't open %s for write\n", filename);
    return false;
  }
  if (fprintf(of, "P6\n%d %d\n%d\n", _num_columns, _num_rows, 255) < 0) {
    fprintf(stderr, "directx_camera_server::write_memory_to_ppm_file(): Can't write header to %s\n", filename);
    return false;
  }
  if (fwrite(bufrgb, 1, _buflen, of) != _buflen) {
    fprintf(stderr, "directx_camera_server::write_memory_to_ppm_file(): Can't write data to %s\n", filename);
    return false;
  }
  fclose(of);
  delete [] bufrgb;

  return true;
}

bool	directx_camera_server::get_pixel_from_memory(unsigned X, unsigned Y, vrpn_uint8 &val, int RGB) const
{
  if (!_status) { return false; };

  // Switch red and blue, since they are backwards in the record (blue is first).
  RGB = 2 - RGB;

  // XXX This will depend on what kind of pixels we have!
  if ( (_maxX <= _minX) || (_maxY <= _minY) ) {
    fprintf(stderr,"directx_camera_server::get_pixel_from_memory(): No image in memory\n");
    return false;
  }
  if ( (X < _minX) || (X > _maxX) || (Y < _minY) || (Y > _maxY) ) {
    return false;
  }
  unsigned  cols = _num_columns;
  val = _buffer[ (Y*cols + X) * 3 + RGB ];
  return true;
}

bool	directx_camera_server::get_pixel_from_memory(unsigned X, unsigned Y, vrpn_uint16 &val, int RGB) const
{
  if (!_status) { return false; };

  // Switch red and blue, since they are backwards in the record (blue is first).
  RGB = 2 - RGB;

  // XXX This will depend on what kind of pixels we have!
  if ( (_maxX <= _minX) || (_maxY <= _minY) ) {
    fprintf(stderr,"directx_camera_server::get_pixel_from_memory(): No image in memory\n");
    return false;
  }
  if ( (X < _minX) || (X > _maxX) || (Y < _minY) || (Y > _maxY) ) {
    return false;
  }
  unsigned  cols = _num_columns;
  val = _buffer[ (Y*cols + X) * 3 + RGB ];
  return true;
}

bool directx_camera_server::send_vrpn_image(vrpn_Imager_Server* svr,vrpn_Connection* svrcon,double g_exposure,int svrchan, int num_chans) const
{
    unsigned y;

    // Send the current frame over to the client in chunks as big as possible (limited by vrpn_IMAGER_MAX_REGION).
    int nRowsPerRegion=vrpn_IMAGER_MAX_REGIONu8/_num_columns;
    svr->send_begin_frame(0, _num_columns-1, 0, _num_rows-1);
    for(y=0; y<_num_rows; y+=nRowsPerRegion) {
      svr->send_region_using_base_pointer(svrchan,0,_num_columns-1,y,__min(_num_rows,y+nRowsPerRegion)-1,
	_buffer+2 /* Send the red channel */, 3, 3*_num_columns, _num_rows, true);
      svr->mainloop();
    }
    if (num_chans >= 2) {
      for(y=0; y<_num_rows; y+=nRowsPerRegion) {
        svr->send_region_using_base_pointer(svrchan+1,0,_num_columns-1,y,__min(_num_rows,y+nRowsPerRegion)-1,
	  _buffer+1 /* Send the green channel */, 3, 3*_num_columns, _num_rows, true);
        svr->mainloop();
      }
    }
    if (num_chans >= 3) {
      for(y=0; y<_num_rows; y+=nRowsPerRegion) {
        svr->send_region_using_base_pointer(svrchan+2,0,_num_columns-1,y,__min(_num_rows,y+nRowsPerRegion)-1,
	  _buffer+0 /* Send the blue channel */, 3, 3*_num_columns, _num_rows, true);
        svr->mainloop();
      }
    }
    svr->send_end_frame(0, _num_columns-1, 0, _num_rows-1);
    svr->mainloop();

    // Mainloop the server connection (once per server mainloop, not once per object).
    svrcon->mainloop();
    return true;
}

//--------------------------------------------------------------------------------------------
// This section implements the callback handler that gets frames from the
// SampleGrabber filter.

directx_samplegrabber_callback::directx_samplegrabber_callback(void) :
  imageReady(false),
  imageDone(false),
  imageSample(NULL),
  _stayAlive(true)
{
}

directx_samplegrabber_callback::~directx_samplegrabber_callback(void)
{
  // Make sure the other thread knows that it is okay to return the
  // buffer and wait until it has had time to do so.
  _stayAlive = false; Sleep(100);
};

HRESULT directx_samplegrabber_callback::QueryInterface(REFIID interfaceRequested, void **handleToInterfaceRequested)
{
  if (handleToInterfaceRequested == NULL) { return E_POINTER; }
  if (interfaceRequested == IID_IUnknown) {
    *handleToInterfaceRequested = static_cast<IUnknown *>(this);
  } else if (interfaceRequested == IID_ISampleGrabberCB) {
    *handleToInterfaceRequested = static_cast<ISampleGrabberCB *>(this);
  } else {
    return E_NOINTERFACE;
  }
  AddRef();
  return S_OK;
}

// This is the routine that processes each sample.  It gets the information about
// the sample (one frame) from the SampleGrabber, then marks itself as being ready
// to process the sample.  It then blocks until the sample has been processed by
// the associated camera server.
// The hand-off is handled by using two booleans acting as semaphores.
// The first semaphore (imageReady)
// controls access to the callback handler's buffer so that the application thread
// will only read it when it is full.  The second sempaphore (imageDone) controls when
// the handler routine can release a sample; it makes sure that the sample is not
// released before the application thread is done processing it.
// The directx camera must be sure to free an open sample (if any) after changing
// the state of the filter graph, so that this doesn't block indefinitely.  This means
// that the destructor for any object using this callback object has to destroy
// this object.  The destructor sets _stayAlive to false to make sure this thread terminates.

HRESULT directx_samplegrabber_callback::SampleCB(double time, IMediaSample *sample)
{
  // Point the image sample to the media sample we have and then set the flag
  // to tell the application it can process it.
  imageSample = sample;
  imageReady = true;

  // Wait until the image has been processed and then return the buffer to the
  // filter graph
  while (!imageDone && _stayAlive) { vrpn_SleepMsecs(1); }
  if (_stayAlive) {
    imageDone = false;
  }

  return S_OK;
}
