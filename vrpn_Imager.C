#include <stdio.h>  // for fprintf, stderr, printf
#include <string.h> // for memcpy, NULL

#include "vrpn_Imager.h"

vrpn_Imager::vrpn_Imager(const char *name, vrpn_Connection *c)
    : vrpn_BaseClass(name, c)
    , d_nRows(0)
    , d_nCols(0)
    , d_nDepth(0)
    , d_nChannels(0)
{
    vrpn_BaseClass::init();
}

int vrpn_Imager::register_types(void)
{
    d_description_m_id =
        d_connection->register_message_type("vrpn_Imager Description");
    d_begin_frame_m_id =
        d_connection->register_message_type("vrpn_Imager Begin_Frame");
    d_end_frame_m_id =
        d_connection->register_message_type("vrpn_Imager End_Frame");
    d_discarded_frames_m_id =
        d_connection->register_message_type("vrpn_Imager Discarded_Frames");
    d_throttle_frames_m_id =
        d_connection->register_message_type("vrpn_Imager Throttle_Frames");
    d_regionu8_m_id =
        d_connection->register_message_type("vrpn_Imager Regionu8");
    d_regionu16_m_id =
        d_connection->register_message_type("vrpn_Imager Regionu16");
    d_regionu12in16_m_id =
        d_connection->register_message_type("vrpn_Imager Regionu12in16");
    d_regionf32_m_id =
        d_connection->register_message_type("vrpn_Imager Regionf32");
    if ((d_description_m_id == -1) || (d_regionu8_m_id == -1) ||
        (d_regionu16_m_id == -1) || (d_regionf32_m_id == -1) ||
        (d_begin_frame_m_id == -1) || (d_end_frame_m_id == -1) ||
        (d_throttle_frames_m_id == -1) || (d_discarded_frames_m_id == -1)) {
        return -1;
    }
    else {
        return 0;
    }
}

vrpn_Imager_Server::vrpn_Imager_Server(const char *name, vrpn_Connection *c,
                                       vrpn_int32 nCols, vrpn_int32 nRows,
                                       vrpn_int32 nDepth)
    : vrpn_Imager(name, c)
    , d_description_sent(false)
    , d_frames_to_send(-1)
    , d_dropped_due_to_throttle(0)
{
    d_nRows = nRows;
    d_nCols = nCols;
    d_nDepth = nDepth;

    // Set up callback handler for ping message from client so that it
    // sends the description.  This will make sure that the other side has
    // heard the descrption before it hears a region message.  Also set this up
    // to fire on the "new connection" system message.

    register_autodeleted_handler(d_ping_message_id, handle_ping_message, this,
                                 d_sender_id);
    register_autodeleted_handler(
        d_connection->register_message_type(vrpn_got_connection),
        handle_ping_message, this, vrpn_ANY_SENDER);

    // Set up a handler for the throttle message, which sets how many more
    // frames to send before dropping them.  Also set up a handler for the
    // last-connection-dropped message, which will cause throttling to go
    // back to -1.
    register_autodeleted_handler(d_throttle_frames_m_id,
                                 handle_throttle_message, this, d_sender_id);
    register_autodeleted_handler(
        d_connection->register_message_type(vrpn_dropped_last_connection),
        handle_last_drop_message, this, vrpn_ANY_SENDER);
}

int vrpn_Imager_Server::add_channel(const char *name, const char *units,
                                    vrpn_float32 minVal, vrpn_float32 maxVal,
                                    vrpn_float32 scale, vrpn_float32 offset)
{
    if (static_cast<unsigned>(d_nChannels) >= vrpn_IMAGER_MAX_CHANNELS) {
        return -1;
    }
    vrpn_strcpy(d_channels[d_nChannels].name, name);
    vrpn_strcpy(d_channels[d_nChannels].units, units);
    d_channels[d_nChannels].minVal = minVal;
    d_channels[d_nChannels].maxVal = maxVal;
    if (scale == 0) {
        fprintf(
            stderr,
            "vrpn_Imager_Server::add_channel(): Scale was zero, set to 1\n");
        scale = 1;
    }
    d_channels[d_nChannels].scale = scale;
    d_channels[d_nChannels].offset = offset;
    d_nChannels++;

    // We haven't sent a proper description now
    d_description_sent = false;
    return d_nChannels - 1;
}

bool vrpn_Imager_Server::send_begin_frame(
    const vrpn_uint16 cMin, const vrpn_uint16 cMax, const vrpn_uint16 rMin,
    const vrpn_uint16 rMax, const vrpn_uint16 dMin, const vrpn_uint16 dMax,
    const struct timeval *time)
{
    // msgbuf must be float64-aligned!  It is the buffer to send to the client
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = (char *)fbuf;
    int buflen = sizeof(fbuf);
    struct timeval timestamp;

    // If we are throttling frames and the frame count has gone to zero,
    // then increment the number of frames missed and return failure to
    // send.
    if (d_frames_to_send == 0) {
        d_dropped_due_to_throttle++;
        return false;
    }

    // If we missed some frames due to throttling, say so.
    if (d_dropped_due_to_throttle > 0) {
        send_discarded_frames(d_dropped_due_to_throttle);
        d_dropped_due_to_throttle = 0;
    }

    // If we are throttling, then decrement the number of frames
    // left to send.
    if (d_frames_to_send > 0) {
        d_frames_to_send--;
    }

    // Make sure the region request has indices all within the image size.
    if ((rMax >= d_nRows) || (rMin > rMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_begin_frame(): Invalid row "
                        "range (%d..%d)\n",
                rMin, rMax);
        return false;
    }
    if ((cMax >= d_nCols) || (cMin > cMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_begin_frame(): Invalid "
                        "column range (%d..%d)\n",
                cMin, cMax);
        return false;
    }
    if ((dMax >= d_nDepth) || (dMin > dMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_begin_frame(): Invalid depth "
                        "range (%d..%d)\n",
                dMin, dMax);
        return false;
    }

    // If the user didn't specify a time, assume they want "now" and look it up.
    if (time != NULL) {
        timestamp = *time;
    }
    else {
        vrpn_gettimeofday(&timestamp, NULL);
    }

    // Tell what the borders of the region are.
    if (vrpn_buffer(&msgbuf, &buflen, dMin) ||
        vrpn_buffer(&msgbuf, &buflen, dMax) ||
        vrpn_buffer(&msgbuf, &buflen, rMin) ||
        vrpn_buffer(&msgbuf, &buflen, rMax) ||
        vrpn_buffer(&msgbuf, &buflen, cMin) ||
        vrpn_buffer(&msgbuf, &buflen, cMax)) {
        return false;
    }

    // Pack the message
    vrpn_int32 len = sizeof(fbuf) - buflen;
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_begin_frame_m_id,
                                   d_sender_id, (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_Imager_Server::send_begin_frame(): cannot write "
                        "message: tossing\n");
        return false;
    }

    return true;
}

bool vrpn_Imager_Server::send_end_frame(
    const vrpn_uint16 cMin, const vrpn_uint16 cMax, const vrpn_uint16 rMin,
    const vrpn_uint16 rMax, const vrpn_uint16 dMin, const vrpn_uint16 dMax,
    const struct timeval *time)
{
    // msgbuf must be float64-aligned!  It is the buffer to send to the client
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = (char *)fbuf;
    int buflen = sizeof(fbuf);
    struct timeval timestamp;

    // If we are discarding frames, return failure to send.
    if (d_dropped_due_to_throttle > 0) {
        return false;
    }

    // Make sure the region request has indices all within the image size.
    if ((rMax >= d_nRows) || (rMin > rMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_end_frame(): Invalid row "
                        "range (%d..%d)\n",
                rMin, rMax);
        return false;
    }
    if ((cMax >= d_nCols) || (cMin > cMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_end_frame(): Invalid column "
                        "range (%d..%d)\n",
                cMin, cMax);
        return false;
    }
    if ((dMax >= d_nDepth) || (dMin > dMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_end_frame(): Invalid depth "
                        "range (%d..%d)\n",
                dMin, dMax);
        return false;
    }

    // If the user didn't specify a time, assume they want "now" and look it up.
    if (time != NULL) {
        timestamp = *time;
    }
    else {
        vrpn_gettimeofday(&timestamp, NULL);
    }

    // Tell what the borders of the region are.
    if (vrpn_buffer(&msgbuf, &buflen, dMin) ||
        vrpn_buffer(&msgbuf, &buflen, dMax) ||
        vrpn_buffer(&msgbuf, &buflen, rMin) ||
        vrpn_buffer(&msgbuf, &buflen, rMax) ||
        vrpn_buffer(&msgbuf, &buflen, cMin) ||
        vrpn_buffer(&msgbuf, &buflen, cMax)) {
        return false;
    }

    // Pack the message
    vrpn_int32 len = sizeof(fbuf) - buflen;
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_end_frame_m_id,
                                   d_sender_id, (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_Imager_Server::send_end_frame(): cannot write "
                        "message: tossing\n");
        return false;
    }

    return true;
}

