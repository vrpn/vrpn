//XXX Adam has code in the SEM simulator whereby the client requests a number
// of images from the server.  Once some have come in, it requests more.  This
// keeps the network pipeline from filling up with a bunch of images.  Could
// be a generally useful feature to keep the latency down.  See below...

// XXX Client can send request for certain number of images (-1 for continuous)
//     Example: client may ask for two to start, and then one more for each to
//     produce one outstanding buffer at all times.
// XXX The example imager for Nano will send a request for -1 (continuous) when it gets the
//     first pong from the server.
// XXX Do we enable a different message type to force a frame send even if there
//     is no new image (or no image at all?).  This would be useful to the example
//     imager for nano but would impose yuck on all of the servers.  Don't do it ...
//     it will fail when reading from a stored file anyway.  Do have the nano
//     imager server send a new copy whenever a nonzero number of images is requested,
//     even if there are no changes.  Bracket this with begin and end regions messages.
//     Other ones won't be bracketed.
// XXX Server sets number to send back to -1 when last connection closed.

// XXX Client can sent request for only subregion of image to be sent
//     Server may ignore this message.
// XXX Server packs marker DISCARDED(four corners of [sub]region) if none requested when one gotten
//     Every frame that is discarded will be so marked, so the client can count lost frames.
//     They are marked as discarded even if they were lost by something in the driver rather
//     than by the connection getting clogged up.  Actually, include a count of how many were
//     discarded (so that if it finds out about them all at once it doesn't have to back a
//     bunch of messages).
// XXX Server packs marker BEGIN(four corners of [sub]region) before sending each frame known to be contiguous
// XXX Server packs marker END(four corners of [sub]region) when done sending each frame known to be contiguous
// XXX Server sets region back to total region when last connection closed.

#ifndef	VRPN_TEMPIMAGER_H
#define	VRPN_TEMPIMAGER_H
#include  "vrpn_Connection.h"
#include  "vrpn_BaseClass.h"

const unsigned vrpn_IMAGER_MAX_CHANNELS = 100;

/// Set of constants to tell how many points you can put into a region
/// depending on the type you are putting in there.  Useful for senders
/// to know how large of a chunk they can send at once.
const unsigned vrpn_IMAGER_MAX_REGIONu8 = (vrpn_CONNECTION_TCP_BUFLEN - 6*sizeof(vrpn_int16) - 6*sizeof(vrpn_int32))/sizeof(vrpn_uint8);
const unsigned vrpn_IMAGER_MAX_REGIONu16 = (vrpn_CONNECTION_TCP_BUFLEN - 6*sizeof(vrpn_int16) - 6*sizeof(vrpn_int32))/sizeof(vrpn_uint16);
const unsigned vrpn_IMAGER_MAX_REGIONf32 = (vrpn_CONNECTION_TCP_BUFLEN - 6*sizeof(vrpn_int16) - 6*sizeof(vrpn_int32))/sizeof(vrpn_float32);

/// Holds the description needed to convert from raw data to values for a channel
class VRPN_API vrpn_TempImager_Channel {
public:
  vrpn_TempImager_Channel(void) { name[0] = '\0'; units[0] = '\0'; minVal = maxVal = 0.0;
      scale = offset = 0; };

  inline  bool	buffer(char **insertPt, vrpn_int32 *buflen) {
    if (vrpn_buffer(insertPt, buflen, minVal) ||
        vrpn_buffer(insertPt, buflen, maxVal) ||
        vrpn_buffer(insertPt, buflen, offset) ||
        vrpn_buffer(insertPt, buflen, scale) ||
	vrpn_buffer(insertPt, buflen, name, sizeof(name)) ||
	vrpn_buffer(insertPt, buflen, units, sizeof(units)) ) {
      return false;
    } else {
      return true;
    }
  }

  inline  bool	unbuffer(const char **buffer) {
    if (vrpn_unbuffer(buffer, &minVal) ||
        vrpn_unbuffer(buffer, &maxVal) ||
        vrpn_unbuffer(buffer, &offset) ||
        vrpn_unbuffer(buffer, &scale) ||
	vrpn_unbuffer(buffer, name, sizeof(name)) ||
	vrpn_unbuffer(buffer, units, sizeof(units)) ) {
      return false;
    } else {
      return true;
    }
  }

  cName	name;			//< Name of the data set stored in this channel
  cName	units;			//< Units for the data set stored in this channel
  vrpn_float32	minVal, maxVal; //< Range of possible values for pixels in this channel
  vrpn_float32	offset, scale;	//< Values in units are (raw_values * scale) + offset
};

/// Base class for temporary implementation of Imager class
class VRPN_API vrpn_TempImager: public vrpn_BaseClass {
public:
  vrpn_TempImager(const char *name, vrpn_Connection *c = NULL);

