#include  <stdio.h>
#include  <string.h>
#include  "vrpn_TempImager.h"

vrpn_TempImager::vrpn_TempImager(const char *name, vrpn_Connection *c) :
  vrpn_BaseClass(name, c),
  _nRows(0), _nCols(0),
  _minX(0), _maxX(0),
  _minY(0), _maxY(0),
  _nChannels(0)
{
    vrpn_BaseClass::init();
}

int vrpn_TempImager::register_types(void)
{
  _description_m_id = d_connection->register_message_type("vrpn_TempImager Description");
  _regionu8_m_id = d_connection->register_message_type("vrpn_TempImager Regionu8");
  _regionu16_m_id = d_connection->register_message_type("vrpn_TempImager Regionu16");
  _regionf32_m_id = d_connection->register_message_type("vrpn_TempImager Regionf32");
  if ((_description_m_id == -1) ||
      (_regionu8_m_id == -1) ||
      (_regionu16_m_id == -1) ||
      (_regionf32_m_id == -1) ) {
    return -1;
  } else {
    return 0;
  }
}

vrpn_TempImager_Server::vrpn_TempImager_Server(const char *name, vrpn_Connection *c,
			 vrpn_int32 nCols, vrpn_int32 nRows,
			 vrpn_float32 minX, vrpn_float32 maxX,
			 vrpn_float32 minY, vrpn_float32 maxY) :
    vrpn_TempImager(name, c),
    _description_sent(false)
{
    _nRows = nRows;
    _nCols = nCols;
    _minX = minX; _maxX = maxX;
    _minY = minY; _maxY = maxY;

    // Set up callback handler for ping message from client so that it
    // sends the description.  This will make sure that the other side has
    // heard the descrption before it hears a region message.  Also set this up
    // to fire on the "new connection" system message.

    register_autodeleted_handler(d_ping_message_id, handle_ping_message, this, d_sender_id);
    register_autodeleted_handler(d_connection->register_message_type(vrpn_got_connection), handle_ping_message, this);
}

int vrpn_TempImager_Server::add_channel(const char *name, const char *units,
		    vrpn_float32 minVal, vrpn_float32 maxVal,
		    vrpn_float32 scale, vrpn_float32 offset)
{
  if (_nChannels >= vrpn_IMAGER_MAX_CHANNELS) {
    return -1;
  }
  strncpy(_channels[_nChannels].name, name, sizeof(cName));
  strncpy(_channels[_nChannels].units, units, sizeof(cName));
  _channels[_nChannels].minVal = minVal;
  _channels[_nChannels].maxVal = maxVal;
  if (scale == 0) {
    fprintf(stderr,"vrpn_TempImager_Server::add_channel(): Scale was zero, set to 1\n");
    scale = 1;
  }
  _channels[_nChannels].scale = scale;
  _channels[_nChannels].offset = offset;
  _nChannels++;

  // We haven't set a proper description now
  _description_sent = false;
  return _nChannels-1;
}

/** As efficiently as possible, pull the values out of the array whose pointer is passed
    in and send them over a VRPN connection as a region message that is appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (0,0) element of the 2D array that holds the image.
	    It is assumed that the data values should be copied exactly, and that
	    offset and scale should be applied at the reading end by the user if
	    they want to get them back into absolute units.
      colStride, rowStride: Number of elements (size of data type) to skip along a column and row
      nRows, invert_y: If invert_y is true, then nRows must be set to the number of rows
	    in the entire image.  If invert_y is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer for "now")
*/