bool vrpn_Imager_Server::send_discarded_frames(const vrpn_uint16 count,
                                               const struct timeval *time)
{
    // msgbuf must be float64-aligned!  It is the buffer to send to the client
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = (char *)fbuf;
    int buflen = sizeof(fbuf);
    struct timeval timestamp;

    // If the user didn't specify a time, assume they want "now" and look it up.
    if (time != NULL) {
        timestamp = *time;
    }
    else {
        vrpn_gettimeofday(&timestamp, NULL);
    }

    // Tell how many frames were skipped.
    if (vrpn_buffer(&msgbuf, &buflen, count)) {
        return false;
    }

    // Pack the message
    vrpn_int32 len = sizeof(fbuf) - buflen;
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_discarded_frames_m_id,
                                   d_sender_id, (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_Imager_Server::send_discarded_frames(): cannot "
                        "write message: tossing\n");
        return false;
    }

    return true;
}

// XXX Re-cast the memcpy loop in terms of offsets (the same way the per-element
// loop is done) and share base calculation between the two; move the if
// statement
// into the inner loop, doing it memcpy or step-at-a-time.

/** As efficiently as possible, pull the values out of the array whose pointer
   is passed
    in and send them over a VRPN connection as a region message that is
   appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (0,0) element of the 2D array that holds the image.
        It is assumed that the data values should be copied exactly, and that
        offset and scale should be applied at the reading end by the user if
        they want to get them back into absolute units.
      colStride, rowStride, depthStride: Number of elements (size of data type)
   to skip along a column, row, and depth
      nRows, invert_rows: If invert_rows is true, then nRows must be set to the
   number of rows
        in the entire image.  If invert_rows is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer
   for "now")
*/

bool vrpn_Imager_Server::send_region_using_base_pointer(
    vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax, vrpn_uint16 rMin,
    vrpn_uint16 rMax, const vrpn_uint8 *data, vrpn_uint32 colStride,
    vrpn_uint32 rowStride, vrpn_uint16 nRows, bool invert_rows,
    vrpn_uint32 depthStride, vrpn_uint16 dMin, vrpn_uint16 dMax,
    const struct timeval *time)
{
    // msgbuf must be float64-aligned!  It is the buffer to send to the client;
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = reinterpret_cast<char *>(fbuf);
    int buflen = sizeof(fbuf);
    struct timeval timestamp;

    // If we are discarding frames, return failure to send.
    if (d_dropped_due_to_throttle > 0) {
        return false;
    }

    // Make sure the region request has a valid channel, has indices all
    // within the image size, and is not too large to fit into the data
    // array (which is sized the way it is to make sure it can be sent in
    // one VRPN reliable message).
    if ((chanIndex < 0) || (chanIndex >= d_nChannels)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid channel index (%d)\n",
                chanIndex);
        return false;
    }
    if ((dMax >= d_nDepth) || (dMin > dMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid depth range (%d..%d)\n",
                dMin, dMax);
        return false;
    }
    if ((rMax >= d_nRows) || (rMin > rMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid row range (%d..%d)\n",
                rMin, rMax);
        return false;
    }
    if ((cMax >= d_nCols) || (cMin > cMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid column range (%d..%d)\n",
                cMin, cMax);
        return false;
    }
    if (static_cast<unsigned>((rMax - rMin + 1) * (cMax - cMin + 1) *
                              (dMax - dMin + 1)) > vrpn_IMAGER_MAX_REGIONu8) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Region too large (%d,%d,%d to %d,%d,%d)\n",
                cMin, rMin, dMin, cMax, rMax, dMax);
        return false;
    }
    if (invert_rows && (nRows < rMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "nRows must not be less than rMax\n");
        return false;
    }

    // Make sure we've sent the description before we send any regions
    if (!d_description_sent) {
        send_description();
        d_description_sent = true;
    }

    // If the user didn't specify a time, assume they want "now" and look it up.
    if (time != NULL) {
        timestamp = *time;
    }
    else {
        vrpn_gettimeofday(&timestamp, NULL);
    }

    // Check the compression status and prepare to do compression if it is
    // called for.
    if (d_channels[chanIndex].d_compression != vrpn_Imager_Channel::NONE) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Compression not implemented\n");
        return false;
    }

    // Tell which channel this region is for, and what the borders of the
    // region are.
    if (vrpn_buffer(&msgbuf, &buflen, chanIndex) ||
        vrpn_buffer(&msgbuf, &buflen, dMin) ||
        vrpn_buffer(&msgbuf, &buflen, dMax) ||
        vrpn_buffer(&msgbuf, &buflen, rMin) ||
        vrpn_buffer(&msgbuf, &buflen, rMax) ||
        vrpn_buffer(&msgbuf, &buflen, cMin) ||
        vrpn_buffer(&msgbuf, &buflen, cMax) ||
        vrpn_buffer(&msgbuf, &buflen, vrpn_IMAGER_VALTYPE_UINT8)) {
        return false;
    }

    // Insert the data into the buffer, copying it as efficiently as possible
    // from the caller's buffer into the buffer we are going to send.  Note that
    // the send buffer is going to be little-endian.  The code looks a little
    // complicated because it short-circuits the copying for the case where the
    // column stride is one element long (using memcpy() on each row) but has to
    // copy one element at a time otherwise.
    // There is also the matter if deciding whether to invert the image in y,
    // which complicates the index calculation and the calculation of the
    // strides.
    int cols = cMax - cMin + 1;
    int linelen = cols * sizeof(data[0]);
    if (colStride == 1) {
        for (unsigned d = dMin; d <= dMax; d++) {
            for (unsigned r = rMin; r <= rMax; r++) {
                unsigned rActual;
                if (invert_rows) {
                    rActual = (nRows - 1) - r;
                }
                else {
                    rActual = r;
                }
                if (buflen < linelen) {
                    return false;
                }
                memcpy(msgbuf,
                       &data[d * depthStride + rActual * rowStride + cMin],
                       linelen);
                msgbuf += linelen;
                buflen -= linelen;
            }
        }
    }
    else {
        if (buflen < (int)((dMax - dMin + 1) * (rMax - rMin + 1) *
                           (cMax - cMin + 1) * sizeof(data[0]))) {
            return false;
        }
        long rowStep = rowStride;
        if (invert_rows) {
            rowStep *= -1;
        }
        for (unsigned d = dMin; d <= dMax; d++) {
            // XXX Turn the depth calculation into start and += like the others
            const vrpn_uint8 *rowStart =
                &data[d * depthStride + rMin * rowStride + cMin];
            if (invert_rows) {
                rowStart = &data[d * depthStride +
                                 (nRows - 1 - rMin) * rowStride + cMin];
            }
            const vrpn_uint8 *copyFrom = rowStart;
            for (unsigned r = rMin; r <= rMax; r++) {
                for (unsigned c = cMin; c <= cMax; c++) {
                    *reinterpret_cast<vrpn_uint8 *>(msgbuf) =
                        *copyFrom; //< Copy the current element
                    msgbuf++;      //< Skip to the next buffer location
                    copyFrom +=
                        colStride; //< Skip appropriate number of elements
                }
                rowStart += rowStep; //< Skip to the start of the next row
                copyFrom = rowStart;
            }
        }
        buflen -= (rMax - rMin + 1) * (cMax - cMin + 1) * sizeof(data[0]);
    }

    // No need to swap endian-ness on single-byte elements.

    // Pack the message
    vrpn_int32 len = sizeof(fbuf) - buflen;
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_regionu8_m_id, d_sender_id,
                                   (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "cannot write message: tossing\n");
        return false;
    }

    return true;
}

/** As efficiently as possible, pull the values out of the array whose pointer
   is passed
    in and send them over a VRPN connection as a region message that is
   appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (0,0) element of the 2D array that holds the image.
        It is assumed that the data values should be copied exactly, and that
        offset and scale should be applied at the reading end by the user if
        they want to get them back into absolute units.
      colStride, rowStride: Number of elements (size of data type) to skip along
   a column and row
      nRows, invert_rows: If invert_rows is true, then nRows must be set to the
   number of rows
        in the entire image.  If invert_rows is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer
   for "now")
*/

