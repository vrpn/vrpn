#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#include "vrpn_Ohmmeter.h"

#ifndef _WIN32
#include <netinet/in.h>
#endif

#include "vrpn_cygwin_hack.h"

#define ASSERT(x)	assert(x)

vrpn_Ohmmeter::vrpn_Ohmmeter(char *name, vrpn_Connection *c)
{
    // Set our connection to the one passed in 
  char * servicename;
  servicename = vrpn_copy_service_name(name);
    connection = c;

    // Register this ohmmeter and the needed message types
    if (connection) {
	my_id = connection->register_sender(servicename);

	channelset_m_id = connection->register_message_type("Channel Settings");
	measure_m_id = connection->register_message_type("Resistance");
	setchannel_m_id = connection->register_message_type("Set Channel");
    }
    timestamp.tv_sec = 0;
    timestamp.tv_usec = 0;
    channel = 0;
    for (vrpn_int32 i = 0; i < MAX_OHMMETER_CHANNELS; i++){
	range_min[i] = 5000;	// Ohms
	voltage[i] = 1.0;	// mV
	filter[i] = 1.0;	// sec
    	status[i] = CHANGING;
    	resistance[i] = 0;
	error[i] = 0;
    }
    num_channels = 1;
  if (servicename)
    delete [] servicename;
}

#define SETOHM_MESSAGE_SIZE (4*sizeof(vrpn_float64))
vrpn_int32 vrpn_Ohmmeter::encode_setchannel_to(char *buf)
{
    // Message includes: vrpn_int32 channel
    //                   vrpn_int32 enable
    //                   vrpn_float64 voltage
    //                   vrpn_float64 range_min
    //                   vrpn_float64 filter
    vrpn_float64 *dBuf = (vrpn_float64 *)buf;
    int index = 0;

    // XXX WARNING -- This assumes a certain byte-order!!!! Only works on PC!
    ASSERT(sizeof(vrpn_int32)==(sizeof(vrpn_float64)/2));
    ((vrpn_int32 *)dBuf)[0] = htonl(channel);
    ((vrpn_int32 *)dBuf)[1] = htonl(enabled[channel]);
    index++;

    dBuf[index++] = htond(voltage[channel]);
    dBuf[index++] = htond(range_min[channel]);
    dBuf[index++] = htond(filter[channel]);
    ASSERT(SETOHM_MESSAGE_SIZE == index*sizeof(vrpn_float64));
    return (index*sizeof(vrpn_float64));
}

#define MEASURE_MESSAGE_SIZE (3*sizeof(vrpn_float64))
vrpn_int32 vrpn_Ohmmeter::encode_measure_to(char *buf)
{
    // Message includes: vrpn_int32 channel
    //                   vrpn_int32 status
    //                   vrpn_float64 resistance
    //                   vrpn_float64 error
    vrpn_float64 *dBuf = (vrpn_float64 *)buf;
    int index = 0;

    // XXX WARNING -- This assumes a certain byte-order!!!! Only works on PC!
    ASSERT(sizeof(vrpn_int32)==(sizeof(vrpn_float64)/2));
    ((vrpn_int32 *)dBuf)[0] = htonl(channel);
    ((vrpn_int32 *)dBuf)[1] = htonl(status[channel]);
    index++;

    dBuf[index++] = htond(resistance[channel]);
    dBuf[index++] = htond(error[channel]);
    ASSERT(MEASURE_MESSAGE_SIZE == (index*sizeof(vrpn_float64)));
    return (index*sizeof(vrpn_float64));
}

#define OHMSET_MESSAGE_SIZE (4*sizeof(vrpn_float64))
vrpn_int32 vrpn_Ohmmeter::encode_channelset_to(char *buf)
{
    // Message includes: vrpn_int32 channel
    //                   vrpn_int32 enable
    //                   vrpn_float64 voltage
    //                   vrpn_float64 range_min
    //                   vrpn_float64 filter
    vrpn_float64 *dBuf = (vrpn_float64 *)buf;
    int index = 0;

    // XXX WARNING -- This assumes a certain byte-order!!!! Only works on PC!
    ASSERT(sizeof(vrpn_int32)==(sizeof(vrpn_float64)/2));
    ((vrpn_int32 *)dBuf)[0] = htonl(channel);
    ((vrpn_int32 *)dBuf)[1] = htonl(enabled[channel]);
    index++;

    dBuf[index++] = htond(voltage[channel]);
    dBuf[index++] = htond(range_min[channel]);
    dBuf[index++] = htond(filter[channel]);
    ASSERT(OHMSET_MESSAGE_SIZE == index*sizeof(vrpn_float64));
    return (index*sizeof(vrpn_float64));
}

