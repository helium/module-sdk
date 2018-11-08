// ADC interface
#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>


#include "adc.h"
#include "tock.h"

enum cb {
  BUFFER_RECEIVE_CB = 0,
};

enum cmd {
  SINGLE_SAMPLE = 1,
};

typedef struct adc_req {
  int error;
  uint8_t channel;
  adc_read_cb user_cb;
  struct adc_req* next;
} adc_req_t;

// callback definition given to Tock
static void adc_cb(int _x __attribute__ ((unused)), int channel, int sample, void* completed_request);
// internal helpers
static int adc_set_callback(subscribe_cb callback, adc_req_t* req);
static int adc_single_sample(uint8_t channel);
static void dummy_cb(int chan __attribute__ ((unused)), uint16_t sample, bool queue_empty __attribute__((unused)));
static int adc_dispatch_request(adc_req_t* req);

// dummy async callback for synchronous calls
static uint16_t * dummy_value;
static bool dummy_fired;
static void dummy_cb(int chan __attribute__ ((unused)), uint16_t sample, bool queue_empty __attribute__((unused))){
  *dummy_value = sample;
  dummy_fired  = true;
}

// Synchronous read simply exercises async call
int adc_read(uint8_t channel, uint16_t* sample) {
  dummy_value = sample;
  dummy_fired = false;

  adc_read_async(channel, &dummy_cb);

  // wait for callback
  yield_for(&dummy_fired);

  return TOCK_SUCCESS;
}


// Request Queue
static adc_req_t *adc_req_queue = NULL;
// Asynchronous Call
int adc_read_async(uint8_t channel, adc_read_cb cb){
  adc_req_t* request = (adc_req_t*)malloc(sizeof(adc_req_t));
  if (request == NULL) return TOCK_ENOMEM;

  request->user_cb = cb;
  request->next    = NULL;
  request->channel = channel;
  // there is a queue
  if (adc_req_queue != NULL) {
    // insert self in queue
    adc_req_queue->next = request;
    adc_req_queue       = request;
    return TOCK_SUCCESS;
  }
  // otherwise, no queue so just execute now
  else{
    adc_req_queue = request;
    return adc_dispatch_request(request);
  }

}

static int adc_set_callback(subscribe_cb callback, adc_req_t* req) {
  return subscribe(DRIVER_NUM_ADC, BUFFER_RECEIVE_CB, callback, (void*) req);
}

static int adc_single_sample(uint8_t channel) {
  return command(DRIVER_NUM_ADC, SINGLE_SAMPLE, channel, 0);
}

static int adc_dispatch_request(adc_req_t* req){
  int err;

  err = adc_set_callback(adc_cb, req);
  if (err < TOCK_SUCCESS) return err;

  err = adc_single_sample(req->channel);
  if (err < TOCK_SUCCESS) return err;

  return TOCK_SUCCESS;
}

// Internal callback
// Handles internal queue, dispatching new requests
// And fires user callback
static void adc_cb(int _x __attribute__ ((unused)),
                   int channel,
                   int sample,
                   void* completed_request) {

  adc_req_t* cur_req = (adc_req_t*) completed_request;

  // keep local copy of callback and next item
  adc_read_cb cb  = cur_req->user_cb;
  adc_req_t* next = cur_req->next;
  // free current request
  free(cur_req);

  // fire callback (WARNING: we don't check for null pointer because
  // it's all internal to this library but this is generally unsafe)
  (*cb)(channel, sample, next == NULL);

  // dispatch next request
  if (next != NULL) {
    adc_dispatch_request(next);
  } else {
    adc_req_queue = NULL;
  }
}