bool vrpn_Imager_Server::send_region_using_base_pointer(
    vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax, vrpn_uint16 rMin,
    vrpn_uint16 rMax, const vrpn_uint16 *data, vrpn_uint32 colStride,
    vrpn_uint32 rowStride, vrpn_uint16 nRows, bool invert_rows,
    vrpn_uint32 depthStride, vrpn_uint16 dMin, vrpn_uint16 dMax,
    const struct timeval *time)
{
    // msgbuf must be float64-aligned!  It is the buffer to send to the client
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = (char *)fbuf;
    int buflen = sizeof(fbuf);
    struct timeval timestamp;

    // If we are discarding frames, return failure to send.
    if (d_dropped_due_to_throttle > 0) {
        return false;
    }

    // Make sure the region request has a valid channel, has indices all
    // within the image size, and is not too large to fit into the data
    // array (which is sized the way it is to make sure it can be sent in
    // one VRPN reliable message).
    if ((chanIndex < 0) || (chanIndex >= d_nChannels)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid channel index (%d)\n",
                chanIndex);
        return false;
    }
    if ((dMax >= d_nDepth) || (dMin > dMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid depth range (%d..%d)\n",
                dMin, dMax);
        return false;
    }
    if ((rMax >= d_nRows) || (rMin > rMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid row range (%d..%d)\n",
                rMin, rMax);
        return false;
    }
    if ((cMax >= d_nCols) || (cMin > cMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid column range (%d..%d)\n",
                cMin, cMax);
        return false;
    }
    if (static_cast<unsigned>((rMax - rMin + 1) * (cMax - cMin + 1) *
                              (dMax - dMin + 1)) > vrpn_IMAGER_MAX_REGIONu16) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Region too large (%d,%d,%d to %d,%d,%d)\n",
                cMin, rMin, dMin, cMax, rMax, dMax);
        return false;
    }
    if (invert_rows && (nRows < rMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "nRows must not be less than rMax\n");
        return false;
    }

    // Make sure we've sent the description before we send any regions
    if (!d_description_sent) {
        send_description();
        d_description_sent = true;
    }

    // If the user didn't specify a time, assume they want "now" and look it up.
    if (time != NULL) {
        timestamp = *time;
    }
    else {
        vrpn_gettimeofday(&timestamp, NULL);
    }

    // Check the compression status and prepare to do compression if it is
    // called for.
    if (d_channels[chanIndex].d_compression != vrpn_Imager_Channel::NONE) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Compression not implemented\n");
        return false;
    }

    // Tell which channel this region is for, and what the borders of the
    // region are.
    if (vrpn_buffer(&msgbuf, &buflen, chanIndex) ||
        vrpn_buffer(&msgbuf, &buflen, dMin) ||
        vrpn_buffer(&msgbuf, &buflen, dMax) ||
        vrpn_buffer(&msgbuf, &buflen, rMin) ||
        vrpn_buffer(&msgbuf, &buflen, rMax) ||
        vrpn_buffer(&msgbuf, &buflen, cMin) ||
        vrpn_buffer(&msgbuf, &buflen, cMax) ||
        vrpn_buffer(&msgbuf, &buflen, vrpn_IMAGER_VALTYPE_UINT16)) {
        return false;
    }

    // Insert the data into the buffer, copying it as efficiently as possible
    // from the caller's buffer into the buffer we are going to send.  Note that
    // the send buffer is going to be little-endian.  The code looks a little
    // complicated because it short-circuits the copying for the case where the
    // column stride is one element long (using memcpy() on each row) but has to
    // copy one element at a time otherwise.
    // There is also the matter if deciding whether to invert the image in y,
    // which complicates the index calculation and the calculation of the
    // strides.
    int cols = cMax - cMin + 1;
    int linelen = cols * sizeof(data[0]);
    if (colStride == 1) {
        for (unsigned d = dMin; d <= dMax; d++) {
            for (unsigned r = rMin; r <= rMax; r++) {
                unsigned rActual;
                if (invert_rows) {
                    rActual = (nRows - 1) - r;
                }
                else {
                    rActual = r;
                }
                if (buflen < linelen) {
                    return false;
                }
                memcpy(msgbuf,
                       &data[d * depthStride + rActual * rowStride + cMin],
                       linelen);
                msgbuf += linelen;
                buflen -= linelen;
            }
        }
    }
    else {
        if (buflen < (int)((dMax - dMin + 1) * (rMax - rMin + 1) *
                           (cMax - cMin + 1) * sizeof(data[0]))) {
            return false;
        }
        long rowStep = rowStride;
        if (invert_rows) {
            rowStep *= -1;
        }
        for (unsigned d = dMin; d <= dMax; d++) {
            // XXX Turn the depth calculation into start and += like the others
            const vrpn_uint16 *rowStart =
                &data[d * depthStride + rMin * rowStride + cMin];
            if (invert_rows) {
                rowStart = &data[d * depthStride +
                                 (nRows - 1 - rMin) * rowStride + cMin];
            }
            const vrpn_uint16 *copyFrom = rowStart;
            for (unsigned r = rMin; r <= rMax; r++) {
                for (unsigned c = cMin; c <= cMax; c++) {
                    memcpy(msgbuf, copyFrom, sizeof(*copyFrom));
                    msgbuf +=
                        sizeof(*copyFrom); //< Skip to the next buffer location
                    copyFrom +=
                        colStride; //< Skip appropriate number of elements
                }
                rowStart += rowStep; //< Skip to the start of the next row
                copyFrom = rowStart;
            }
        }
        buflen -= (rMax - rMin + 1) * (cMax - cMin + 1) * sizeof(data[0]);
    }

    // Swap endian-ness of the buffer if we are on a big-endian machine.
    if (vrpn_big_endian) {
        fprintf(stderr, "XXX Imager Region needs swapping on Big-endian\n");
        return false;
    }

    // Pack the message
    vrpn_int32 len = sizeof(fbuf) - buflen;
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_regionu16_m_id,
                                   d_sender_id, (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "cannot write message: tossing\n");
        return false;
    }

    return true;
}

/** As efficiently as possible, pull the values out of the array whose pointer
   is passed
    in and send them over a VRPN connection as a region message that is
   appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (0,0) element of the 2D array that holds the image.
        It is assumed that the data values should be copied exactly, and that
        offset and scale should be applied at the reading end by the user if
        they want to get them back into absolute units.
      colStride, rowStride: Number of elements (size of data type) to skip along
   a column and row
      nRows, invert_rows: If invert_rows is true, then nRows must be set to the
   number of rows
        in the entire image.  If invert_rows is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer
   for "now")
*/

bool vrpn_Imager_Server::send_region_using_base_pointer(
    vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax, vrpn_uint16 rMin,
    vrpn_uint16 rMax, const vrpn_float32 *data, vrpn_uint32 colStride,
    vrpn_uint32 rowStride, vrpn_uint16 nRows, bool invert_rows,
    vrpn_uint32 depthStride, vrpn_uint16 dMin, vrpn_uint16 dMax,
    const struct timeval *time)
{
    // msgbuf must be float64-aligned!  It is the buffer to send to the client
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = (char *)fbuf;
    int buflen = sizeof(fbuf);
    struct timeval timestamp;

    // If we are discarding frames, return failure to send.
    if (d_dropped_due_to_throttle > 0) {
        return false;
    }

    // Make sure the region request has a valid channel, has indices all
    // within the image size, and is not too large to fit into the data
    // array (which is sized the way it is to make sure it can be sent in
    // one VRPN reliable message).
    if ((chanIndex < 0) || (chanIndex >= d_nChannels)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid channel index (%d)\n",
                chanIndex);
        return false;
    }
    if ((dMax >= d_nDepth) || (dMin > dMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid depth range (%d..%d)\n",
                dMin, dMax);
        return false;
    }
    if ((rMax >= d_nRows) || (rMin > rMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid row range (%d..%d)\n",
                rMin, rMax);
        return false;
    }
    if ((cMax >= d_nCols) || (cMin > cMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Invalid column range (%d..%d)\n",
                cMin, cMax);
        return false;
    }
    if (static_cast<unsigned>((rMax - rMin + 1) * (cMax - cMin + 1) *
                              (dMax - dMin + 1)) > vrpn_IMAGER_MAX_REGIONf32) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Region too large (%d,%d,%d to %d,%d,%d)\n",
                cMin, rMin, dMin, cMax, rMax, dMax);
        return false;
    }
    if (invert_rows && (nRows < rMax)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "nRows must not be less than rMax\n");
        return false;
    }

    // Make sure we've sent the description before we send any regions
    if (!d_description_sent) {
        send_description();
        d_description_sent = true;
    }

    // If the user didn't specify a time, assume they want "now" and look it up.
    if (time != NULL) {
        timestamp = *time;
    }
    else {
        vrpn_gettimeofday(&timestamp, NULL);
    }

    // Check the compression status and prepare to do compression if it is
    // called for.
    if (d_channels[chanIndex].d_compression != vrpn_Imager_Channel::NONE) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "Compression not implemented\n");
        return false;
    }

    // Tell which channel this region is for, and what the borders of the
    // region are.
    if (vrpn_buffer(&msgbuf, &buflen, chanIndex) ||
        vrpn_buffer(&msgbuf, &buflen, dMin) ||
        vrpn_buffer(&msgbuf, &buflen, dMax) ||
        vrpn_buffer(&msgbuf, &buflen, rMin) ||
        vrpn_buffer(&msgbuf, &buflen, rMax) ||
        vrpn_buffer(&msgbuf, &buflen, cMin) ||
        vrpn_buffer(&msgbuf, &buflen, cMax) ||
        vrpn_buffer(&msgbuf, &buflen, vrpn_IMAGER_VALTYPE_FLOAT32)) {
        return false;
    }

    // Insert the data into the buffer, copying it as efficiently as possible
    // from the caller's buffer into the buffer we are going to send.  Note that
    // the send buffer is going to be little-endian.  The code looks a little
    // complicated because it short-circuits the copying for the case where the
    // column stride is one element long (using memcpy() on each row) but has to
    // copy one element at a time otherwise.
    // There is also the matter if deciding whether to invert the image in y,
    // which complicates the index calculation and the calculation of the
    // strides.
    int cols = cMax - cMin + 1;
    int linelen = cols * sizeof(data[0]);
    if (colStride == 1) {
        for (unsigned d = dMin; d <= dMax; d++) {
            for (unsigned r = rMin; r <= rMax; r++) {
                unsigned rActual;
                if (invert_rows) {
                    rActual = (nRows - 1) - r;
                }
                else {
                    rActual = r;
                }
                if (buflen < linelen) {
                    return false;
                }
                memcpy(msgbuf,
                       &data[d * depthStride + rActual * rowStride + cMin],
                       linelen);
                msgbuf += linelen;
                buflen -= linelen;
            }
        }
    }
    else {
        if (buflen < (int)((dMax - dMin + 1) * (rMax - rMin + 1) *
                           (cMax - cMin + 1) * sizeof(data[0]))) {
            return false;
        }
        long rowStep = rowStride;
        if (invert_rows) {
            rowStep *= -1;
        }
        for (unsigned d = dMin; d <= dMax; d++) {
            // XXX Turn the depth calculation into start and += like the others
            const vrpn_float32 *rowStart =
                &data[d * depthStride + rMin * rowStride + cMin];
            if (invert_rows) {
                rowStart = &data[d * depthStride +
                                 (nRows - 1 - rMin) * rowStride + cMin];
            }
            const vrpn_float32 *copyFrom = rowStart;
            for (unsigned r = rMin; r <= rMax; r++) {
                for (unsigned c = cMin; c <= cMax; c++) {
                    memcpy(msgbuf, copyFrom, sizeof(*copyFrom));
                    msgbuf +=
                        sizeof(*copyFrom); //< Skip to the next buffer location
                    copyFrom +=
                        colStride; //< Skip appropriate number of elements
                }
                rowStart += rowStep; //< Skip to the start of the next row
                copyFrom = rowStart;
            }
        }
        buflen -= (rMax - rMin + 1) * (cMax - cMin + 1) * sizeof(data[0]);
    }

    // Swap endian-ness of the buffer if we are on a big-endian machine.
    if (vrpn_big_endian) {
        fprintf(stderr, "XXX Imager Region needs swapping on Big-endian\n");
        return false;
    }

    // Pack the message
    vrpn_int32 len = sizeof(fbuf) - buflen;
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_regionf32_m_id,
                                   d_sender_id, (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_base_pointer(): "
                        "cannot write message: tossing\n");
        return false;
    }

    return true;
}