bool  vrpn_TempImager_Server::send_region_using_base_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_uint8 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride, vrpn_uint16 nRows, bool invert_y,
		    const struct timeval *time)
{
  // msgbuf must be float64-aligned!  It is the buffer to send to the client; static to avoid reallocating
  static  vrpn_float64 fbuf [vrpn_CONNECTION_TCP_BUFLEN/sizeof(vrpn_float64)];
  char	  *msgbuf = (char *) fbuf;
  int	  buflen = sizeof(fbuf);
  struct  timeval timestamp;

  // Make sure the region request has a valid channel, has indices all
  // within the image size, and is not too large to fit into the data
  // array (which is sized the way it is to make sure it can be sent in
  // one VRPN reliable message).
  if ( (chanIndex < 0) || (chanIndex >= _nChannels) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid channel index (%d)\n", chanIndex);
    return false;
  }
  if ( (rMax >= _nRows) || (rMin > rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid row range (%d..%d)\n", rMin, rMax);
    return false;
  }
  if ( (cMax >= _nCols) || (cMin > cMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid column range (%d..%d)\n", cMin, cMax);
    return false;
  }
  if ( (rMax-rMin+1) * (cMax-cMin+1) * sizeof(vrpn_uint8) > vrpn_IMAGER_MAX_REGIONu8) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Region to large\n");
    return false;
  }
  if ( invert_y && (nRows < rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): nRows must not be less than rMax\n");
    return false;
  }

  // If the user didn't specify a time, assume they want "now" and look it up.
  if (time != NULL) {
    timestamp = *time;
  } else {
    gettimeofday(&timestamp, NULL);
  }

  // Tell which channel this region is for, and what the borders of the
  // region are.
  if (vrpn_buffer(&msgbuf, &buflen, chanIndex) ||
      vrpn_buffer(&msgbuf, &buflen, rMin) ||
      vrpn_buffer(&msgbuf, &buflen, rMax) ||
      vrpn_buffer(&msgbuf, &buflen, cMin) ||
      vrpn_buffer(&msgbuf, &buflen, cMax) ||
      vrpn_buffer(&msgbuf, &buflen, vrpn_IMAGER_VALTYPE_UINT8) ) {
    return false;
  }

  // Insert the data into the buffer, copying it as efficiently as possible
  // from the caller's buffer into the buffer we are going to send.  Note that
  // the send buffer is going to be little-endian.  The code looks a little
  // complicated because it short-circuits the copying for the case where the
  // column stride is one element long (using memcpy() on each row) but has to
  // copy one element at a time otherwise.
  // There is also the matter if deciding whether to invert the image in y,
  // which complicates the index calculation and the calculation of the strides.
  int cols = cMax-cMin+1;
  int linelen = cols * sizeof(data[0]);
  if (colStride == 1) {
    for (unsigned r = rMin; r <= rMax; r++) {
      unsigned rActual;
      if (invert_y) {
	rActual = (nRows-1)-r;
      } else {
	rActual = r;
      }
      if (buflen < linelen) {
	return false;
      }
      memcpy(msgbuf, &data[rActual*rowStride+cMin], linelen);
      msgbuf += linelen;
      buflen -= linelen;
    }
  } else {
    const vrpn_uint8 *rowStart = &data[rMin*rowStride + cMin];
    if (invert_y) {
      rowStart = &data[(nRows-1-rMin)*rowStride + cMin];
      rowStride *= -1;
    }
    const vrpn_uint8 *copyFrom = rowStart;
    if (buflen < (int)((rMax-rMin+1)*(cMax-cMin+1)*sizeof(*copyFrom))) {
      return false;
    }
    for (unsigned r = rMin; r <= rMax; r++) {
      for (unsigned c = cMin; c <= cMax; c++) {
	*msgbuf = *copyFrom;	//< Copy the current element
	msgbuf++;		//< Skip to the next buffer location
	copyFrom += colStride;	//< Skip appropriate number of elements
      }
      rowStart += rowStride;	//< Skip to the start of the next row
      copyFrom = rowStart;
    }
    buflen -= (rMax-rMin+1)*(cMax-cMin+1)*sizeof(*copyFrom);
  }

  // No need to swap endian-ness on single-byte elements.

  // Pack the message
  vrpn_int32  len = sizeof(fbuf) - buflen;
  if (d_connection && d_connection->pack_message(len, timestamp,
                               _regionu8_m_id, d_sender_id, (char*)(void*)fbuf,
                               vrpn_CONNECTION_RELIABLE)) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): cannot write message: tossing\n");
    return false;
  }

  return true;
}

