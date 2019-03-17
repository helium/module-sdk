#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "uart.h"

typedef enum {
  WRITE = 1,
  READ = 2
} cmd_t;

typedef struct write_request {
  cmd_t type;
  uart_cb user_cb;
  uint8_t uart_num;
  uint8_t* buf;
  int len;
  struct write_request* next;
} request_t;



static int uart_dispatch_request(request_t* req);

// Request Queue
static request_t *uart_queue = NULL;
bool queue_empty = false;

static void internal_callback(int _x __attribute__ ((unused)),
                              int uart_num __attribute__ ((unused)),
                              int _z __attribute__ ((unused)),
                              void* completed_request) {

  request_t* cur_req = (request_t*) completed_request;

  // preserve a local pointer to next item
  request_t* next = cur_req->next;

  // fire callback
  if (cur_req->user_cb != NULL) {
    (*cur_req->user_cb)(cur_req->uart_num, cur_req->buf, cur_req->len);
  }

  // free old request after firing callback
  if (cur_req->type == WRITE) {
    free(cur_req->buf);
  }
  free(cur_req);

  if (next != NULL) {
    // dispatch next request
    uart_dispatch_request(next);
  } else {
    // eat the tail of the queue
    uart_queue  = NULL;
    queue_empty = true;
  }
}

// dummy async callback for synchronous calls
static bool dummy_fired;
static uint32_t read_len = 0;
static void dummy_cb(uint8_t uart_num __attribute__ ((unused)), uint8_t * data __attribute__ (
                       (unused)), uint8_t len __attribute__ (
                       (unused))){
  dummy_fired = true;
  read_len = len;
}

int uart_writestr(uint8_t uart_num, const char* buf){
  int err;
  err = uart_write(uart_num, (uint8_t*) buf, strlen(buf));
  return err;
}

int uart_writestr_async(uint8_t uart_num, const char* buf, uart_cb cb){
  int err;
  err = uart_write_async(uart_num, (uint8_t*) buf, strlen(buf), cb);
  return err;
}

int uart_write(uint8_t uart_num, uint8_t* buf, size_t len){
  int err;

  dummy_fired = false;
  err         = uart_write_async(uart_num, buf, len, &dummy_cb);
  if (err < TOCK_SUCCESS) return err;

  // wait for callback
  yield_for(&dummy_fired);

  return TOCK_SUCCESS;
}

static int uart_dispatch_request(request_t* req){
  int err;

  err = allow(DRIVER_NUM_UART, req->type | (req->uart_num << 16), (void*) req->buf, req->len);
  if (err < 0) return err;

  err = subscribe(DRIVER_NUM_UART, req->type | (req->uart_num << 16), internal_callback, req);
  if (err < 0) return err;

  err = command(DRIVER_NUM_UART, req->type | (req->uart_num << 16), req->len, 0);
  return err;
}

request_t* allocate_and_initialize_request(cmd_t type, uint8_t uart_num, uint8_t* buf, size_t len,
                                           uart_cb cb, bool no_copy);
request_t* allocate_and_initialize_request(cmd_t type, uint8_t uart_num, uint8_t* buf, size_t len,
                                           uart_cb cb, bool no_copy){
  request_t* request = (request_t*) calloc(1,sizeof(request_t));

  if (request == NULL) {
    if (!queue_empty) {
      yield_for(&queue_empty);
      request = (request_t*) calloc(1,sizeof(request_t));
    }else {
      return NULL;
    }
    if (request == NULL) return NULL;
  }

  request->type = type;
  if (type == READ || no_copy) {
    request->buf = buf;
  } else {
    uint8_t* out_buf = (uint8_t*) calloc(1, len);
    if (out_buf == NULL) {
      if (!queue_empty) {
        yield_for(&queue_empty);
        out_buf = (uint8_t*) calloc(1,len);
      }else {
        return NULL;
      }
      if (out_buf == NULL) return NULL;
    }
    memcpy(out_buf, buf, len);
    request->buf = out_buf;
  }

  request->user_cb  = cb;
  request->uart_num = uart_num;
  request->len      = len;
  request->next     = NULL;
  return request;
}