/** As efficiently as possible, pull the values out of the array whose pointer
   is passed
    in and send them over a VRPN connection as a region message that is
   appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (cMin,rMin) element of the 2D array that holds the
   image.
        It is assumed that the data values should be copied exactly, and that
        offset and scale should be applied at the reading end by the user if
        they want to get them back into absolute units.
      colStride, rowStride: Number of elements (size of data type) to skip along
   a column and row
      nRows, invert_rows: If invert_rows is true, then nRows must be set to the
   number of rows
        in the entire image.  If invert_rows is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer
   for "now")
*/

bool vrpn_Imager_Server::send_region_using_first_pointer(
    vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax, vrpn_uint16 rMin,
    vrpn_uint16 rMax, const vrpn_uint8 *data, vrpn_uint32 colStride,
    vrpn_uint32 rowStride, vrpn_uint16 nRows, bool invert_rows,
    vrpn_uint32 depthStride, vrpn_uint16 dMin, vrpn_uint16 dMax,
    const struct timeval *time)
{
    // If we are discarding frames, return failure to send.
    if (d_dropped_due_to_throttle > 0) {
        return false;
    }

    // Point the data pointer back before the first pointer, to the place it
    // should be to make the index math work out.
    const vrpn_uint8 *new_base =
        data - (cMin + rowStride * rMin + depthStride * dMin);
    if (send_region_using_base_pointer(
            chanIndex, cMin, cMax, rMin, rMax, new_base, colStride, rowStride,
            nRows, invert_rows, depthStride, dMin, dMax, time)) {
        return true;
    }
    else {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_first_pointer():"
                        " Call to send using offset base_pointer failed.\n");
        return false;
    }
}

/** As efficiently as possible, pull the values out of the array whose pointer
   is passed
    in and send them over a VRPN connection as a region message that is
   appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (cMin, rMin) element of the 2D array that holds the
   image.
        It is assumed that the data values should be copied exactly, and that
        offset and scale should be applied at the reading end by the user if
        they want to get them back into absolute units.
      colStride, rowStride: Number of elements (size of data type) to skip along
   a column and row
      nRows, invert_rows: If invert_rows is true, then nRows must be set to the
   number of rows
        in the entire image.  If invert_rows is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer
   for "now")
*/

bool vrpn_Imager_Server::send_region_using_first_pointer(
    vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax, vrpn_uint16 rMin,
    vrpn_uint16 rMax, const vrpn_uint16 *data, vrpn_uint32 colStride,
    vrpn_uint32 rowStride, vrpn_uint16 nRows, bool invert_rows,
    vrpn_uint32 depthStride, vrpn_uint16 dMin, vrpn_uint16 dMax,
    const struct timeval *time)
{
    // If we are discarding frames, return failure to send.
    if (d_dropped_due_to_throttle > 0) {
        return false;
    }

    // Point the data pointer back before the first pointer, to the place it
    // should be to make the index math work out.
    const vrpn_uint16 *new_base =
        data - (cMin + rowStride * rMin + depthStride * dMin);
    if (send_region_using_base_pointer(
            chanIndex, cMin, cMax, rMin, rMax, new_base, colStride, rowStride,
            nRows, invert_rows, depthStride, dMin, dMax, time)) {
        return true;
    }
    else {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_first_pointer():"
                        " Call to send using offset base_pointer failed.\n");
        return false;
    }
}

/** As efficiently as possible, pull the values out of the array whose pointer
   is passed
    in and send them over a VRPN connection as a region message that is
   appropriate for
    the type that has been passed in (the type that data points to).
      Chanindex: What channel this region holds data for
      cMin, cMax: Range of columns to extract from this data set
      rMin, rMax: Range of rows to extract from this data set
      data: Points at the (cMin, rMin) element of the 2D array that holds the
   image.
        It is assumed that the data values should be copied exactly, and that
        offset and scale should be applied at the reading end by the user if
        they want to get them back into absolute units.
      colStride, rowStride: Number of elements (size of data type) to skip along
   a column and row
      nRows, invert_rows: If invert_rows is true, then nRows must be set to the
   number of rows
        in the entire image.  If invert_rows is false, then nRows is not used.
      time: What time the message should have associated with it (NULL pointer
   for "now")
*/

bool vrpn_Imager_Server::send_region_using_first_pointer(
    vrpn_int16 chanIndex, vrpn_uint16 cMin, vrpn_uint16 cMax, vrpn_uint16 rMin,
    vrpn_uint16 rMax, const vrpn_float32 *data, vrpn_uint32 colStride,
    vrpn_uint32 rowStride, vrpn_uint16 nRows, bool invert_rows,
    vrpn_uint32 depthStride, vrpn_uint16 dMin, vrpn_uint16 dMax,
    const struct timeval *time)
{
    // If we are discarding frames, return failure to send.
    if (d_dropped_due_to_throttle > 0) {
        return false;
    }

    // Point the data pointer back before the first pointer, to the place it
    // should be to make the index math work out.
    const vrpn_float32 *new_base =
        data - (cMin + rowStride * rMin + depthStride * dMin);
    if (send_region_using_base_pointer(
            chanIndex, cMin, cMax, rMin, rMax, new_base, colStride, rowStride,
            nRows, invert_rows, depthStride, dMin, dMax, time)) {
        return true;
    }
    else {
        fprintf(stderr, "vrpn_Imager_Server::send_region_using_first_pointer():"
                        " Call to send using offset base_pointer failed.\n");
        return false;
    }
}

bool vrpn_Imager_Server::send_description(void)
{
    // msgbuf must be float64-aligned!
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = (char *)fbuf;
    int buflen = sizeof(fbuf);
    struct timeval timestamp;
    int i;

    // Pack the description of all of the fields in the imager into the buffer,
    // including the channel descriptions.
    if (vrpn_buffer(&msgbuf, &buflen, d_nDepth) ||
        vrpn_buffer(&msgbuf, &buflen, d_nRows) ||
        vrpn_buffer(&msgbuf, &buflen, d_nCols) ||
        vrpn_buffer(&msgbuf, &buflen, d_nChannels)) {
        fprintf(stderr, "vrpn_Imager_Server::send_description(): Can't pack "
                        "message header, tossing\n");
        return false;
    }
    for (i = 0; i < d_nChannels; i++) {
        if (!d_channels[i].buffer(&msgbuf, &buflen)) {
            fprintf(stderr, "vrpn_Imager_Server::send_description(): Can't "
                            "pack message channel, tossing\n");
            return false;
        }
    }

    // Pack the buffer into the connection's outgoing reliable queue, if we have
    // a valid connection.
    vrpn_int32 len = sizeof(fbuf) - buflen;
    vrpn_gettimeofday(&timestamp, NULL);
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_description_m_id,
                                   d_sender_id, (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_Imager_Server::send_description(): cannot write "
                        "message: tossing\n");
        return false;
    }

    d_description_sent = true;
    return true;
}