  // Data member accessors.
  vrpn_int32	nRows(void) const { return d_nRows; };
  vrpn_int32	nCols(void) const { return d_nCols; };
  vrpn_float32	minX(void) const { return d_minX; };
  vrpn_float32	maxX(void) const { return d_maxX; };
  vrpn_float32	minY(void) const { return d_minY; };
  vrpn_float32	maxY(void) const { return d_maxY; };

protected:
  vrpn_int32	d_nRows;	//< Number of rows in the image
  vrpn_int32	d_nCols;	//< Number of columns in the image
  vrpn_float32	d_minX,d_maxX;	//< Real-space range of X pixel centers in units (meters, radians)
  vrpn_float32	d_minY,d_maxY;	//< Real-space range of Y pixel centers in units (meters, radians)
  vrpn_int32	d_nChannels;	//< Number of image data channels
  vrpn_TempImager_Channel d_channels[vrpn_IMAGER_MAX_CHANNELS];

  virtual int register_types(void);
  vrpn_int32	d_description_m_id; //< ID of the message type describing the range and channels
  vrpn_int32	d_regionu8_m_id;    //< ID of the message type describing a region with 8-bit unsigned entries
  vrpn_int32	d_regionu16_m_id;   //< ID of the message type describing a region with 16-bit unsigned entries
  vrpn_int32	d_regionf32_m_id;   //< ID of the message type describing a region with 32-bit float entries
};

class VRPN_API vrpn_TempImager_Server: public vrpn_TempImager {
public:
  vrpn_TempImager_Server(const char *name, vrpn_Connection *c,
			 vrpn_int32 nCols, vrpn_int32 nRows,
			 vrpn_float32 minX = 0, vrpn_float32 maxX = 1,
			 vrpn_float32 minY = 0, vrpn_float32 MaxY = 1);

  /// Add a channel to the server, returns index of the channel or -1 on failure.
  int	add_channel(const char *name, const char *units = "unsigned8bit",
		    vrpn_float32 minVal = 0, vrpn_float32 maxVal = 255,
		    vrpn_float32 scale = 1, vrpn_float32 offset = 0);

  /// Pack and send the region as efficiently as possible; strides are in steps of the element being sent.
  // These functions each take a pointer to the base of the image to be sent: its [0,0] element.
  bool	send_region_using_base_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_uint8 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride,
		    vrpn_uint16 nRows = 0, bool invert_y = false,
		    const struct timeval *time = NULL);
  bool	send_region_using_base_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_uint16 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride,
		    vrpn_uint16 nRows = 0, bool invert_y = false,
		    const struct timeval *time = NULL);
  bool	send_region_using_base_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_float32 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride,
		    vrpn_uint16 nRows = 0, bool invert_y = false,
		    const struct timeval *time = NULL);

  /// Pack and send the region as efficiently as possible; strides are in steps of the element being sent.
  // These functions each take a pointer to the first of the data values to be sent.  This is a
  // pointer to the [cMin, rMin] element of the image to be sent.  Note that if the Y value is inverted,
  // this will NOT be a pointer to the beginning of the data block, but rather the the beginning of
  // the last line in the data block.  Note that rowStride will be less than the number of rows in the
  // whole image if the data is tightly packed into a block and the region does not cover all columns.
  bool	send_region_using_first_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_uint8 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride,
		    vrpn_uint16 nRows = 0, bool invert_y = false,
		    const struct timeval *time = NULL);
  bool	send_region_using_first_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_uint16 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride,
		    vrpn_uint16 nRows = 0, bool invert_y = false,
		    const struct timeval *time = NULL);
  bool	send_region_using_first_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_float32 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride,
		    vrpn_uint16 nRows = 0, bool invert_y = false,
		    const struct timeval *time = NULL);

  /// Set the resolution to a different value than it had been before.  Returns true on success.
  bool	set_resolution(vrpn_int32 nCols, vrpn_int32 nRows);

  /// Set the range of the region to a different range.  Returns true on success.
  bool	set_range(vrpn_float32 minX, vrpn_float32 maxX, vrpn_float32 minY, vrpn_float32 maxY);

  /// Handle baseclass ping/pong messages
  virtual void	mainloop(void);

protected:
  bool	d_description_sent;    //< Has the description message been sent?
  /// Sends a description of the imager so the remote can process the region messages
  bool	send_description(void);

  // This method makes sure we send a description whenever we get a ping from
  // a client object.
  static  int VRPN_CALLBACK handle_ping_message(void *userdata, vrpn_HANDLERPARAM p);
};

//------------------------------------------------------------------------------
// Users deal with things below this line.

