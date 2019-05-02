#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packetizer.h"
#include "rf.h"
#include "tock.h"

#define RF_DRIVER           (0xCC1352)

#define ALLOW_NUM_W             (1)
#define ALLOW_NUM_C             (2)

#define SUBSCRIBE_TX                  (1)
#define SUBSCRIBE_GET_ADDR            (2)


enum cmd {
  COMMAND_DRIVER_CHECK = 0,
  COMMAND_ENABLE = 1,
  COMMAND_RETURN_RADIO_STATUS = 2,
  COMMAND_STOP_RADIO_OPERATION = 3,
  COMMAND_FORCE_STOP_RADIO = 4,
  COMMAND_SET_DEVICE_CONFIG = 5,
  COMMAND_SEND = 6,
  COMMAND_SET_ADDRESS = 7,
};

// Request Queue
static rf_req_t *rf_queue = NULL;
static bool queue_empty   = false;
int rf_dispatch_request(rf_req_t* req);

bool fired;


static void tx_done_callback(int result,
                             __attribute__ ((unused)) int arg2,
                             __attribute__ ((unused)) int arg3,
                             void* completed_request) {
  
 //  rf_req_t* cur_req = (rf_req_t*) completed_request;

 //  // keep local copy of callback and next item
 //  rf_cb cb       = cur_req->user_cb;
 //  rf_req_t* next = cur_req->next;

 //  // fire callback

 //  if (cb != NULL) {
	// if (cur_req->payload->type == PL_TYPE_PACKETIZER) {
	//   (*cb)(result, cur_req->payload);
	//   // free current request
	//   // packet_disassemble(cur_req->payload->packet);
	//   // free(cur_req->payload);
	//   // free(cur_req);
	// } else if (cur_req->payload->type == PL_TYPE_NONE) {
	//   (*cb)(result, cur_req->payload);
	//   // free(cur_req->payload->raw_packet->data);
	//   // free(cur_req->payload->packet);
	//   // free(cur_req->payload);
	//   // free(cur_req);

	// }
 //  }
  
 //  // dispatch next request
 //  if (next != NULL) {
 //    rf_dispatch_request(next);
 //  } else {
 //    // eat the tail of the queue
 //    rf_queue    = NULL;
 //    queue_empty = true;
 //  }
  fired = true;
}

static bool got_addr = false;
static void get_addr_cb(int result,
  __attribute__ ((unused)) int arg2,
  __attribute__ ((unused)) int arg3,
  void* data) {
  got_addr = true;
  uint32_t * device_id = (uint32_t*) data;
  *device_id = result;
};

int rf_driver_check(void) {
  return command(RF_DRIVER, COMMAND_DRIVER_CHECK, 0, 0);
}

int rf_enable(void) {
  return command(RF_DRIVER, COMMAND_ENABLE, 0, 0);
}

int rf_set_oui(unsigned char *address) {
  if (!address) return TOCK_EINVAL;
  int err = subscribe(RF_DRIVER, SUBSCRIBE_GET_ADDR, 0, 0);
  if (err < 0) return err;
  return command(RF_DRIVER, COMMAND_SET_ADDRESS, 0, 0);
}

int rf_get_device_id(uint32_t * device_id) {
  got_addr = false;  
  int err = subscribe(RF_DRIVER, SUBSCRIBE_GET_ADDR, get_addr_cb, (void *) device_id);
  if (err < 0) return err;
  yield_for(&got_addr);
}