bool vrpn_Imager_Server::set_resolution(vrpn_int32 nCols, vrpn_int32 nRows,
                                        vrpn_int32 nDepth)
{
    if ((nCols <= 0) || (nRows <= 0) || (nDepth <= 0)) {
        fprintf(
            stderr,
            "vrpn_Imager_Server::set_resolution(): Invalid size (%d, %d, %d)\n",
            nCols, nRows, nDepth);
        return false;
    }
    d_nDepth = nDepth;
    d_nCols = nCols;
    d_nRows = nRows;
    return send_description();
}

void vrpn_Imager_Server::mainloop(void) { server_mainloop(); }

int vrpn_Imager_Server::handle_ping_message(void *userdata, vrpn_HANDLERPARAM)
{
    vrpn_Imager_Server *me = (vrpn_Imager_Server *)userdata;
    me->send_description();
    return 0;
}

int vrpn_Imager_Server::handle_throttle_message(void *userdata,
                                                vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_Imager_Server *me = (vrpn_Imager_Server *)userdata;

    // Get the requested number of frames from the buffer
    vrpn_int32 frames_to_send;
    if (vrpn_unbuffer(&bufptr, &frames_to_send)) {
        return -1;
    }

    // If the requested number of frames is negative, then we set
    // for unbounded sending.  The next time a begin_frame message
    // is sent, it will start the process going again.
    if (frames_to_send < 0) {
        me->d_frames_to_send = -1;
        return 0;
    }

    // If we were sending continuously, store the number of frames
    // to send.
    if (me->d_frames_to_send == -1) {
        me->d_frames_to_send = frames_to_send;

        // If we already had a throttle limit set, then increment it
        // by the count.
    }
    else {
        me->d_frames_to_send += frames_to_send;
    }

    return 0;
}

int vrpn_Imager_Server::handle_last_drop_message(void *userdata,
                                                 vrpn_HANDLERPARAM)
{
    vrpn_Imager_Server *me = (vrpn_Imager_Server *)userdata;

    // Last connection has dropped, so set the throttling behavior back to
    // the default, which is to send as fast as we can.
    me->d_frames_to_send = -1;
    me->d_dropped_due_to_throttle = 0;
    return 0;
}

vrpn_Imager_Remote::vrpn_Imager_Remote(const char *name, vrpn_Connection *c)
    : vrpn_Imager(name, c)
    , d_got_description(false)
{
    // Register the handlers for the description message and the region change
    // messages
    register_autodeleted_handler(d_description_m_id, handle_description_message,
                                 this, d_sender_id);

    // Register the region-handling messages for the different message types.
    // All of the types use the same handler, since the type of region is
    // encoded
    // in one of the parameters of the region function.
    register_autodeleted_handler(d_regionu8_m_id, handle_region_message, this,
                                 d_sender_id);
    register_autodeleted_handler(d_regionu16_m_id, handle_region_message, this,
                                 d_sender_id);
    register_autodeleted_handler(d_regionf32_m_id, handle_region_message, this,
                                 d_sender_id);
    register_autodeleted_handler(d_begin_frame_m_id, handle_begin_frame_message,
                                 this, d_sender_id);
    register_autodeleted_handler(d_end_frame_m_id, handle_end_frame_message,
                                 this, d_sender_id);
    register_autodeleted_handler(d_discarded_frames_m_id,
                                 handle_discarded_frames_message, this,
                                 d_sender_id);

    // Register the handler for the connection dropped message
    register_autodeleted_handler(
        d_connection->register_message_type(vrpn_dropped_connection),
        handle_connection_dropped_message, this);
}

void vrpn_Imager_Remote::mainloop(void)
{
    client_mainloop();
    if (d_connection) {
        d_connection->mainloop();
    };
}

const vrpn_Imager_Channel *vrpn_Imager_Remote::channel(unsigned chanNum) const
{
    if (chanNum >= (unsigned)d_nChannels) {
        return NULL;
    }
    return &d_channels[chanNum];
}

int vrpn_Imager_Remote::handle_description_message(void *userdata,
                                                   vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_Imager_Remote *me = (vrpn_Imager_Remote *)userdata;
    int i;

    // Get my new information from the buffer
    if (vrpn_unbuffer(&bufptr, &me->d_nDepth) ||
        vrpn_unbuffer(&bufptr, &me->d_nRows) ||
        vrpn_unbuffer(&bufptr, &me->d_nCols) ||
        vrpn_unbuffer(&bufptr, &me->d_nChannels)) {
        return -1;
    }
    for (i = 0; i < me->d_nChannels; i++) {
        if (!me->d_channels[i].unbuffer(&bufptr)) {
            return -1;
        }
    }

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->d_description_list.call_handlers(p.msg_time);

    me->d_got_description = true;
    return 0;
}

int vrpn_Imager_Remote::handle_region_message(void *userdata,
                                              vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_Imager_Remote *me = (vrpn_Imager_Remote *)userdata;
    vrpn_IMAGERREGIONCB rp;
    vrpn_Imager_Region reg;

    // Create an instance of a region helper class and read its
    // parameters from the buffer (setting its _valBuf pointer at
    // the start of the data in the buffer).  Set it to valid and then
    // call the user callback and then set it to invalid before
    // deleting it.
    if (vrpn_unbuffer(&bufptr, &reg.d_chanIndex) ||
        vrpn_unbuffer(&bufptr, &reg.d_dMin) ||
        vrpn_unbuffer(&bufptr, &reg.d_dMax) ||
        vrpn_unbuffer(&bufptr, &reg.d_rMin) ||
        vrpn_unbuffer(&bufptr, &reg.d_rMax) ||
        vrpn_unbuffer(&bufptr, &reg.d_cMin) ||
        vrpn_unbuffer(&bufptr, &reg.d_cMax) ||
        vrpn_unbuffer(&bufptr, &reg.d_valType)) {
        fprintf(stderr, "vrpn_Imager_Remote::handle_region_message(): Can't "
                        "unbuffer parameters!\n");
        return -1;
    }
    reg.d_valBuf = bufptr;
    reg.d_valid = true;

    // Check the compression status and prepare to do decompression if it is
    // called for.
    if (me->d_channels[reg.d_chanIndex].d_compression !=
        vrpn_Imager_Channel::NONE) {
        fprintf(stderr, "vrpn_Imager_Remote::handle_region_message(): "
                        "Compression not implemented\n");
        return -1;
    }

    // Fill in a user callback structure with the data
    rp.msg_time = p.msg_time;
    rp.region = &reg;

    // ONLY if we have gotten a description message,
    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    if (me->d_got_description) {
        me->d_region_list.call_handlers(rp);
    }

    reg.d_valid = false;
    return 0;
}

int vrpn_Imager_Remote::handle_begin_frame_message(void *userdata,
                                                   vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_Imager_Remote *me = (vrpn_Imager_Remote *)userdata;
    vrpn_IMAGERBEGINFRAMECB bf;

    bf.msg_time = p.msg_time;
    if (vrpn_unbuffer(&bufptr, &bf.dMin) || vrpn_unbuffer(&bufptr, &bf.dMax) ||
        vrpn_unbuffer(&bufptr, &bf.rMin) || vrpn_unbuffer(&bufptr, &bf.rMax) ||
        vrpn_unbuffer(&bufptr, &bf.cMin) || vrpn_unbuffer(&bufptr, &bf.cMax)) {
        fprintf(stderr, "vrpn_Imager_Remote::handle_begin_frame_message(): "
                        "Can't unbuffer parameters!\n");
        return -1;
    }

    // ONLY if we have gotten a description message,
    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    if (me->d_got_description) {
        me->d_begin_frame_list.call_handlers(bf);
    }

    return 0;
}

int vrpn_Imager_Remote::handle_end_frame_message(void *userdata,
                                                 vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_Imager_Remote *me = (vrpn_Imager_Remote *)userdata;
    vrpn_IMAGERENDFRAMECB ef;

    ef.msg_time = p.msg_time;
    if (vrpn_unbuffer(&bufptr, &ef.dMin) || vrpn_unbuffer(&bufptr, &ef.dMax) ||
        vrpn_unbuffer(&bufptr, &ef.rMin) || vrpn_unbuffer(&bufptr, &ef.rMax) ||
        vrpn_unbuffer(&bufptr, &ef.cMin) || vrpn_unbuffer(&bufptr, &ef.cMax)) {
        fprintf(stderr, "vrpn_Imager_Remote::handle_end_frame_message(): Can't "
                        "unbuffer parameters!\n");
        return -1;
    }

    // ONLY if we have gotten a description message,
    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    if (me->d_got_description) {
        me->d_end_frame_list.call_handlers(ef);
    }

    return 0;
}