/** As efficiently as possible, pull the values out of the array whose pointer is passed
    in and send them over a VRPN connection as a region message that is appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (0,0) element of the 2D array that holds the image.
	    It is assumed that the data values should be copied exactly, and that
	    offset and scale should be applied at the reading end by the user if
	    they want to get them back into absolute units.
      colStride, rowStride: Number of elements (size of data type) to skip along a column and row
      nRows, invert_y: If invert_y is true, then nRows must be set to the number of rows
	    in the entire image.  If invert_y is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer for "now")
*/

bool  vrpn_TempImager_Server::send_region_using_base_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_uint16 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride,
		    vrpn_uint16 nRows, bool invert_y,
		    const struct timeval *time)
{
  // msgbuf must be float64-aligned!  It is the buffer to send to the client
  static  vrpn_float64 fbuf [vrpn_CONNECTION_TCP_BUFLEN/sizeof(vrpn_float64)];
  char	  *msgbuf = (char *) fbuf;
  int	  buflen = sizeof(fbuf);
  struct  timeval timestamp;

  // Make sure the region request has a valid channel, has indices all
  // within the image size, and is not too large to fit into the data
  // array (which is sized the way it is to make sure it can be sent in
  // one VRPN reliable message).
  if ( (chanIndex < 0) || (chanIndex >= _nChannels) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid channel index (%d)\n", chanIndex);
    return false;
  }
  if ( (rMax >= _nRows) || (rMin > rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid row range (%d..%d)\n", rMin, rMax);
    return false;
  }
  if ( (cMax >= _nCols) || (cMin > cMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid column range (%d..%d)\n", cMin, cMax);
    return false;
  }
  if ( (rMax-rMin+1) * (cMax-cMin+1) * sizeof(vrpn_uint16) > vrpn_IMAGER_MAX_REGIONu16) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Region to large\n");
    return false;
  }
  if ( invert_y && (nRows < rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): nRows must not be less than rMax\n");
    return false;
  }

  // If the user didn't specify a time, assume they want "now" and look it up.
  if (time != NULL) {
    timestamp = *time;
  } else {
    gettimeofday(&timestamp, NULL);
  }

  // Tell which channel this region is for, and what the borders of the
  // region are.
  if (vrpn_buffer(&msgbuf, &buflen, chanIndex) ||
      vrpn_buffer(&msgbuf, &buflen, rMin) ||
      vrpn_buffer(&msgbuf, &buflen, rMax) ||
      vrpn_buffer(&msgbuf, &buflen, cMin) ||
      vrpn_buffer(&msgbuf, &buflen, cMax) ||
      vrpn_buffer(&msgbuf, &buflen, vrpn_IMAGER_VALTYPE_UINT16) ) {
    return false;
  }

  // Insert the data into the buffer, copying it as efficiently as possible
  // from the caller's buffer into the buffer we are going to send.  Note that
  // the send buffer is going to be little-endian.  The code looks a little
  // complicated because it short-circuits the copying for the case where the
  // column stride is one element long (using memcpy() on each row) but has to
  // copy one element at a time otherwise.
  // There is also the matter if deciding whether to invert the image in y,
  // which complicates the index calculation and the calculation of the strides.
  int cols = cMax-cMin+1;
  int linelen = cols * sizeof(data[0]);
  if (colStride == 1) {
    for (unsigned r = rMin; r <= rMax; r++) {
      unsigned rActual;
      if (invert_y) {
	rActual = (nRows-1)-r;
      } else {
	rActual = r;
      }
      if (buflen < linelen) {
	return false;
      }
      memcpy(msgbuf, &data[rActual*rowStride+cMin], linelen);
      msgbuf += linelen;
      buflen -= linelen;
    }
  } else {
    const vrpn_uint16 *rowStart = &data[rMin*rowStride + cMin];
    if (invert_y) {
      rowStart = &data[(nRows-1-rMin)*rowStride + cMin];
      rowStride *= -1;
    }
    const vrpn_uint16 *copyFrom = rowStart;
    if (buflen < (int)((rMax-rMin+1)*(cMax-cMin+1)*sizeof(*copyFrom))) {
      return false;
    }
    for (unsigned r = rMin; r <= rMax; r++) {
      for (unsigned c = cMin; c <= cMax; c++) {
	memcpy(msgbuf, copyFrom, sizeof(*copyFrom));
	msgbuf+= sizeof(*copyFrom); //< Skip to the next buffer location
	copyFrom += colStride;	    //< Skip appropriate number of elements
      }
      rowStart += rowStride;	//< Skip to the start of the next row
      copyFrom = rowStart;
    }
    buflen -= (rMax-rMin+1)*(cMax-cMin+1)*sizeof(*copyFrom);
  }

  // Swap endian-ness of the buffer if we are on a big-endian machine.
  if (vrpn_big_endian) {
    fprintf(stderr, "XXX TempImager Region needs swapping on Big-endian\n");
    return false;
  }

  // Pack the message
  vrpn_int32  len = sizeof(fbuf) - buflen;
  if (d_connection && d_connection->pack_message(len, timestamp,
                               _regionu16_m_id, d_sender_id, (char*)(void*)fbuf,
                               vrpn_CONNECTION_RELIABLE)) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): cannot write message: tossing\n");
    return false;
  }

  return true;
}