// Should this only be in the client?
#ifndef VRPN_CLIENT_ONLY
#ifdef _WIN32
static	unsigned long	duration(struct timeval t1, struct timeval t2)
{
	return (t1.tv_usec - t2.tv_usec) +
	       1000000L * (t1.tv_sec - t2.tv_sec);
}

vrpn_Ohmmeter_ORPX2::vrpn_Ohmmeter_ORPX2(char *name, vrpn_Connection *c = NULL, 
	vrpn_float32 hz):vrpn_Ohmmeter(name,c)
{
	last_channel_switch_time = timestamp;
    update_rate = hz;
    theORPX = new ORPX_SerialComm("COM1");
    for (vrpn_int32 i = 0; i < NUM_ORPX_CHANNELS; i++){
	channel_acquisition_time[i] = 10;	// seconds
	chan_data[i].channel = i;
	chan_data[i].Vmeas = NUM_ORPX_SETTINGS - 1;
	chan_data[i].range = 4;
	chan_data[i].filter = (vrpn_int32)((NUM_ORPX_SETTINGS -1)/2);
	// now set floats to the corresponding real values
	range_min[i] = orpx_ranges[chan_data[i].range];
	voltage[i] = orpx_voltages[chan_data[i].Vmeas];
	filter[i] = orpx_filters[chan_data[i].filter];
    }
    if (connection)
	if (connection->register_handler(setchannel_m_id,
	    handle_change_message, this, my_id))
		fprintf(stderr, "vrpn_ORPX2:can't register handler\n");
	change_list = NULL;
	this->register_change_handler(this, handle_channel_set);
}