int vrpn_Imager_Remote::handle_discarded_frames_message(void *userdata,
                                                        vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_Imager_Remote *me = (vrpn_Imager_Remote *)userdata;
    vrpn_IMAGERDISCARDEDFRAMESCB df;

    df.msg_time = p.msg_time;
    if (vrpn_unbuffer(&bufptr, &df.count)) {
        fprintf(stderr, "vrpn_Imager_Remote::handle_discarded_frames_message():"
                        " Can't unbuffer parameters!\n");
        return -1;
    }

    // ONLY if we have gotten a description message,
    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    if (me->d_got_description) {
        me->d_discarded_frames_list.call_handlers(df);
    }

    return 0;
}

int vrpn_Imager_Remote::handle_connection_dropped_message(void *userdata,
                                                          vrpn_HANDLERPARAM)
{
    vrpn_Imager_Remote *me = (vrpn_Imager_Remote *)userdata;

    // We have no description message, so don't call region callbacks
    me->d_got_description = false;

    return 0;
}

bool vrpn_Imager_Remote::throttle_sender(vrpn_int32 N)
{
    // msgbuf must be float64-aligned!
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = (char *)fbuf;
    int buflen = sizeof(fbuf);
    struct timeval timestamp;

    // Pack the throttle count request.
    if (vrpn_buffer(&msgbuf, &buflen, N)) {
        fprintf(stderr, "vrpn_ImagerPose_Server::throttle_sender(): Can't pack "
                        "message header, tossing\n");
        return false;
    }

    // Pack the buffer into the connection's outgoing reliable queue, if we have
    // a valid connection.
    vrpn_int32 len = sizeof(fbuf) - buflen;
    vrpn_gettimeofday(&timestamp, NULL);
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_throttle_frames_m_id,
                                   d_sender_id, (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_ImagerPose_Server::throttle_sender(): cannot "
                        "write message: tossing\n");
        return false;
    }

    return true;
}

/** As efficiently as possible, pull the values out of the VRPN buffer and put
   them into
    the array whose pointer is passed in, transcoding as needed to convert it
   into the
    type of the pointer passed in.
      data: Points at the (0,0) element of the 2D array that is to hold the
   image.
        It is assumed that the data values should be copied exactly, and that
        offset and scale should be applied at the reading end by the user if
        they want to get them back into absolute units.
      repeat: How many times to copy each element (for example, this will be 3
   if we are
          copying an unsigned 8-bit value from the network into the Red, Green,
   and Blue
          entries of a color map.
      colStride, rowStride: Number of elements (size of data type) to skip along
   a column and row.
          This tells how far to jump ahead when writing the next element in the
   row (must
          be larger or the same as the repeat) and how make elements to skip to
   get to the
          beginning of the next column.  For example, copying a uint from the
   network into
          the Red, Green, and Blue elements of an RGBA texture would have
   repeat=3, colStride=4,
          and rowStride=4*(number of columns).
      numRows, invert_rows: numRows needs to equal the number of rows in the
   entire image if
          invert_rows is true; it is not used otherwise.  invert_rows inverts
   the image in y
          as it is copying it into the user's buffer.
*/

bool vrpn_Imager_Region::decode_unscaled_region_using_base_pointer(
    vrpn_uint8 *data, vrpn_uint32 colStride, vrpn_uint32 rowStride,
    vrpn_uint32 depthStride, vrpn_uint16 nRows, bool invert_rows,
    unsigned repeat) const
{
    // Make sure the parameters are reasonable
    if (colStride < repeat) {
        fprintf(stderr, "vrpn_Imager_Region::decode_unscaled_region_using_base_"
                        "pointer(): colStride must be >= repeat\n");
        return false;
    }

    if (invert_rows && (nRows < d_rMax)) {
        fprintf(stderr, "vrpn_Imager_Region::decode_unscaled_region_using_base_"
                        "pointer(): nRows must not be less than _rMax\n");
        return false;
    }

    // If the type of data in the buffer doesn't match the type of data the user
    // wants, we need to convert each element along the way.
    if (d_valType == vrpn_IMAGER_VALTYPE_UINT8) {
        // The data type matches what we the user is asking for.  No transcoding
        // needed.
        // Insert the data into the buffer, copying it as efficiently as
        // possible
        // from the network buffer into the caller's buffer .  Note that
        // the network buffer is little-endian.  The code looks a little
        // complicated because it short-circuits the copying for the case where
        // the
        // column stride and repeat are one element long (using memcpy() on each
        // row) but has to
        // copy one element at a time otherwise.
        int cols = d_cMax - d_cMin + 1;
        int linelen = cols * sizeof(data[0]);
        if ((colStride == 1) && (repeat == 1)) {
            const vrpn_uint8 *msgbuf = (const vrpn_uint8 *)d_valBuf;
            for (unsigned d = d_dMin; d <= d_dMax; d++) {
                for (unsigned r = d_rMin; r <= d_rMax; r++) {
                    unsigned rActual;
                    if (invert_rows) {
                        rActual = (nRows - 1) - r;
                    }
                    else {
                        rActual = r;
                    }
                    memcpy(
                        &data[d * depthStride + rActual * rowStride + d_cMin],
                        msgbuf, linelen);
                    msgbuf += linelen;
                }
            }
        }
        else {
            long rowStep = rowStride;
            if (invert_rows) {
                rowStep *= -1;
            }
            const vrpn_uint8 *msgbuf = (const vrpn_uint8 *)d_valBuf;
            for (unsigned d = d_dMin; d <= d_dMax; d++) {
                vrpn_uint8 *rowStart =
                    &data[d * depthStride + d_rMin * rowStride +
                          d_cMin * repeat];
                if (invert_rows) {
                    rowStart = &data[d * depthStride +
                                     (nRows - 1 - d_rMin) * rowStride +
                                     d_cMin * repeat];
                }
                vrpn_uint8 *copyTo = rowStart;
                for (unsigned r = d_rMin; r <= d_rMax; r++) {
                    for (unsigned c = d_cMin; c <= d_cMax; c++) {
                        for (unsigned rpt = 0; rpt < repeat; rpt++) {
                            *(copyTo + rpt) =
                                *msgbuf; //< Copy the current element
                        }
                        msgbuf++; //< Skip to the next buffer location
                        copyTo +=
                            colStride; //< Skip appropriate number of elements
                    }
                    rowStart += rowStep; //< Skip to the start of the next row
                    copyTo = rowStart;
                }
            }
        }
    }
    else if (d_valType == vrpn_IMAGER_VALTYPE_FLOAT32) {
        // Transcode from 32-bit floating-point to 8-bit values during the
        // conversion process.
        // As this is unscaled, we do not adjust the values -- simply jam the
        // values in and
        // let C++ conversion do the work for us.

        // Swap endian-ness of the buffer if we are on a big-endian machine.
        if (vrpn_big_endian) {
            fprintf(stderr, "XXX Imager Region needs swapping on Big-endian\n");
            return false;
        }

        long rowStep = rowStride;
        if (invert_rows) {
            rowStep *= -1;
        }
        const vrpn_float32 *msgbuf = (const vrpn_float32 *)d_valBuf;
        for (unsigned d = d_dMin; d <= d_dMax; d++) {
            vrpn_uint8 *rowStart =
                &data[d * depthStride + d_rMin * rowStride + d_cMin * repeat];
            if (invert_rows) {
                rowStart =
                    &data[d * depthStride + (nRows - 1 - d_rMin) * rowStride +
                          d_cMin * repeat];
            }
            vrpn_uint8 *copyTo = rowStart;
            for (unsigned r = d_rMin; r <= d_rMax; r++) {
                for (unsigned c = d_cMin; c <= d_cMax; c++) {
                    for (unsigned rpt = 0; rpt < repeat; rpt++) {
                        *(copyTo + rpt) = static_cast<vrpn_uint8>(
                            *msgbuf); //< Copy the current element
                    }
                    msgbuf++;            //< Skip to the next buffer location
                    copyTo += colStride; //< Skip appropriate number of elements
                }
                rowStart += rowStep; //< Skip to the start of the next row
                copyTo = rowStart;
            }
        }
    }
    else if (d_valType == vrpn_IMAGER_VALTYPE_UINT16) {
        // transcode from 16 bit image to 8 bit data for app

        long rowStep = rowStride;
        if (invert_rows) {
            rowStep *= -1;
        }
        const vrpn_uint16 *msgbuf = (const vrpn_uint16 *)d_valBuf;
        for (unsigned d = d_dMin; d <= d_dMax; d++) {
            vrpn_uint8 *rowStart =
                &data[d * depthStride + d_rMin * rowStride + d_cMin * repeat];
            if (invert_rows) {
                rowStart =
                    &data[d * depthStride + (nRows - 1 - d_rMin) * rowStride +
                          d_cMin * repeat];
            }
            vrpn_uint8 *copyTo = rowStart;
            for (unsigned r = d_rMin; r <= d_rMax; r++) {
                for (unsigned c = d_cMin; c <= d_cMax; c++) {
                    for (unsigned rpt = 0; rpt < repeat; rpt++) {
                        *(copyTo + rpt) = static_cast<vrpn_uint8>(
                            (*msgbuf) >>
                            8); //< Copy the current element (take top 8 bits)
                    }
                    msgbuf++;            //< Skip to the next buffer location
                    copyTo += colStride; //< Skip appropriate number of elements
                }
                rowStart += rowStep; //< Skip to the start of the next row
                copyTo = rowStart;
            }
        }
    }
    else {
        printf("vrpn_Imager_Region::decode_unscaled_region_using_base_pointer()"
               ": Transcoding not implemented yet for this type\n");
        printf("d_valType = %i\n", d_valType);
        return false;
    }

    // No need to swap endianness on single-byte entities.
    return true;
}