/** As efficiently as possible, pull the values out of the array whose pointer is passed
    in and send them over a VRPN connection as a region message that is appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (0,0) element of the 2D array that holds the image.
	    It is assumed that the data values should be copied exactly, and that
	    offset and scale should be applied at the reading end by the user if
	    they want to get them back into absolute units.
      colStride, rowStride: Number of elements (size of data type) to skip along a column and row
      nRows, invert_y: If invert_y is true, then nRows must be set to the number of rows
	    in the entire image.  If invert_y is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer for "now")
*/

bool  vrpn_TempImager_Server::send_region_using_base_pointer(vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax,
		    vrpn_uint16 rMin, vrpn_uint16 rMax, const vrpn_float32 *data,
		    vrpn_uint32	colStride, vrpn_uint32 rowStride, vrpn_uint16 nRows, bool invert_y,
		    const struct timeval *time)
{
  // msgbuf must be float64-aligned!  It is the buffer to send to the client
  static  vrpn_float64 fbuf [vrpn_CONNECTION_TCP_BUFLEN/sizeof(vrpn_float64)];
  char	  *msgbuf = (char *) fbuf;
  int	  buflen = sizeof(fbuf);
  struct  timeval timestamp;

  // Make sure the region request has a valid channel, has indices all
  // within the image size, and is not too large to fit into the data
  // array (which is sized the way it is to make sure it can be sent in
  // one VRPN reliable message).
  if ( (chanIndex < 0) || (chanIndex >= _nChannels) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid channel index (%d)\n", chanIndex);
    return false;
  }
  if ( (rMax >= _nRows) || (rMin > rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid row range (%d..%d)\n", rMin, rMax);
    return false;
  }
  if ( (cMax >= _nCols) || (cMin > cMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Invalid column range (%d..%d)\n", cMin, cMax);
    return false;
  }
  if ( (rMax-rMin+1) * (cMax-cMin+1) * sizeof(vrpn_float32) > vrpn_IMAGER_MAX_REGIONf32) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): Region to large\n");
    return false;
  }
  if ( invert_y && (nRows < rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): nRows must not be less than rMax\n");
    return false;
  }

  // If the user didn't specify a time, assume they want "now" and look it up.
  if (time != NULL) {
    timestamp = *time;
  } else {
    gettimeofday(&timestamp, NULL);
  }

  // Tell which channel this region is for, and what the borders of the
  // region are.
  if (vrpn_buffer(&msgbuf, &buflen, chanIndex) ||
      vrpn_buffer(&msgbuf, &buflen, rMin) ||
      vrpn_buffer(&msgbuf, &buflen, rMax) ||
      vrpn_buffer(&msgbuf, &buflen, cMin) ||
      vrpn_buffer(&msgbuf, &buflen, cMax) ||
      vrpn_buffer(&msgbuf, &buflen, vrpn_IMAGER_VALTYPE_FLOAT32) )  {
    return false;
  }

  // Insert the data into the buffer, copying it as efficiently as possible
  // from the caller's buffer into the buffer we are going to send.  Note that
  // the send buffer is going to be little-endian.  The code looks a little
  // complicated because it short-circuits the copying for the case where the
  // column stride is one element long (using memcpy() on each row) but has to
  // copy one element at a time otherwise.
  // There is also the matter if deciding whether to invert the image in y,
  // which complicates the index calculation and the calculation of the strides.
  int cols = cMax-cMin+1;
  int linelen = cols * sizeof(data[0]);
  if (colStride == 1) {
    for (unsigned r = rMin; r <= rMax; r++) {
      unsigned rActual;
      if (invert_y) {
	rActual = (nRows-1)-r;
      } else {
	rActual = r;
      }
      if (buflen < linelen) {
	return false;
      }
      memcpy(msgbuf, &data[rActual*rowStride+cMin], linelen);
      msgbuf += linelen;
      buflen -= linelen;
    }
  } else {
    const vrpn_float32 *rowStart = &data[rMin*rowStride + cMin];
    if (invert_y) {
      rowStart = &data[(nRows-1-rMin)*rowStride + cMin];
      rowStride *= -1;
    }
    const vrpn_float32 *copyFrom = rowStart;
    if (buflen < (int)((rMax-rMin+1)*(cMax-cMin+1)*sizeof(*copyFrom))) {
      return false;
    }
    for (unsigned r = rMin; r <= rMax; r++) {
      for (unsigned c = cMin; c <= cMax; c++) {
	memcpy(msgbuf, copyFrom, sizeof(*copyFrom));
	msgbuf+= sizeof(*copyFrom); //< Skip to the next buffer location
	copyFrom += colStride;	    //< Skip appropriate number of elements
      }
      rowStart += rowStride;	//< Skip to the start of the next row
      copyFrom = rowStart;
    }
    buflen -= (rMax-rMin+1)*(cMax-cMin+1)*sizeof(*copyFrom);
  }

  // Swap endian-ness of the buffer if we are on a big-endian machine.
  if (vrpn_big_endian) {
    fprintf(stderr, "XXX TempImager Region needs swapping on Big-endian\n");
    return false;
  }

  // Pack the message
  vrpn_int32  len = sizeof(fbuf) - buflen;
  if (d_connection && d_connection->pack_message(len, timestamp,
                               _regionf32_m_id, d_sender_id, (char*)(void*)fbuf,
                               vrpn_CONNECTION_RELIABLE)) {
    fprintf(stderr,"vrpn_TempImager_Server::send_region_using_base_pointer(): cannot write message: tossing\n");
    return false;
  }

  return true;
}


