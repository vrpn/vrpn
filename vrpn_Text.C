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
        gettimeofday(&now,NULL);

	// send type, level and message
	return send_text_message(msg, now, type, level);
}


vrpn_Text_Receiver::vrpn_Text_Receiver (const char * name,
                                        vrpn_Connection * c) :
    vrpn_BaseClass(name, c),
    change_list (NULL)
{
  init();
  if (d_connection) {
    register_autodeleted_handler(d_text_message_id, handle_message, this, d_sender_id);
  }
};

vrpn_Text_Receiver::~vrpn_Text_Receiver()
{
	vrpn_TEXTMESSAGELIST	*next;

	// Delete all of the callback handlers that other code had registered
	// with this object. This will free up the memory taken by the list
	while (change_list != NULL) {
		next = change_list->next;
		delete change_list;
		change_list = next;
	}
}

int vrpn_Text_Receiver::register_message_handler(void *userdata,
                vrpn_TEXTHANDLER handler){
  vrpn_TEXTMESSAGELIST *new_entry;

  // Ensure that the handler is non-NULL
  if (handler == NULL){
    fprintf(stderr,
            "vrpn_Text_Remote::register_handler: NULL handler\n");
    return -1;
  }

  // Allocate and initialize the new entry
  if ( (new_entry = new vrpn_TEXTMESSAGELIST) == NULL) {
    fprintf(stderr,
            "vrpn_Text::register_handler: Out of memory\n");
    return -1;
  }
  new_entry->handler = handler;
  new_entry->userdata = userdata;

  // Add this handler to the chain at the beginning (don't check to see
  // if it is already there, since duplication is okay).
  new_entry->next = change_list;
  change_list = new_entry;

  return 0;
}

int vrpn_Text_Receiver::unregister_message_handler(void *userdata,
                vrpn_TEXTHANDLER handler){
  // The pointer at *snitch points to victim
        vrpn_TEXTMESSAGELIST   *victim, **snitch;

        // Find a handler with this registry in the list (any one will do,
        // since all duplicates are the same).
        snitch = &change_list;
        victim = *snitch;
        while ( (victim != NULL) &&
                ( (victim->handler != handler) ||
                  (victim->userdata != userdata) )) {
            snitch = &( (*snitch)->next );
            victim = victim->next;
        }

        // Make sure we found one
        if (victim == NULL) {
                fprintf(stderr,
                   "vrpn_Text::unregister_handler: No such handler\n");
                return -1;
        }

        // Remove the entry from the list
        *snitch = victim->next;
        delete victim;
        return 0;
}


int vrpn_Text_Receiver::handle_message (void * userdata, vrpn_HANDLERPARAM p) {

  vrpn_Text_Receiver * me = (vrpn_Text_Receiver *) userdata;
  vrpn_TEXTCB cp;
  vrpn_TEXTMESSAGELIST * list =  me->change_list;

  cp.msg_time = p.msg_time;
  me->decode_text_message_from_buffer(cp.message, &cp.type, &cp.level, p.buffer);
  
  // Go down the list of callbacks that have been registered.
  // Fill in the parameter and call each.
  while (list != NULL) {
    list->handler(list->userdata, cp);
    list = list->next;
  }
  return 0;
}