// This routine handles both 16-bit and 12-bit-in-16 iamges.
bool vrpn_Imager_Region::decode_unscaled_region_using_base_pointer(
    vrpn_uint16 *data, vrpn_uint32 colStride, vrpn_uint32 rowStride,
    vrpn_uint32 depthStride, vrpn_uint16 nRows, bool invert_rows,
    unsigned repeat) const
{
    // Make sure the parameters are reasonable
    if (colStride < repeat) {
        fprintf(stderr, "vrpn_Imager_Region::decode_unscaled_region_using_base_"
                        "pointer(): colStride must be >= repeat\n");
        return false;
    }
    if (invert_rows && (nRows < d_rMax)) {
        fprintf(stderr, "vrpn_Imager_Region::decode_unscaled_region_using_base_"
                        "pointer(): nRows must not be less than _rMax\n");
        return false;
    }

    // If the type of data in the buffer matches the type of data the user
    // wants, no need to convert each element along the way.
    if ((d_valType == vrpn_IMAGER_VALTYPE_UINT16) ||
        (d_valType == vrpn_IMAGER_VALTYPE_UINT12IN16)) {

        // The data type matches what we the user is asking for.  No transcoding
        // needed.
        // Insert the data into the buffer, copying it as efficiently as
        // possible
        // from the network buffer into the caller's buffer .  Note that
        // the network buffer is little-endian.  The code looks a little
        // complicated because it short-circuits the copying for the case where
        // the
        // column stride and repeat are one element long (using memcpy() on each
        // row) but has to
        // copy one element at a time otherwise.
        int cols = d_cMax - d_cMin + 1;
        int linelen = cols * sizeof(data[0]);
        if ((colStride == 1) && (repeat == 1)) {
            const vrpn_uint16 *msgbuf = (const vrpn_uint16 *)d_valBuf;
            for (unsigned d = d_dMin; d <= d_dMax; d++) {
                for (unsigned r = d_rMin; r <= d_rMax; r++) {
                    unsigned rActual;
                    if (invert_rows) {
                        rActual = (nRows - 1) - r;
                    }
                    else {
                        rActual = r;
                    }
                    memcpy(
                        &data[d * depthStride + rActual * rowStride + d_cMin],
                        msgbuf, linelen);
                    msgbuf += cols;
                }
            }
        }
        else {
            long rowStep = rowStride;
            if (invert_rows) {
                rowStep *= -1;
            }
            const vrpn_uint16 *msgbuf = (const vrpn_uint16 *)d_valBuf;
            for (unsigned d = d_dMin; d <= d_dMax; d++) {
                vrpn_uint16 *rowStart =
                    &data[d * depthStride + d_rMin * rowStride +
                          d_cMin * repeat];
                if (invert_rows) {
                    rowStart = &data[d * depthStride +
                                     (nRows - 1 - d_rMin) * rowStride +
                                     d_cMin * repeat];
                }
                vrpn_uint16 *copyTo = rowStart;
                for (unsigned r = d_rMin; r <= d_rMax; r++) {
                    for (unsigned c = d_cMin; c <= d_cMax; c++) {
                        for (unsigned rpt = 0; rpt < repeat; rpt++) {
                            *(copyTo + rpt) =
                                *msgbuf; //< Copy the current element
                        }
                        msgbuf++; //< Skip to the next buffer location
                        copyTo +=
                            colStride; //< Skip appropriate number of elements
                    }
                    rowStart += rowStep; //< Skip to the start of the next row
                    copyTo = rowStart;
                }
            }
        }

        // The data type sent does not match the type asked for.  We will need
        // to trans-code
        // from the format it is in to the format they want.
    }
    else if (d_valType == vrpn_IMAGER_VALTYPE_UINT8) {
        // Convert from unsigned integer 8-bit to unsigned integer 16-bit
        // by shifting each left 8 bits (same as multiplying by 256).  Note
        // that this does not map 255 to the maximum 16-bit value, but doing
        // that would require either floating-point math or precomputing that
        // math and using table look-up.
        long rowStep = rowStride;
        if (invert_rows) {
            rowStep *= -1;
        }
        const vrpn_uint8 *msgbuf = (const vrpn_uint8 *)d_valBuf;
        for (unsigned d = d_dMin; d <= d_dMax; d++) {
            vrpn_uint16 *rowStart =
                &data[d * depthStride + d_rMin * rowStride + d_cMin * repeat];
            if (invert_rows) {
                rowStart =
                    &data[d * depthStride + (nRows - 1 - d_rMin) * rowStride +
                          d_cMin * repeat];
            }
            vrpn_uint16 *copyTo = rowStart;
            for (unsigned r = d_rMin; r <= d_rMax; r++) {
                for (unsigned c = d_cMin; c <= d_cMax; c++) {
                    for (unsigned rpt = 0; rpt < repeat; rpt++) {
                        //< Shift and copy the current element
                        *(copyTo + rpt) =
                            (static_cast<vrpn_uint16>(*msgbuf) << 8);
                    }
                    msgbuf++;            //< Skip to the next buffer location
                    copyTo += colStride; //< Skip appropriate number of elements
                }
                rowStart += rowStep; //< Skip to the start of the next row
                copyTo = rowStart;
            }
        }
    }
    else {
        fprintf(stderr, "vrpn_Imager_Region::decode_unscaled_region_using_base_"
                        "pointer(): XXX Transcoding this type not yet "
                        "implemented\n");
        return false;
    }

    // Swap endian-ness of the buffer if we are on a big-endian machine.
    if (vrpn_big_endian) {
        fprintf(stderr, "XXX Imager Region needs swapping on Big-endian\n");
        return false;
    }

    return true;
}

bool vrpn_Imager_Region::decode_unscaled_region_using_base_pointer(
    vrpn_float32 *data, vrpn_uint32 colStride, vrpn_uint32 rowStride,
    vrpn_uint32 depthStride, vrpn_uint16 nRows, bool invert_rows,
    unsigned repeat) const
{
    // Make sure the parameters are reasonable
    if (colStride < repeat) {
        fprintf(stderr, "vrpn_Imager_Region::decode_unscaled_region_using_base_"
                        "pointer(): colStride must be >= repeat\n");
        return false;
    }

    // If the type of data in the buffer doesn't match the type of data the user
    // wants, we need to convert each element along the way.
    if (d_valType != vrpn_IMAGER_VALTYPE_FLOAT32) {
        printf("vrpn_Imager_Region::decode_unscaled_region_using_base_pointer()"
               ": Transcoding not implemented yet\n");
        return false;
    }
    if (invert_rows && (nRows < d_rMax)) {
        fprintf(stderr, "vrpn_Imager_Region::decode_unscaled_region_using_base_"
                        "pointer(): nRows must not be less than _rMax\n");
        return false;
    }

    // The data type matches what we the user is asking for.  No transcoding
    // needed.
    // Insert the data into the buffer, copying it as efficiently as possible
    // from the network buffer into the caller's buffer .  Note that
    // the network buffer is little-endian.  The code looks a little
    // complicated because it short-circuits the copying for the case where the
    // column stride and repeat are one element long (using memcpy() on each
    // row) but has to
    // copy one element at a time otherwise.
    int cols = d_cMax - d_cMin + 1;
    int linelen = cols * sizeof(data[0]);
    if ((colStride == 1) && (repeat == 1)) {
        const vrpn_float32 *msgbuf = (const vrpn_float32 *)d_valBuf;
        for (unsigned d = d_dMin; d <= d_dMax; d++) {
            for (unsigned r = d_rMin; r <= d_rMax; r++) {
                unsigned rActual;
                if (invert_rows) {
                    rActual = (nRows - 1) - r;
                }
                else {
                    rActual = r;
                }
                memcpy(&data[d * depthStride + rActual * rowStride + d_cMin],
                       msgbuf, linelen);
                msgbuf += linelen;
            }
        }
    }
    else {
        long rowStep = rowStride;
        if (invert_rows) {
            rowStep *= -1;
        }
        const vrpn_float32 *msgbuf = (const vrpn_float32 *)d_valBuf;
        for (unsigned d = d_dMin; d <= d_dMax; d++) {
            vrpn_float32 *rowStart =
                &data[d * depthStride + d_rMin * rowStride + d_cMin * repeat];
            if (invert_rows) {
                rowStart =
                    &data[d * depthStride + (nRows - 1 - d_rMin) * rowStride +
                          d_cMin * repeat];
            }
            vrpn_float32 *copyTo = rowStart;
            for (unsigned r = d_rMin; r <= d_rMax; r++) {
                for (unsigned c = d_cMin; c <= d_cMax; c++) {
                    for (unsigned rpt = 0; rpt < repeat; rpt++) {
                        *(copyTo + rpt) = *msgbuf; //< Copy the current element
                    }
                    msgbuf++;            //< Skip to the next buffer location
                    copyTo += colStride; //< Skip appropriate number of elements
                }
                rowStart += rowStep; //< Skip to the start of the next row
                copyTo = rowStart;
            }
        }
    }

    // Swap endian-ness of the buffer if we are on a big-endian machine.
    if (vrpn_big_endian) {
        fprintf(stderr, "XXX Imager Region needs swapping on Big-endian\n");
        return false;
    }

    return true;
}