bool  vrpn_TempImager_Server::send_description(void)
{
  // msgbuf must be float64-aligned!
  static  vrpn_float64 fbuf [vrpn_CONNECTION_TCP_BUFLEN/sizeof(vrpn_float64)];
  char	  *msgbuf = (char *) fbuf;
  int	  buflen = sizeof(fbuf);
  struct  timeval timestamp;
  int	  i;

  // Pack the description of all of the fields in the imager into the buffer,
  // including the channel descriptions.
  if (vrpn_buffer(&msgbuf, &buflen, _minX) ||
      vrpn_buffer(&msgbuf, &buflen, _maxX) ||
      vrpn_buffer(&msgbuf, &buflen, _minY) ||
      vrpn_buffer(&msgbuf, &buflen, _maxY) ||
      vrpn_buffer(&msgbuf, &buflen, _nRows) ||
      vrpn_buffer(&msgbuf, &buflen, _nCols) ||
      vrpn_buffer(&msgbuf, &buflen, _nChannels) ) {
    fprintf(stderr,"vrpn_TempImager_Server::send_description(): Can't pack message header, tossing\n");
    return false;
  }
  for (i = 0; i < _nChannels; i++) {
    if (!_channels[i].buffer(&msgbuf, &buflen)) {
      fprintf(stderr,"vrpn_TempImager_Server::send_description(): Can't pack message channel, tossing\n");
      return false;
    }
  }

  // Pack the buffer into the connection's outgoing reliable queue, if we have
  // a valid connection.
  vrpn_int32  len = sizeof(fbuf) - buflen;
  gettimeofday(&timestamp, NULL);
  if (d_connection && d_connection->pack_message(len, timestamp,
                               _description_m_id, d_sender_id, (char *)(void*)fbuf,
                               vrpn_CONNECTION_RELIABLE)) {
    fprintf(stderr,"vrpn_TempImager_Server::send_description(): cannot write message: tossing\n");
    return false;
  }

  _description_sent = true;
  return true;
}