const vrpn_uint16 vrpn_IMAGER_VALTYPE_UNKNOWN	= 0;
const vrpn_uint16 vrpn_IMAGER_VALTYPE_UINT8	= 1;
const vrpn_uint16 vrpn_IMAGER_VALTYPE_UINT16	= 2;
const vrpn_uint16 vrpn_IMAGER_VALTYPE_FLOAT32	= 3;

class VRPN_API	vrpn_TempImager_Region;

typedef struct _vrpn_IMAGERREGIONCB {
  struct timeval		msg_time; //< Timestamp of the region data's change
  const vrpn_TempImager_Region	*region;  //< New region of the image
} vrpn_IMAGERREGIONCB;

typedef void (VRPN_CALLBACK *vrpn_IMAGERREGIONHANDLER) (void * userdata,
					  const vrpn_IMAGERREGIONCB info);
typedef void (VRPN_CALLBACK *vrpn_IMAGERDESCRIPTIONHANDLER) (void * userdata,
					       const struct timeval msg_time);

/// Helper function to convert data for a sub-region of one channel of
// the image.  This is passed to the user callback handler and aids in
// getting values out of the buffer.  The region is only valid during
// the actual callback handler, so users should not store pointers to
// it for later use.
class VRPN_API vrpn_TempImager_Region {
  friend class VRPN_API vrpn_TempImager_Remote;
  friend void VRPN_CALLBACK java_vrpn_handle_region_change( void * userdata, const vrpn_IMAGERREGIONCB info );

public:
  vrpn_TempImager_Region(void) { d_chanIndex = -1; d_rMin = d_rMax = d_cMin = d_cMax = 0; 
				 d_valBuf = NULL; d_valType = vrpn_IMAGER_VALTYPE_UNKNOWN;
				 d_valid = false; }

  /// Returns the number of values in the region.
  inline vrpn_uint32 getNumVals( ) const {
    if (!d_valid) { return 0;
    } else { return ( d_rMax - d_rMin + 1 ) * ( d_cMax - d_cMin + 1 ); }
  }

  /// Reads pixel from the region with no scale and offset applied to the value.  Not
  /// the most efficient way to read the pixels out -- use the block read routines.
  inline  bool	read_unscaled_pixel(vrpn_uint16 c, vrpn_uint16 r, vrpn_uint8 &val) const {
    if ( !d_valid || (c < d_cMin) || (c > d_cMax) || (r < d_rMin) || (r > d_rMax) ) {
      fprintf(stderr, "vrpn_TempImager_Region::read_unscaled_pixel(): Invalid region or out of range\n");
      return false;
    } else {
      if (d_valType != vrpn_IMAGER_VALTYPE_UINT8) {
	fprintf(stderr, "XXX vrpn_TempImager_Region::read_unscaled_pixel(): Transcoding not implemented yet\n");
	return false;
      } else {
	val = ((vrpn_uint8 *)d_valBuf)[(c - d_cMin) + (r - d_rMin)*(d_cMax - d_cMin+1)];
      }
    }
    return true;
  }

  /// Reads pixel from the region with no scale and offset applied to the value.  Not
  /// the most efficient way to read the pixels out -- use the block read routines.
  inline  bool	read_unscaled_pixel(vrpn_uint16 c, vrpn_uint16 r, vrpn_uint16 &val) const {
    if ( !d_valid || (c < d_cMin) || (c > d_cMax) || (r < d_rMin) || (r > d_rMax) ) {
      fprintf(stderr, "vrpn_TempImager_Region::read_unscaled_pixel(): Invalid region or out of range\n");
      return false;
    } else {
      if (d_valType != vrpn_IMAGER_VALTYPE_UINT16) {
	fprintf(stderr, "XXX vrpn_TempImager_Region::read_unscaled_pixel(): Transcoding not implemented yet\n");
	return false;
      } else if (vrpn_big_endian) {
	fprintf(stderr, "XXX vrpn_TempImager_Region::read_unscaled_pixel(): Not implemented on big-endian yet\n");
	return false;
      } else {
	val = ((vrpn_uint16 *)d_valBuf)[(c - d_cMin) + (r - d_rMin)*(d_cMax - d_cMin+1)];
      }
    }
    return true;
  }