vrpn_ImagerPose::vrpn_ImagerPose(const char *name, vrpn_Connection *c)
    : vrpn_BaseClass(name, c)
{
    vrpn_BaseClass::init();

    d_origin[0] = d_origin[1] = d_origin[2] = 0.0;
    d_dCol[0] = d_dCol[1] = d_dCol[2] = 0.0;
    d_dRow[0] = d_dRow[1] = d_dRow[2] = 0.0;
    d_dDepth[0] = d_dDepth[1] = d_dDepth[2] = 0.0;
};

int vrpn_ImagerPose::register_types(void)
{
    d_description_m_id =
        d_connection->register_message_type("vrpn_ImagerPose Description");
    if (d_description_m_id == -1) {
        return -1;
    }
    else {
        return 0;
    }
}

bool vrpn_ImagerPose::compute_pixel_center(vrpn_float64 *center,
                                           const vrpn_Imager &image,
                                           vrpn_uint16 col, vrpn_uint16 row,
                                           vrpn_uint16 depth)
{
    // Make sure we don't have a NULL pointer to fill our return into.
    if (center == NULL) {
        fprintf(
            stderr,
            "vrpn_ImagerPose::compute_pixel_center(): NULL center pointer\n");
        return false;
    }

    // Ensure that the pixel coordinate is within bounds
    if ((col >= image.nCols()) || (row >= image.nRows()) ||
        (depth >= image.nDepth())) {
        fprintf(stderr, "vrpn_ImagerPose::compute_pixel_center(): Pixel index "
                        "out of range\n");
        return false;
    }

    // The pixel centers are located at the midpoint in row, column, and
    // depth space of each pixel.  The pixel range is therefore half a
    // pixel past the centers of all pixels in the image.  This means that
    // there is one extra pixel-step included in the range (half to the
    // negative of the origin, half to the positive of the last entry).
    // So, the pixel center is one half-step plus a number of whole steps
    // equal to its index.
    vrpn_float64 stepC = 1.0 / image.nCols();
    vrpn_float64 stepR = 1.0 / image.nRows();
    vrpn_float64 stepD = 1.0 / image.nDepth();

    center[0] = d_origin[0] + (0.5 + col) * stepC * d_dCol[0] +
                (0.5 + row) * stepR * d_dRow[0] +
                (0.5 + depth) * stepD * d_dDepth[0];
    center[1] = d_origin[1] + (0.5 + col) * stepC * d_dCol[1] +
                (0.5 + row) * stepR * d_dRow[1] +
                (0.5 + depth) * stepD * d_dDepth[1];
    center[2] = d_origin[2] + (0.5 + col) * stepC * d_dCol[2] +
                (0.5 + row) * stepR * d_dRow[2] +
                (0.5 + depth) * stepD * d_dDepth[2];

    return true;
}

vrpn_ImagerPose_Server::vrpn_ImagerPose_Server(
    const char *name, const vrpn_float64 origin[3], const vrpn_float64 dCol[3],
    const vrpn_float64 dRow[3], const vrpn_float64 *dDepth, vrpn_Connection *c)
    : vrpn_ImagerPose(name, c)
{
    memcpy(d_origin, origin, sizeof(d_origin));
    memcpy(d_dCol, dCol, sizeof(d_dCol));
    memcpy(d_dRow, dRow, sizeof(d_dRow));
    if (dDepth != NULL) {
        memcpy(d_dDepth, dDepth, sizeof(d_dDepth));
    }

    // Set up callback handler for ping message from client so that it
    // sends the description.  This will make sure that the other side has
    // heard the descrption before it hears a region message.  Also set this up
    // to fire on the "new connection" system message.

    register_autodeleted_handler(d_ping_message_id, handle_ping_message, this,
                                 d_sender_id);
    register_autodeleted_handler(
        d_connection->register_message_type(vrpn_got_connection),
        handle_ping_message, this, vrpn_ANY_SENDER);
};

bool vrpn_ImagerPose_Server::set_range(const vrpn_float64 origin[3],
                                       const vrpn_float64 dCol[3],
                                       const vrpn_float64 dRow[3],
                                       const vrpn_float64 *dDepth)
{
    memcpy(d_origin, origin, sizeof(d_origin));
    memcpy(d_dCol, dCol, sizeof(d_dCol));
    memcpy(d_dRow, dRow, sizeof(d_dRow));
    if (dDepth != NULL) {
        memcpy(d_dDepth, dDepth, sizeof(d_dDepth));
    }
    return send_description();
}

bool vrpn_ImagerPose_Server::send_description(void)
{
    // msgbuf must be float64-aligned!
    vrpn_float64 fbuf[vrpn_CONNECTION_TCP_BUFLEN / sizeof(vrpn_float64)];
    char *msgbuf = (char *)fbuf;
    int buflen = sizeof(fbuf);
    struct timeval timestamp;

    // Pack the description of all of the fields in the imager into the buffer,
    // including the channel descriptions.
    if (vrpn_buffer(&msgbuf, &buflen, d_origin[0]) ||
        vrpn_buffer(&msgbuf, &buflen, d_origin[1]) ||
        vrpn_buffer(&msgbuf, &buflen, d_origin[2]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dDepth[0]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dDepth[1]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dDepth[2]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dRow[0]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dRow[1]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dRow[2]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dCol[0]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dCol[1]) ||
        vrpn_buffer(&msgbuf, &buflen, d_dCol[2])) {
        fprintf(stderr, "vrpn_ImagerPose_Server::send_description(): Can't "
                        "pack message header, tossing\n");
        return false;
    }

    // Pack the buffer into the connection's outgoing reliable queue, if we have
    // a valid connection.
    vrpn_int32 len = sizeof(fbuf) - buflen;
    vrpn_gettimeofday(&timestamp, NULL);
    if (d_connection &&
        d_connection->pack_message(len, timestamp, d_description_m_id,
                                   d_sender_id, (char *)(void *)fbuf,
                                   vrpn_CONNECTION_RELIABLE)) {
        fprintf(stderr, "vrpn_ImagerPose_Server::send_description(): cannot "
                        "write message: tossing\n");
        return false;
    }

    return true;
}

int vrpn_ImagerPose_Server::handle_ping_message(void *userdata,
                                                vrpn_HANDLERPARAM)
{
    vrpn_ImagerPose_Server *me = (vrpn_ImagerPose_Server *)userdata;
    me->send_description();
    return 0;
}

void vrpn_ImagerPose_Server::mainloop(void) { server_mainloop(); }

vrpn_ImagerPose_Remote::vrpn_ImagerPose_Remote(const char *name,
                                               vrpn_Connection *c)
    : vrpn_ImagerPose(name, c)
{
    // Register the handlers for the description message
    register_autodeleted_handler(d_description_m_id, handle_description_message,
                                 this, d_sender_id);
}

int vrpn_ImagerPose_Remote::handle_description_message(void *userdata,
                                                       vrpn_HANDLERPARAM p)
{
    const char *bufptr = p.buffer;
    vrpn_ImagerPose_Remote *me = (vrpn_ImagerPose_Remote *)userdata;

    // Get my new information from the buffer
    if (vrpn_unbuffer(&bufptr, &me->d_origin[0]) ||
        vrpn_unbuffer(&bufptr, &me->d_origin[1]) ||
        vrpn_unbuffer(&bufptr, &me->d_origin[2]) ||
        vrpn_unbuffer(&bufptr, &me->d_dDepth[0]) ||
        vrpn_unbuffer(&bufptr, &me->d_dDepth[1]) ||
        vrpn_unbuffer(&bufptr, &me->d_dDepth[2]) ||
        vrpn_unbuffer(&bufptr, &me->d_dRow[0]) ||
        vrpn_unbuffer(&bufptr, &me->d_dRow[1]) ||
        vrpn_unbuffer(&bufptr, &me->d_dRow[2]) ||
        vrpn_unbuffer(&bufptr, &me->d_dCol[0]) ||
        vrpn_unbuffer(&bufptr, &me->d_dCol[1]) ||
        vrpn_unbuffer(&bufptr, &me->d_dCol[2])) {
        return -1;
    }

    // Go down the list of callbacks that have been registered.
    // Fill in the parameter and call each.
    me->d_description_list.call_handlers(p.msg_time);

    return 0;
}

void vrpn_ImagerPose_Remote::mainloop(void)
{
    client_mainloop();
    if (d_connection) {
        d_connection->mainloop();
    };
}