void  vrpn_TempImager_Server::mainloop(void)
{
  server_mainloop();
}

int  vrpn_TempImager_Server::handle_ping_message(void *userdata, vrpn_HANDLERPARAM p)
{
  vrpn_TempImager_Server  *me = (vrpn_TempImager_Server*)userdata;
  me->send_description();
  return 0;
}

vrpn_TempImager_Remote::vrpn_TempImager_Remote(const char *name, vrpn_Connection *c) :
  vrpn_TempImager(name, c),
  _got_description(false),
  _region_list(NULL),
  _description_list(NULL)
{
  // Register the handlers for the description message and the region change messages
  register_autodeleted_handler(_description_m_id, handle_description_message, this, d_sender_id);

  // Register the region-handling messages for the different message types.
  // All of the types use the same handler, since the type of region is encoded
  // in one of the parameters of the region function.
  register_autodeleted_handler(_regionu8_m_id, handle_region_message, this, d_sender_id);
  register_autodeleted_handler(_regionu16_m_id, handle_region_message, this, d_sender_id);
  register_autodeleted_handler(_regionf32_m_id, handle_region_message, this, d_sender_id);

  // Register the handler for the connection dropped message
  register_autodeleted_handler(d_connection->register_message_type(vrpn_dropped_connection), handle_connection_dropped_message, this);
}

void  vrpn_TempImager_Remote::mainloop(void)
{
  client_mainloop();
  if (d_connection) { d_connection->mainloop(); };
}

const vrpn_TempImager_Channel *vrpn_TempImager_Remote::channel(unsigned chanNum) const
{
  if (chanNum >= (unsigned)_nChannels) {
    return NULL;
  }
  return &_channels[chanNum];
}

int vrpn_TempImager_Remote::register_description_handler(void *userdata,
			vrpn_IMAGERDESCRIPTIONHANDLER handler)
{
  vrpn_DESCRIPTIONLIST	*new_entry;

  // Ensure that the handler is non-NULL
  if (handler == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::register_description_handler: NULL handler\n");
    return -1;
  }

  // Allocate and initialize the new entry
  if ( (new_entry = new vrpn_DESCRIPTIONLIST) == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::register_description_handler: Out of memory\n");
    return -1;
  }
  new_entry->handler = handler;
  new_entry->userdata = userdata;

  // Add this handler to the chain at the beginning (don't check to see
  // if it is already there, since duplication is okay).
  new_entry->next = _description_list;
  _description_list = new_entry;

  _got_description = true;
  return 0;
}

int vrpn_TempImager_Remote::unregister_description_handler(void *userdata,
			vrpn_IMAGERDESCRIPTIONHANDLER handler)
{
  // The pointer at *snitch points to victim
  vrpn_DESCRIPTIONLIST	*victim, **snitch;

  // Find a handler with this registry in the list (any one will do,
  // since all duplicates are the same).
  snitch = &_description_list;
  victim = *snitch;
  while ( (victim != NULL) &&
	  ( (victim->handler != handler) ||
	    (victim->userdata != userdata) )) {
    snitch = &( (*snitch)->next );
    victim = victim->next;
  }

  // Make sure we found one
  if (victim == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::unregister_description_handler: No such handler\n");
    return -1;
  }

  // Remove the entry from the list
  *snitch = victim->next;
  delete victim;

  return 0;
}

int vrpn_TempImager_Remote::register_region_handler(void *userdata,
			vrpn_IMAGERREGIONHANDLER handler)
{
  vrpn_REGIONLIST	*new_entry;

  // Ensure that the handler is non-NULL
  if (handler == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::register_region_handler: NULL handler\n");
    return -1;
  }

  // Allocate and initialize the new entry
  if ( (new_entry = new vrpn_REGIONLIST) == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::register_region_handler: Out of memory\n");
    return -1;
  }
  new_entry->handler = handler;
  new_entry->userdata = userdata;

  // Add this handler to the chain at the beginning (don't check to see
  // if it is already there, since duplication is okay).
  new_entry->next = _region_list;
  _region_list = new_entry;

  return 0;
}