  /// Reads pixel from the region with no scale and offset applied to the value.  Not
  /// the most efficient way to read the pixels out -- use the block read routines.
  inline  bool	read_unscaled_pixel(vrpn_uint16 c, vrpn_uint16 r, vrpn_float32 &val) const {
    if ( !d_valid || (c < d_cMin) || (c > d_cMax) || (r < d_rMin) || (r > d_rMax) ) {
      fprintf(stderr, "vrpn_TempImager_Region::read_unscaled_pixel(): Invalid region or out of range\n");
      return false;
    } else {
      if (d_valType != vrpn_IMAGER_VALTYPE_FLOAT32) {
	fprintf(stderr, "XXX vrpn_TempImager_Region::read_unscaled_pixel(): Transcoding not implemented yet\n");
	return false;
      } else if (vrpn_big_endian) {
	fprintf(stderr, "XXX vrpn_TempImager_Region::read_unscaled_pixel(): Not implemented on big-endian yet\n");
	return false;
      } else {
	val = ((vrpn_float32 *)d_valBuf)[(c - d_cMin) + (r - d_rMin)*(d_cMax - d_cMin+1)];
      }
    }
    return true;
  }

  // Bulk read routines to copy the whole region right into user structures as
  // efficiently as possible.
  /// XXX Enable inversion of image in Y when reading
  bool	decode_unscaled_region_using_base_pointer(vrpn_uint8 *data,
    vrpn_uint32 colStride, vrpn_uint32 rowStride,
    vrpn_uint32 nRows = 0, bool invert_y = false, unsigned repeat = 1) const;
  bool	decode_unscaled_region_using_base_pointer(vrpn_uint16 *data,
    vrpn_uint32 colStride, vrpn_uint32 rowStride,
    vrpn_uint32 nRows = 0, bool invert_y = false, unsigned repeat = 1) const;
  bool	decode_unscaled_region_using_base_pointer(vrpn_float32 *data,
    vrpn_uint32 colStride, vrpn_uint32 rowStride,
    vrpn_uint32 nRows = 0, bool invert_y = false, unsigned repeat = 1) const;

  vrpn_int16  d_chanIndex;	//< Which channel this region holds data for 
  vrpn_uint16 d_rMin, d_rMax;	//< Range of indices for the rows
  vrpn_uint16 d_cMin, d_cMax;	//< Range of indices for the columns

protected:
  const	void  *d_valBuf;	//< Pointer to the buffer of values
  vrpn_uint16 d_valType;	//< Type of the values in the buffer
  bool	      d_valid;		//< Tells whether the helper can be used.
};

/// This is the class users deal with: it tells the format and the region data when it arrives.
class VRPN_API vrpn_TempImager_Remote: public vrpn_TempImager {
public:
  vrpn_TempImager_Remote(const char *name, vrpn_Connection *c = NULL);

  /// Register a handler for when new data arrives (can look up info in object when this happens)
  virtual int register_region_handler(void *userdata, vrpn_IMAGERREGIONHANDLER handler);
  virtual int unregister_region_handler(void *userdata, vrpn_IMAGERREGIONHANDLER handler);

  /// Register a handler for when the object's description changes (if desired)
  virtual int register_description_handler(void *userdata, vrpn_IMAGERDESCRIPTIONHANDLER handler);
  virtual int unregister_description_handler(void *userdata, vrpn_IMAGERDESCRIPTIONHANDLER handler);

  /// Call this each time through the program's main loop
  virtual void	mainloop(void);

  /// Accessors for the member variables: can be queried in the handler for object changes
  vrpn_int32	nRows(void) const { return d_nRows; };
  vrpn_int32	nCols(void) const { return d_nCols; };
  vrpn_float32	minX(void) const { return d_minX; };
  vrpn_float32	maxX(void) const { return d_maxX; };
  vrpn_float32	minY(void) const { return d_minY; };
  vrpn_float32	maxY(void) const { return d_maxY; };
  vrpn_int32	nChannels(void) const { return d_nChannels; };
  const vrpn_TempImager_Channel *channel(unsigned chanNum) const;

protected:
  bool	  d_got_description;	//< Have we gotten a description yet?
  // Lists to keep track of registered user handlers.
  typedef struct vrpn_TIDCS {
      void			    *userdata;
      vrpn_IMAGERDESCRIPTIONHANDLER handler;
      struct vrpn_TIDCS		    *next;
  } vrpn_DESCRIPTIONLIST;
  vrpn_DESCRIPTIONLIST *d_description_list;

  typedef struct vrpn_TIRCS {
      void			    *userdata;
      vrpn_IMAGERREGIONHANDLER	    handler;
      struct vrpn_TIRCS		    *next;
  } vrpn_REGIONLIST;
  vrpn_REGIONLIST *d_region_list;

  /// Handler for description change message from the server.
  static int VRPN_CALLBACK handle_region_message(void *userdata, vrpn_HANDLERPARAM p);

  /// Handler for region change message from the server.
  static int VRPN_CALLBACK handle_description_message(void *userdata, vrpn_HANDLERPARAM p);

  /// Handler for connection dropped message
  static int VRPN_CALLBACK handle_connection_dropped_message(void *userdata, vrpn_HANDLERPARAM p);
};

#endif
