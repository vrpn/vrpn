#include "vrpn_Text.h"
#include <stdio.h>
#include <string.h>
#ifndef	_WIN32
#include <netinet/in.h>
#endif

vrpn_Text::vrpn_Text(char *name, vrpn_Connection *c){
	char* servicename;
        servicename = vrpn_copy_service_name(name);

	if(c)
	    connection = c;
	else
	    connection = vrpn_get_connection_by_name(name);
        if( connection != NULL){
            my_id = connection->register_sender(servicename);
            message_id  = connection->register_message_type("vrpn Text ascii type");
            if((my_id == -1) ||(message_id  == -1)){
                fprintf(stderr, "vrpn_Text: Can't register IDs\n");
                connection = NULL;
            }
        }
}

void vrpn_Text::mainloop(){
	connection->mainloop();
}

vrpn_Connection *vrpn_Text::connectionPtr(){
        return connection;

}

vrpn_int32 vrpn_Text::encode_to (char * buf, vrpn_uint32 type,
                          vrpn_uint32 level, const char * msg){

  vrpn_uint32 net_type;
  vrpn_uint32 net_level;

  net_type  = htonl(type);
  net_level = htonl(level);  // reorder to network byte order

  memcpy(buf, &net_type, sizeof(net_type));
  memcpy(buf + sizeof(net_type), &net_level, sizeof(net_level));
  strncpy(buf + sizeof(net_type) + sizeof(net_level), msg,
          vrpn_MAX_TEXT_LEN);

  return 0;
}


vrpn_int32 vrpn_Text::decode_to(char *msg, vrpn_uint32 *type,
                         vrpn_uint32 *level,const char *buf){

  vrpn_uint32 net_level;
  vrpn_uint32 net_type;

  memcpy(&net_type, buf, sizeof(net_type));
  memcpy(&net_level, buf + sizeof(net_type), sizeof(net_level)); 
  *type = ntohl(net_type);
  *level = ntohl(net_level);

  strncpy(msg, buf + sizeof(net_type)+ sizeof(net_level),
          vrpn_MAX_TEXT_LEN);

  return 0;	
}


int vrpn_Text_Sender::send_message(const char *msg,
                            vrpn_uint32 type,
                            vrpn_uint32 level)
{
        /*vrpn_Connection *connection = new vrpn_Connection();
        vrpn_int32 my_type = connection->register_message_type("Test_type");
        vrpn_int32 my_id = connection->register_sender("Test0@ioglab.cs.unc.edu");
	*/

	char buffer [2 * sizeof(vrpn_int32) + vrpn_MAX_TEXT_LEN];
        struct timeval now;

        gettimeofday(&now,NULL);

	// send both level and message

        encode_to(buffer,  type, level, msg);
        connection->pack_message(sizeof(buffer), now, message_id, my_id,
                                 buffer, vrpn_CONNECTION_RELIABLE);  // TCH
        connection->mainloop();
	return 0;
}


vrpn_Text_Receiver::vrpn_Text_Receiver (char * name,
                                        vrpn_Connection * c) :  // TCH
    vrpn_Text(name, c),
    change_list (NULL)  // TCH
{
  if (connectionPtr())
    connectionPtr()->register_handler(message_id, handle_message, this, my_id);
};

vrpn_Text_Receiver::~vrpn_Text_Receiver()
{
	vrpn_TEXTMESSAGELIST	*next;

	// Unregister all of the handlers that have been registered with the
	// connection so that they won't yank once the object has been deleted.
	if (connectionPtr()) {
	  if (connectionPtr()->unregister_handler(message_id, handle_message,
		this, my_id)) {
		fprintf(stderr,"vrpn_Text_Receiver: can't unregister handler\n");
		fprintf(stderr,"   (internal VRPN error -- expect a seg fault)\n");
	  }
	}

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
 
  // FILL IN THE BLANK HERE
  //FILL IN THE BLANK HERE

// temporary code  by Lin Cui on 09/04/98
  me->decode_to(cp.message, &cp.type, &cp.level,p.buffer);

  // Go down the list of callbacks that have been registered.
  // Fill in the parameter and call each.
  while (list != NULL) {
    list->handler(list->userdata, cp);
    list = list->next;
  }
  return 0;
}