int vrpn_TempImager_Remote::unregister_region_handler(void *userdata,
			vrpn_IMAGERREGIONHANDLER handler)
{
  // The pointer at *snitch points to victim
  vrpn_REGIONLIST	*victim, **snitch;

  // Find a handler with this registry in the list (any one will do,
  // since all duplicates are the same).
  snitch = &_region_list;
  victim = *snitch;
  while ( (victim != NULL) &&
	  ( (victim->handler != handler) ||
	    (victim->userdata != userdata) )) {
    snitch = &( (*snitch)->next );
    victim = victim->next;
  }

  // Make sure we found one
  if (victim == NULL) {
    fprintf(stderr, "vrpn_TempImager_Remote::unregister_region_handler: No such handler\n");
    return -1;
  }

  // Remove the entry from the list
  *snitch = victim->next;
  delete victim;

  return 0;
}

int vrpn_TempImager_Remote::handle_description_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
  const char *bufptr = p.buffer;
  vrpn_TempImager_Remote *me = (vrpn_TempImager_Remote *)userdata;
  vrpn_DESCRIPTIONLIST *handler = me->_description_list;
  int i;

  // Get my new information from the buffer
  if (vrpn_unbuffer(&bufptr, &me->_minX) ||
      vrpn_unbuffer(&bufptr, &me->_maxX) ||
      vrpn_unbuffer(&bufptr, &me->_minY) ||
      vrpn_unbuffer(&bufptr, &me->_maxY) ||
      vrpn_unbuffer(&bufptr, &me->_nRows) ||
      vrpn_unbuffer(&bufptr, &me->_nCols) ||
      vrpn_unbuffer(&bufptr, &me->_nChannels) ) {
    return -1;
  }
  for (i = 0; i < me->_nChannels; i++) {
    if (!me->_channels[i].unbuffer(&bufptr)) {
      return -1;
    }
  }

  // Go down the list of callbacks that have been registered.
  // Fill in the parameter and call each.
  while (handler != NULL) {
	  handler->handler(handler->userdata, p.msg_time);
	  handler = handler->next;
  }

  return 0;
}

int vrpn_TempImager_Remote::handle_region_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
  const char *bufptr = p.buffer;
  vrpn_TempImager_Remote *me = (vrpn_TempImager_Remote *)userdata;
  vrpn_REGIONLIST *handler = me->_region_list;
  vrpn_IMAGERREGIONCB rp;
  vrpn_TempImager_Region  reg;

  // Create an instance of a region helper class and read its
  // parameters from the buffer (setting its _valBuf pointer at
  // the start of the data in the buffer).  Set it to valid and then
  // call the user callback and then set it to invalid before
  // deleting it.
  if (vrpn_unbuffer(&bufptr, &reg._chanIndex) ||
      vrpn_unbuffer(&bufptr, &reg._rMin) ||
      vrpn_unbuffer(&bufptr, &reg._rMax) ||
      vrpn_unbuffer(&bufptr, &reg._cMin) ||
      vrpn_unbuffer(&bufptr, &reg._cMax) ||
      vrpn_unbuffer(&bufptr, &reg._valType) )  {
    fprintf(stderr, "vrpn_TempImager_Remote::handle_region_message(): Can't unbuffer parameters!\n");
    return -1;
  }
  reg._valBuf = bufptr;
  reg._valid = true;

  // Fill in a user callback structure with the data
  rp.msg_time = p.msg_time;
  rp.region = &reg;

  // ONLY if we have gotten a description message,
  // Go down the list of callbacks that have been registered.
  // Fill in the parameter and call each.
  if (me->_got_description) while (handler != NULL) {
	  handler->handler(handler->userdata, rp);
	  handler = handler->next;
  }

  reg._valid = false;
  return 0;
}

int vrpn_TempImager_Remote::handle_connection_dropped_message(void *userdata,
	vrpn_HANDLERPARAM p)
{
  vrpn_TempImager_Remote *me = (vrpn_TempImager_Remote *)userdata;

  // We have no description message, so don't call region callbacks
  me->_got_description = false;

  return 0;
}