void vrpn_Ohmmeter_ORPX2::mainloop(void) {
    struct timeval current_time;
    char msgbuf[1000];
    vrpn_int32 len;

    gettimeofday(&current_time, NULL);
    if (duration(current_time, timestamp) >= 1000000.0/update_rate) {
		//update the time
		timestamp.tv_sec = current_time.tv_sec;
		timestamp.tv_usec = current_time.tv_usec;
		if ((duration(current_time, last_channel_switch_time) >= 
			(1000000.0)*channel_acquisition_time[channel]) 
			|| (!enabled[channel])){
			// find next enabled channel and switch to it
			for (vrpn_int32 i=0; i < NUM_ORPX_CHANNELS; i++){
				channel = (channel+1)%NUM_ORPX_CHANNELS;
				if (enabled[channel]) break;
			}
			last_channel_switch_time = current_time;
		}
		if (!enabled[channel]) return; // => no channels enabled
		get_measurement_report();

		if (connection) {
			len = encode_measure_to(msgbuf);
			if (connection->pack_message(len, timestamp, measure_m_id,
			my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
			    fprintf(stderr, "ORPX2: can't write message: tossing\n");
			}
		}
    }
}

void vrpn_Ohmmeter_ORPX2::get_measurement_report(void) {
    if (theORPX) {
		if (!theORPX->read_and_write(resistance[channel],error[channel],
			chan_data[channel])){
			status[channel] = ITF_ERROR;
		}
		else if (chan_data[channel].saturated){
			status[channel] = M_OVERFLO;
		}
		else if (chan_data[channel].measurement < 5900)
			// too close to top of range (according to the
			// original ORPX software)
			status[channel] = R_OVERFLO;
		else if (chan_data[channel].measurement > 65536-328)
			// too close to bottom of range (according to the
			// original ORPX software)
			status[channel] = M_UNDERFLO;
		else{
			status[channel] = MEASURING;
		}
    }
}

// this function tries to satisfy the request within the limitations on the
// parameter settings of the ORPX2. Since the ORPX2 only allows certain
// values for parameters, this function finds the one that is closest to
// the value we request.
void vrpn_Ohmmeter_ORPX2::handle_channel_set(void *userdata, 
						const vrpn_SETOHMCB &info)
{

	vrpn_Ohmmeter_ORPX2 *me = (vrpn_Ohmmeter_ORPX2 *)userdata;
    vrpn_int32 i;
	vrpn_int32 len;
	char outputbuf[1000];
	struct timeval current_time;
    vrpn_float64 v_del, r_del, f_del;
    vrpn_int32 v_new = 0, r_new = 0, f_new = 0;
    // set parameters for channel number info.channel_num
    orpx_params_t *chan = &((me->chan_data)[info.channel_num]);
	vrpn_int32 current_channel = me->channel;
	me->channel = info.channel_num;
    (me->enabled)[info.channel_num] = info.enable;
    v_del = fabs(info.voltage - orpx_voltages[0]);
    r_del = fabs(info.range_min - orpx_ranges[0]);
    f_del = fabs(info.filter - orpx_filters[0]);
    for (i = 1; i < NUM_ORPX_SETTINGS; i++) {
	if (fabs(info.voltage - orpx_voltages[i]) < v_del) {
	    v_del = fabs(info.voltage -orpx_voltages[i]);
	    v_new = i;
	}
	if (fabs(info.range_min - orpx_ranges[i]) < r_del) {
	    r_del = fabs(info.range_min - orpx_ranges[i]);
	    r_new = i;
	}
	if (fabs(info.filter - orpx_filters[i]) < f_del) {
	    f_del = fabs(info.filter - orpx_filters[i]);
	    f_new = i;
	}
    }
	// we want to make sure that the minimum resistance in the range is
	// always less than that requested
    if ((info.range_min < orpx_ranges[r_new]) && (r_new != 0))
	r_new--;
    chan->Vmeas = v_new;
    chan->range = r_new;
    chan->filter = f_new;
	gettimeofday(&current_time, NULL);
	me->timestamp.tv_sec = current_time.tv_sec;
	me->timestamp.tv_usec = current_time.tv_usec;

	if (me->connection){	//send message saying that channel was set
		len = me->encode_channelset_to(outputbuf);
		if (me->connection->pack_message(len, me->timestamp, 
		    me->channelset_m_id, 
		    me->my_id, outputbuf,vrpn_CONNECTION_RELIABLE)) {
		      fprintf(stderr, "ORPX2: can't write message: tossing\n");
		}
	}
	me->channel = current_channel;
		
}

int vrpn_Ohmmeter_ORPX2::handle_change_message(void *userdata, 
							vrpn_HANDLERPARAM p)
{
    vrpn_Ohmmeter_ORPX2 *me = (vrpn_Ohmmeter_ORPX2 *)userdata;
    vrpn_float64 *params = (vrpn_float64 *)(p.buffer);
    vrpn_SETOHMCB info;
    vrpn_ORPXCHANGELIST *handler = me->change_list;

    if (p.payload_len != SETOHM_MESSAGE_SIZE){
	fprintf(stderr, "vrpn_ORPX: setohm message payload error\n");
	fprintf(stderr, "     (got %d, expected %d)\n",
		p.payload_len, SETOHM_MESSAGE_SIZE);
	return -1;
    }
    info.msg_time = p.msg_time;
    info.channel_num = ntohl( ((vrpn_int32 *)params)[0]);
    info.enable = ntohl( ((vrpn_int32 *)params)[1]);
    params++;
    info.voltage = ntohd(*params++);
    info.range_min = ntohd(*params++);
    info.filter = ntohd(*params++);

    while (handler != NULL) {
	handler->handler(handler->userdata, info);
	handler = handler->next;
    }
	return 0;
}

int vrpn_Ohmmeter_ORPX2::register_change_handler(void *userdata,
        vrpn_SETOHMCHANGEHANDLER handler)
{
    vrpn_ORPXCHANGELIST *new_entry;

    if (handler == NULL) {
        fprintf(stderr, "vrpn_ORPX::reg_handler:NULL handler\n");
        return -1;
    }
    if ((new_entry = new vrpn_ORPXCHANGELIST) == NULL) {
        fprintf(stderr, "vrpn_ORPX::reg_handler:out of memory\n");
        return -1;
    }
    new_entry->handler = handler;
    new_entry->userdata = userdata;
    
    new_entry->next = change_list;
    change_list = new_entry;
    return 0;
}

int vrpn_Ohmmeter_ORPX2::unregister_change_handler(void *userdata,
        vrpn_SETOHMCHANGEHANDLER handler)
{
    vrpn_ORPXCHANGELIST *victim, **snitch;

    snitch = &change_list;
    victim = *snitch;
    while ( (victim != NULL) &&
        ( (victim->handler != handler) ||
          (victim->userdata != userdata) )) {
          snitch = &( (*snitch)->next );
          victim = victim->next;
    }   
    
    if (victim == NULL) {
        fprintf(stderr,"vrpn_ORPX::unreg_handler: No such handler\n");
        return -1;
    }
    // Remove the entry from the list
    *snitch = victim->next;
    delete victim;
    
    return 0;
}

#endif // _WIN32
#endif // VRPN_CLIENT_ONLY

vrpn_Ohmmeter_Remote::vrpn_Ohmmeter_Remote(char *name, vrpn_Connection *c):
	vrpn_Ohmmeter(name, vrpn_get_connection_by_name(name))
{
	if (c == NULL)
		connection = vrpn_get_connection_by_name(name);
	else
		connection = c;
    ohmset_change_list = NULL;
    measure_change_list = NULL;
    if (connection == NULL) {
	fprintf(stderr,"vrpn_Ohmmeter_Remote: No connection\n");
	return;
    }

    // Register a handler for the measurement change callback
    if (connection->register_handler(measure_m_id, handle_measurement_message,
	this, my_id)) {
	    fprintf(stderr, 
		"vrpn_Ohmmeter_Remote: can't register meas. handler\n");
	    connection = NULL;
	    return;
    }
    if (connection->register_handler(channelset_m_id, handle_ohmset_message,
	this, my_id)) {
	    fprintf(stderr,
		 "vrpn_Ohmmeter_Remote: can't register ohmset handler\n");
	    connection = NULL;
	    return;
    }

    if (connection->get_File_Connection()){
	fprintf(stderr, "WARNING: Reading ohmmeter messages from all"
		" senders recorded in log file\n");
	if (connection->register_handler(measure_m_id,
		    handle_measurement_message, this, vrpn_ANY_SENDER)) {
	    fprintf(stderr, "vrpn_Ohmmeter_Remote: can't register handler1\n");
	}
	if (connection->register_handler(channelset_m_id,
		     handle_ohmset_message, this, vrpn_ANY_SENDER)) {
	    fprintf(stderr, "vrpn_Ohmmeter_Remote: can't register handler2\n");
	}
    }
    
    
    gettimeofday(&timestamp, NULL);
}

void vrpn_Ohmmeter_Remote::mainloop(void)
{
	if (connection) connection->mainloop();
}

int vrpn_Ohmmeter_Remote::set_channel_parameters(vrpn_int32 chan, vrpn_int32 enable,
	vrpn_float64 volt, vrpn_float64 r_min, vrpn_float64 filt)
{
    char msgbuf[1000];
    vrpn_int32 len;
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    timestamp.tv_sec = current_time.tv_sec;
    timestamp.tv_usec = current_time.tv_usec;

    channel = chan;
    enabled[channel] = enable;
    voltage[channel] = volt;
    range_min[channel] = r_min;
    filter[channel] = filt;

    if(connection) {
	len = encode_setchannel_to(msgbuf);
	if (connection->pack_message(len,timestamp,setchannel_m_id,
		my_id, msgbuf, vrpn_CONNECTION_RELIABLE)) {
		fprintf(stderr, "vrpn_Ohm_Rem: can't write message:tossing\n");
		return -1;
	}
	connection->mainloop();
    }
    return 0;
}

int vrpn_Ohmmeter_Remote::register_measurement_handler(void *userdata,
	vrpn_OHMMEASUREMENTHANDLER handler)
{
    vrpn_OHMMEASCHANGELIST *new_entry;

    if (handler == NULL) {
	fprintf(stderr, "vrpn_Ohm_Rem::reg_handler:NULL handler\n");
	return -1;
    }
    if ((new_entry = new vrpn_OHMMEASCHANGELIST) == NULL) {
	fprintf(stderr, "vrpn_Ohm_Rem::reg_handler:out of memory\n");
	return -1;
    }
    new_entry->handler = handler;
    new_entry->userdata = userdata;

    new_entry->next = measure_change_list;
    measure_change_list = new_entry;
    return 0;
}

int vrpn_Ohmmeter_Remote::unregister_measurement_handler(void *userdata,
	vrpn_OHMMEASUREMENTHANDLER handler)
{
    vrpn_OHMMEASCHANGELIST *victim, **snitch;

    snitch = &measure_change_list;
    victim = *snitch;
    while ( (victim != NULL) &&
	( (victim->handler != handler) ||
	  (victim->userdata != userdata) )) {
	  snitch = &( (*snitch)->next );
	  victim = victim->next;
    }

    if (victim == NULL) {
	fprintf(stderr,"vrpn_Ohm_Rem::unreg_handler: No such handler\n");
	return -1;
    }
    // Remove the entry from the list
    *snitch = victim->next;
    delete victim;

    return 0;
}

int vrpn_Ohmmeter_Remote::register_ohmset_handler(void *userdata,
        vrpn_OHMSETCHANGEHANDLER handler)
{
    vrpn_OHMSETCHANGELIST *new_entry;

    if (handler == NULL) {
        fprintf(stderr, "vrpn_Ohm_Rem::reg_handler:NULL handler\n");
        return -1;
    }
    if ((new_entry = new vrpn_OHMSETCHANGELIST) == NULL) {
        fprintf(stderr, "vrpn_Ohm_Rem::reg_handler:out of memory\n");
        return -1;
    }
    new_entry->handler = handler;
    new_entry->userdata = userdata;
    
    new_entry->next = ohmset_change_list;
    ohmset_change_list = new_entry;
    return 0;
}

int vrpn_Ohmmeter_Remote::unregister_ohmset_handler(void *userdata,
        vrpn_OHMSETCHANGEHANDLER handler)
{
    vrpn_OHMSETCHANGELIST *victim, **snitch;

    snitch = &ohmset_change_list;
    victim = *snitch;
    while ( (victim != NULL) &&
        ( (victim->handler != handler) ||
          (victim->userdata != userdata) )) {
          snitch = &( (*snitch)->next );
          victim = victim->next;
    }   
    
    if (victim == NULL) {
        fprintf(stderr,"vrpn_Ohm_Rem::unreg_handler: No such handler\n");
        return -1;
    }
    // Remove the entry from the list
    *snitch = victim->next;
    delete victim;
    
    return 0;
}
int vrpn_Ohmmeter_Remote::handle_measurement_message(void *userdata,
						vrpn_HANDLERPARAM p)
{
    vrpn_Ohmmeter_Remote *me = (vrpn_Ohmmeter_Remote *)userdata;
    vrpn_float64 *params = (vrpn_float64 *)(p.buffer);
    vrpn_OHMMEASUREMENTCB info;
    vrpn_OHMMEASCHANGELIST *handler = me->measure_change_list;

    if (p.payload_len != MEASURE_MESSAGE_SIZE){
        fprintf(stderr, "vrpn_Ohm_Rem: measure message payload error\n");
        fprintf(stderr, "     (got %d, expected %d)\n",
                p.payload_len, MEASURE_MESSAGE_SIZE);
        return -1;
    }
    info.msg_time = p.msg_time;
    info.channel_num = ntohl( ((vrpn_int32 *)params)[0]);
    info.status = ntohl( ((vrpn_int32 *)params)[1]);
    params++;
    info.resistance = ntohd(*params++);
    info.error = ntohd(*params++);

    while (handler != NULL) {
        handler->handler(handler->userdata, info);
        handler = handler->next;
    }
	return 0;
}

int vrpn_Ohmmeter_Remote::handle_ohmset_message(void *userdata,
                                                vrpn_HANDLERPARAM p)
{
    vrpn_Ohmmeter_Remote *me = (vrpn_Ohmmeter_Remote *)userdata;
    vrpn_float64 *params = (vrpn_float64 *)(p.buffer);
    vrpn_OHMSETCB info;
    vrpn_OHMSETCHANGELIST *handler = me->ohmset_change_list;

    if (p.payload_len != OHMSET_MESSAGE_SIZE){
        fprintf(stderr, "vrpn_Ohm_Rem: ohmset message payload error\n");
        fprintf(stderr, "     (got %d, expected %d)\n",
                p.payload_len, OHMSET_MESSAGE_SIZE);
        return -1;
    }
    info.msg_time = p.msg_time;
    info.channel_num = ntohl( ((vrpn_int32 *)params)[0]);
    info.enabled = ntohl( ((vrpn_int32 *)params)[1]);
    params++;
    info.voltage = ntohd(*params++);
    info.range_min = ntohd(*params++);
    info.filter = ntohd(*params++);

    while (handler != NULL) {
        handler->handler(handler->userdata, info);
        handler = handler->next;
    }
	return 0;
}

