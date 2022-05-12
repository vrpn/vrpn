#include <math.h>                       // for pow
#include <stdio.h>                      // for fprintf, stderr, NULL, etc
#include <stdlib.h>                     // for rand

#include "vrpn_Configure.h"             // for VRPN_CALLBACK
#include "vrpn_Connection.h"
#include "vrpn_Imager.h"                // for vrpn_IMAGERREGIONCB, etc
#include "vrpn_Types.h"                 // for vrpn_uint16

//-----------------------------------------------------------------
// This section contains a copy of an image that is shared between
// the client and server.  It is completely an artifact of this test
// program.  The program works by sending the whole image one piece
// at a time from the server to the client; the client then compares
// with the image to make sure it matches.

vrpn_uint16 *image = NULL;	  //< Holds the image to be sent and tested
const vrpn_uint16 image_x_size = 256;
const vrpn_uint16 image_y_size = 128;

// Allocate the image and fill it with random numbers
bool  init_test_image(void)
{
  vrpn_uint16 x,y;

  if ( (image = new vrpn_uint16[image_x_size * image_y_size]) == NULL) {
    fprintf(stderr, "Could not allocate image\n");
    return false;
  }
  for (x = 0; x < image_x_size; x++) {
    for (y = 0; y < image_y_size; y++) {
      image[x + y*image_x_size] = rand();
    }
  }

  return true;
}

//-----------------------------------------------------------------
// This section contains code that does what the server should do

vrpn_Connection  *svrcon;	//< Connection for server to talk on
vrpn_Imager_Server	      *svr;	//< Image server to be used to send
int			      svrchan;	//< Server channel index for image data

bool  init_server_code(void)
{
  if ( (svrcon = vrpn_create_server_connection()) == NULL) {
    fprintf(stderr, "Could not open server connection\n");
    return false;
  }
  if ( (svr = new vrpn_Imager_Server("TestImage", svrcon,
    image_x_size, image_y_size)) == NULL) {
    fprintf(stderr, "Could not open Imager Server\n");
    return false;
  }
  if ( (svrchan = svr->add_channel("value", "unsigned16bit", 0, (float)(pow(2.0,16)-1))) == -1) {
    fprintf(stderr, "Could not add channel to server image\n");
    return false;
  }

  return true;
}

void  mainloop_server_code(void)
{
  static  vrpn_uint16 y = 0;	//< Loops through the image and sends data

  // Send the current row over to the client.
  svr->send_region_using_base_pointer(svrchan, 0, image_x_size-1, y,y, image, 1, image_x_size);
  svr->mainloop();
  printf("Sent a region\n");

  // Go to the next line for next time.
  if (++y == image_y_size) { y = 0; }

  // Mainloop the server connection (once per server mainloop, not once per object)
  svrcon->mainloop();
}

//-----------------------------------------------------------------
// This section contains code that does what the client should do.
// It listens until it has gotten twice as many rows as there are in
// the image and then indicates that it is done.

vrpn_Imager_Remote	*clt;	  //< Client imager
int   client_rows_gotten = 0;	  //< Keeps track of how many rows we've gotten
bool  client_done = false;	  //< Lets us know if the client wants to quit

// Handler to get an image region in and check it against the image to make
// sure it came over okay.  Also keeps track of how many rows we have.

void  VRPN_CALLBACK handle_region_data(void *, const vrpn_IMAGERREGIONCB info)
{
  unsigned x,y;
  const	vrpn_Imager_Channel *chan;

  // Find out the scale and offset for this channel.
  if ( (chan = clt->channel(info.region->d_chanIndex)) == NULL) {
    fprintf(stderr, "Warning: Illegal channel index (%d) in region report\n", info.region->d_chanIndex);
    return;
  }
  double scale = chan->scale;
  double offset = chan->offset;

  // Check the image data against the image to make sure it matches.
  for (y = info.region->d_rMin; y <= info.region->d_rMax; y++) {
    for (x = info.region->d_cMin; x <= info.region->d_cMax; x++) {
      vrpn_uint16 val;
      if (!info.region->read_unscaled_pixel(x,y,val)) {
	fprintf(stderr, "Error indexing region that was read\n");
	client_done = true;
	return;
      }
      val = (vrpn_uint16)(val * scale + offset);
      if (image[x+y*image_x_size] != val) {
	fprintf(stderr, "Error: Read pixel (%d) does not match stored pixel (%d)\n",
	  val, image[x+y*image_x_size]);
	client_done = true;
	return;
      }
    }
  }

  // One more region... are we done?
  printf("Got a region\n");
  if (++client_rows_gotten >= 2*image_y_size) {
    printf("Got %d rows -- success!\n", client_rows_gotten);
    client_done = true;
  }
}

bool  init_client_code(void)
{
  // Open the client object and set the callback handler for new region data
  if ( (clt = new vrpn_Imager_Remote("TestImage@localhost")) == NULL) {
    fprintf(stderr, "Error: Cannot create remote image object\n");
    return false;
  }
  if (clt->register_region_handler(NULL, handle_region_data) == -1) {
    fprintf(stderr, "Error: cannot register handler for regions\n");
    return false;
  }

  // Set callback handler for new description data (user code might not do this).

  return true;
}

void  mainloop_client_code(void)
{
  clt->mainloop();
}

//-----------------------------------------------------------------
// Mostly just calls the above functions; split into client and
// server parts is done clearly to help people who want to use this
// as example code.  You could pull the above functions down into
// the main() program body without trouble (except for the callback
// handler, of course).

int main(int, char *[])
{
  if (!init_test_image()) { return -1; }
  if (!init_server_code()) { return -1; }
  if (!init_client_code()) { return -1; }

  while (!client_done) {
    mainloop_server_code();
    mainloop_client_code();
  }

  if (clt) { delete clt; }
  if (svr) { delete svr; }

  svrcon->removeReference();

  return 0;
}
