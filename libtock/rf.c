#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "packetizer.h"
#include "rf.h"
#include "tock.h"

#define RF_DRIVER           (0xCC1352)
#define ALLOW_NUM_W             (1)
#define ALLOW_NUM_C             (2)
#define SUBSCRIBE_TX            (1)

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

typedef struct rf_req {
  packet_t* packet;
  rf_cb user_cb;
  struct rf_req* next;
} rf_req_t;

// Request Queue
static rf_req_t *rf_queue = NULL;
static bool queue_empty   = false;
int rf_dispatch_request(rf_req_t* req);
static void tx_done_callback(int result,
                             __attribute__ ((unused)) int arg2,
                             __attribute__ ((unused)) int arg3,
                             void* completed_request) {
  rf_req_t* cur_req = (rf_req_t*) completed_request;

  // keep local copy of callback and next item
  rf_cb cb       = cur_req->user_cb;
  rf_req_t* next = cur_req->next;

  // fire callback

  if (cb != NULL) {
    (*cb)(result, cur_req->packet);
  }
  // free current request
  packet_disassemble(cur_req->packet);
  free(cur_req);

  // dispatch next request
  if (next != NULL) {
    rf_dispatch_request(next);
  } else {
    // eat the tail of the queue
    rf_queue    = NULL;
    queue_empty = true;
  }
}

int rf_driver_check(void) {
  return command(RF_DRIVER, COMMAND_DRIVER_CHECK, 0, 0);
}

int rf_enable(void) {
  return command(RF_DRIVER, COMMAND_ENABLE, 0, 0);
}

int rf_set_oui(unsigned char *address) {
  if (!address) return TOCK_EINVAL;
  int err = allow(RF_DRIVER, ALLOW_NUM_C, (void *) address, 10);
  if (err < 0) return err;
  return command(RF_DRIVER, COMMAND_SET_ADDRESS, 0, 0);
}

int rf_dispatch_request(rf_req_t* req){
  int err;

  err = allow(RF_DRIVER, ALLOW_NUM_W, (void *) req->packet->data, req->packet->len);
  if (err < 0) return err;

  // Subscribe to the transmit callback
  err = subscribe(RF_DRIVER, SUBSCRIBE_TX,
                  tx_done_callback, (void *) req);
  if (err < 0) return err;

  // Issue the send command and wait for the transmission to be done.
  return command(RF_DRIVER, COMMAND_SEND, 0, 0);
}

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

  request->packet  = packet;
  request->user_cb = cb;
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

// Request Queue
static int dummy_result;
bool fired;
static void dummy_cb(int result, packet_t* packet __attribute__ ((unused))){
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