/** As efficiently as possible, pull the values out of the VRPN buffer and put them into
    the array whose pointer is passed in, transcoding as needed to convert it into the
    type of the pointer passed in.
      data: Points at the (0,0) element of the 2D array that is to hold the image.
	    It is assumed that the data values should be copied exactly, and that
	    offset and scale should be applied at the reading end by the user if
	    they want to get them back into absolute units.
      repeat: How many times to copy each element (for example, this will be 3 if we are
	      copying an unsigned 8-bit value from the network into the Red, Green, and Blue
	      entries of a color map.
      colStride, rowStride: Number of elements (size of data type) to skip along a column and row.
	      This tells how far to jump ahead when writing the next element in the row (must
	      be larger or the same as the repeat) and how make elements to skip to get to the
	      beginning of the next column.  For example, copying a uint from the network into
	      an RGBA texture would have repeat=3, colStride=4,
	      and rowStride=4*(number of columns).
      numRows, invert_y: numRows needs to equal the number of rows in the entire image if
	      invert_y is true; it is not used otherwise.  invert_y inverts the image in y
	      as it is copying it into the user's buffer.
*/

bool  vrpn_TempImager_Region::decode_unscaled_region_using_base_pointer(vrpn_uint8 *data,
    vrpn_uint32 colStride, vrpn_uint32 rowStride, vrpn_uint32 nRows, bool invert_y,
    unsigned repeat) const
{
  // Make sure the parameters are reasonable
  if (colStride < repeat) {
    fprintf(stderr,"vrpn_TempImager_Region::decode_unscaled_region_using_base_pointer(): colStride must be >= repeat\n");
    return false;
  }

  // If the type of data in the buffer doesn't match the type of data the user
  // wants, we need to convert each element along the way.
  if (_valType != vrpn_IMAGER_VALTYPE_UINT8) {
    printf("vrpn_TempImager_Region::decode_unscaled_region_using_base_pointer(): Transcoding not implemented yet\n");
    // No need to swap endianness on single-byte entities.
    return false;
  }
  if ( invert_y && (nRows < _rMax) ) {
    fprintf(stderr,"vrpn_TempImager_Region::decode_unscaled_region_using_base_pointer(): nRows must not be less than _rMax\n");
    return false;
  }

  // The data type matches what we the user is asking for.  No transcoding needed.
  // Insert the data into the buffer, copying it as efficiently as possible
  // from the network buffer into the caller's buffer .  Note that
  // the network buffer is little-endian.  The code looks a little
  // complicated because it short-circuits the copying for the case where the
  // column stride and repeat are one element long (using memcpy() on each row) but has to
  // copy one element at a time otherwise.
  int cols = _cMax - _cMin+1;
  int linelen = cols * sizeof(data[0]);
  if ( (colStride == 1) && (repeat == 1) ) {
    const vrpn_uint8  *msgbuf = (const vrpn_uint8 *)_valBuf;
    for (unsigned r = _rMin; r <= _rMax; r++) {
      unsigned rActual;
      if (invert_y) {
	rActual = (nRows-1)-r;
      } else {
	rActual = r;
      }
      memcpy(&data[rActual*rowStride+_cMin], msgbuf, linelen);
      msgbuf += linelen;
    }
  } else {
    const vrpn_uint8  *msgbuf = (const vrpn_uint8 *)_valBuf;
    vrpn_uint8 *rowStart = &data[_rMin*rowStride + _cMin];
    if (invert_y) {
      rowStart = &data[(nRows-1 - _rMin)*rowStride + _cMin];
      rowStride *= -1;
    }
    vrpn_uint8 *copyTo = rowStart;
    for (unsigned r = _rMin; r <= _rMax; r++) {
      for (unsigned c = _cMin; c <= _cMax; c++) {
	for (unsigned rpt = 0; rpt < repeat; rpt++) {
	  *(copyTo+rpt) = *msgbuf;  //< Copy the current element
	}
	msgbuf++;		    //< Skip to the next buffer location
	copyTo += colStride;	    //< Skip appropriate number of elements
      }
      rowStart += rowStride;	//< Skip to the start of the next row
      copyTo = rowStart;
    }
  }

  // No need to swap endianness on single-byte entities.
  return true;
}