int rf_dispatch_request(rf_req_t* req){
  int err;
  switch(req->payload->type){
	case PL_TYPE_NONE:
	  err = allow(RF_DRIVER, ALLOW_NUM_W, (void *) req->payload->raw_packet->data, req->payload->raw_packet->len);
	  if (err < 0) return err;

	  // Subscribe to the transmit callback
	  err = subscribe(RF_DRIVER, SUBSCRIBE_TX,
					  tx_done_callback, (void *) req);
	  if (err < 0) return err;

	  // Issue the send command and wait for the transmission to be done.
	  return command(RF_DRIVER, COMMAND_SEND, PL_TYPE_NONE, 0);
	
	case PL_TYPE_PACKETIZER:
	  err = allow(RF_DRIVER, ALLOW_NUM_W, (void *) req->payload->packet->data, req->payload->packet->len);
	  if (err < 0) return err;

	  // Subscribe to the transmit callback
	  err = subscribe(RF_DRIVER, SUBSCRIBE_TX,
					  tx_done_callback, (void *) req);
	  if (err < 0) return err;

	  // Issue the send command and wait for the transmission to be done.
	  return command(RF_DRIVER, COMMAND_SEND, PL_TYPE_PACKETIZER, 0);
	case PL_TYPE_CAUTERIZE:
	  return 0;
	default:
	  return 0;
  }
}

rf_req_t* allocate_raw_request(raw_packet_t* packet, rf_cb cb);
rf_req_t* allocate_request(packet_t* packet, rf_cb cb);

rf_req_t* allocate_request(packet_t* packet, rf_cb cb){
  rf_req_t* request = (rf_req_t*) calloc(1,sizeof(rf_req_t));

  if (request == NULL) {
    if (!queue_empty) {
      yield_for(&queue_empty);
      request = (rf_req_t*) calloc(1,sizeof(rf_req_t));
    }else {
      return NULL;
    }
    if (request == NULL) return NULL;
  }

  request->payload->type	= PL_TYPE_PACKETIZER;
  request->payload->packet	= packet;
  request->user_cb	= cb;
  return request;
}

rf_req_t* allocate_raw_request(raw_packet_t* packet, rf_cb cb){
  rf_req_t* request = (rf_req_t*) calloc(1,sizeof(rf_req_t));

  if (request == NULL) {
    if (!queue_empty) {
      yield_for(&queue_empty);
      request = (rf_req_t*) calloc(1,sizeof(rf_req_t));
    }else {
      return NULL;
    }
    if (request == NULL) return NULL;
  }
  
  request->payload->type		= PL_TYPE_NONE;
  request->payload->raw_packet	= packet;
  request->user_cb		= cb;
  return request;
}

int rf_send_async(packet_t* packet, rf_cb cb) {

  rf_req_t * request = allocate_request(packet, cb);

  // there is a queue
  if (rf_queue != NULL) {
    queue_empty = false;
    // insert self in queue
    request->next = request;
    request       = request;
    return TOCK_SUCCESS;
  }
  // otherwise, no queue so just execute now
  else{
    queue_empty = request;
    return rf_dispatch_request(request);
  }
}

int rf_send_raw_async(raw_packet_t* packet, rf_cb cb) {
  rf_req_t * request = allocate_raw_request(packet, cb);

  // there is a queue
  if (rf_queue != NULL) {
    queue_empty = false;
    // insert self in queue
    request->next = request;
    request       = request;
    return TOCK_SUCCESS;
  }
  // otherwise, no queue so just execute now
  else{
    queue_empty = request;
    return rf_dispatch_request(request);
  }
}
// Request Queue
static int dummy_result;
static void dummy_cb(int result, payload_t* req __attribute__ ((unused))){
  dummy_result = result;
  fired        = true;
}

int rf_send(packet_t* packet) {
  int err;
  fired = false;
  err   = rf_send_async(packet, &dummy_cb);
  if (err < 0) {
    packet_disassemble(packet);
    free(rf_queue);
    rf_queue = NULL;
    return err;
  }

  yield_for(&fired);
  return dummy_result;
}

int rf_send_raw(raw_packet_t* packet) {
  fired = false;
  
  uint err;
  err = allow(RF_DRIVER, ALLOW_NUM_W, (void *) packet->data, packet->len);
  if (err < 0) return err;

  // Subscribe to the transmit callback
  err = subscribe(RF_DRIVER, SUBSCRIBE_TX,
          tx_done_callback, (void *) NULL);
  if (err < 0) return err;

  // Issue the send command and wait for the transmission to be done.
  err = command(RF_DRIVER, COMMAND_SEND, PL_TYPE_NONE, 0);
  if (err < 0) return err;
  yield_for(&fired);
  free(packet->data);
  free(packet);
}