int uart_place_request(request_t* request);
int uart_place_request(request_t* request){
  // there is a queue
  if (uart_queue != NULL) {
    queue_empty = false;
    // insert self in queue
    uart_queue->next = request;
    uart_queue       = request;
    return TOCK_SUCCESS;
  }
  // otherwise, no queue so just execute now
  else{
    uart_queue = request;
    return uart_dispatch_request(request);
  }
}

int uart_write_async(uint8_t uart_num, uint8_t* buf, size_t len, uart_cb cb){
  request_t* request = allocate_and_initialize_request(WRITE, uart_num, buf, len, cb, false);
  if (request == NULL) {
    return TOCK_ENOMEM;
  }
  uart_place_request(request);
  return TOCK_SUCCESS;
}

int uart_write_async_unsafe(uint8_t uart_num, uint8_t* buf, size_t len, uart_cb cb){
  request_t* request = allocate_and_initialize_request(WRITE, uart_num, buf, len, cb, true);
  if (request == NULL) {
    return TOCK_ENOMEM;
  }
  uart_place_request(request);
  return TOCK_SUCCESS;
}

int uart_read(uint8_t uart_num, uint8_t* buf, size_t len){
  dummy_fired = false;
  if(TOCK_SUCCESS == uart_read_async(uart_num, buf, len, &dummy_cb)){
    // wait for callback
    yield_for(&dummy_fired);
    return read_len;
  }
  else{
    return 0;
  }
  
}
int uart_read_async(uint8_t uart_num, uint8_t* buf, size_t len, uart_cb cb){
  request_t* request = allocate_and_initialize_request(READ, uart_num, buf, len, cb, false);
  if (request == NULL) {
    return TOCK_ENOMEM;
  }
  uart_place_request(request);
  return TOCK_SUCCESS;
}

/* Returns TOCK_FAIL on failure, or else the character received */
int uart_read_byte(uint8_t uart_num, uint8_t * data){
  (void) uart_num;
  (void) data;
  return 0;
}

// Abort an ongoing receive call.
int uart_read_abort(uint8_t uart_num){
  (void) uart_num;
  return 0;
}

#include <stdarg.h>
#include <stdio.h>

#define INITIAL_BUFFER 128

char * allocate_buffer(uint size);
char * allocate_buffer(uint size){
  char  * buffer = malloc(size);
  if (buffer == NULL) {
    if (!queue_empty) {
      yield_for(&queue_empty);
      buffer =  malloc(size);
    } else {
      return NULL;
    }
    if (buffer == NULL) return NULL;
  }
  return buffer;
}

int _write_strf_async(uint8_t uart_num,  uart_cb cb, const char *fmt,
                      va_list args) __attribute__((format(printf, 3, 0)));
int _write_strf_async(uint8_t uart_num,  uart_cb cb, const char *fmt, va_list args){
  int numbytes;
  char  * buffer = allocate_buffer(INITIAL_BUFFER);
  if (buffer == NULL) return 0;

  // sprintf returns how many bytes it actually needed
  numbytes = vsnprintf(buffer, INITIAL_BUFFER, fmt, args);

  // if it was bigger than our initial size, allocate bigger
  if (numbytes > INITIAL_BUFFER) {
    free(buffer);
    buffer   = allocate_buffer(numbytes);
    numbytes = vsnprintf(buffer, numbytes, fmt, args);
    uart_write_async_unsafe(uart_num, (uint8_t*) buffer, numbytes, cb);
  } else {
    uart_write_async_unsafe(uart_num, (uint8_t*) buffer, numbytes, cb);
  }

  return numbytes;
}

int uart_writestrf(uint8_t uart_num, const char *fmt, ...) {
  int err;
  va_list args;
  va_start(args, fmt);

  dummy_fired = false;
  err         = _write_strf_async(uart_num, &dummy_cb, fmt, args);
  if (err < TOCK_SUCCESS) return err;

  // wait for callback
  yield_for(&dummy_fired);

  return TOCK_SUCCESS;
}


int uart_writestrf_async(uint8_t uart_num,  uart_cb cb, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  return _write_strf_async(uart_num, cb, fmt, args);
}

int printf_async(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  return _write_strf_async(0, NULL, fmt, args);
}
