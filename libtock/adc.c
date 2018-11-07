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

int adc_set_callback(subscribe_cb callback, void* callback_args);
int adc_set_callback(subscribe_cb callback, void* callback_args) {
  return subscribe(DRIVER_NUM_ADC, BUFFER_RECEIVE_CB, callback, callback_args);
}

int adc_single_sample(uint8_t channel);
int adc_single_sample(uint8_t channel) {
  return command(DRIVER_NUM_ADC, SINGLE_SAMPLE, channel, 0);
}

typedef struct adc_req {
  int error;
  uint8_t channel;
  adc_read_cb user_cb;
  struct adc_req* next;
} adc_req_t;

static adc_req_t *adc_req_tail = NULL;

// Internal callback for creating synchronous functions
static void adc_cb(int _x __attribute__ ((unused)),
                   int channel,
                   int sample,
                   void* callback_args) {

  adc_req_t* cur_req = (adc_req_t*) callback_args;
  
  if(cur_req->user_cb == NULL){
    // there is a bug in adc.c if this happened
  }

  adc_read_cb cb = cur_req->user_cb;
  adc_req_t* next = cur_req->next;
  free(cur_req);
  (*cb)(channel, sample, next == NULL);

  if(next != NULL){
    next->error = adc_set_callback(adc_cb, (void*) next);
    if (next->error < TOCK_SUCCESS) return;

    next->error = adc_single_sample(next->channel);
    if (next->error < TOCK_SUCCESS) return;
  }
  else{
    adc_req_tail = NULL;
  }
}

static uint16_t * dummy_value;
static bool dummy_fired;
void dummy_cb(int chan __attribute__ ((unused)), uint16_t sample, bool queue_empty __attribute__((unused)));
void dummy_cb(int chan __attribute__ ((unused)), uint16_t sample, bool queue_empty __attribute__((unused))){
  *dummy_value = sample;
  dummy_fired = true;
}

int adc_read(uint8_t channel, uint16_t* sample) {
  dummy_value = sample;
  dummy_fired = false;

  adc_read_async(channel, &dummy_cb);

  // wait for callback
  yield_for(&dummy_fired);

  return TOCK_SUCCESS;
}

int adc_read_async(uint8_t channel, adc_read_cb cb){
  int err;
  adc_req_t* request = (adc_req_t*)malloc(sizeof(adc_req_t));
  if (request == NULL) return TOCK_ENOMEM;

  request->user_cb = cb;

// there is a queue
  if(adc_req_tail != NULL){
    // insert self in queue
    request->channel = channel;
    adc_req_tail->next = request;
    adc_req_tail = request;
  }
  // otherwise, just execute now
  else{
    adc_req_tail = request;
    request->next = NULL;
    err = adc_set_callback(adc_cb, (void*) request);
    if (err < TOCK_SUCCESS) return err;

    err = adc_single_sample(channel);
    if (err < TOCK_SUCCESS) return err;
  }

  return TOCK_SUCCESS;
}
