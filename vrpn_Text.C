#include "vrpn_Text.h"
#include <stdio.h>
#include <string.h>
#ifndef	_WIN32
#include <netinet/in.h>
#endif

int vrpn_Text_Sender::send_message(const char *msg,
                            vrpn_TEXT_SEVERITY type,
                            vrpn_uint32 level)
{
        struct timeval now;
        vrpn_gettimeofday(&now,NULL);

	// send type, level and message
	return send_text_message(msg, now, type, level);
}


vrpn_Text_Receiver::vrpn_Text_Receiver (const char * name,
                                        vrpn_Connection * c) :
    vrpn_BaseClass(name, c)
{
  init();
  if (d_connection) {
    register_autodeleted_handler(d_text_message_id, handle_message, this, d_sender_id);
  }
};

vrpn_Text_Receiver::~vrpn_Text_Receiver()
{
}

int vrpn_Text_Receiver::handle_message (void * userdata, vrpn_HANDLERPARAM p) {

  vrpn_Text_Receiver * me = (vrpn_Text_Receiver *) userdata;
  vrpn_TEXTCB cp;

  cp.msg_time = p.msg_time;
  me->decode_text_message_from_buffer(cp.message, &cp.type, &cp.level, p.buffer);
  
  // Go down the list of callbacks that have been registered.
  // Fill in the parameter and call each.
  me->d_callback_list.call_handlers(cp);
  return 0;
}


