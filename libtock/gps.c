#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gps.h"

typedef enum {
  READLINE = 1 
} cmd_t;

#define BUFFER_SIZE (512)
char buffer[BUFFER_SIZE];

typedef struct gps_line {
  char* buf;
  struct gps_line* next;
} gps_line_t;

// Request Queue

// head is first in (where we should pop from)
static gps_line_t* gps_line_head = NULL;
// tail is last in 
static gps_line_t* gps_line_tail = NULL;

static gps_cb user_cb = NULL;

static void internal_callback(int _x __attribute__ ((unused)),
                              int loc,
                              int _z __attribute__ ((unused)),
                              void* completed_request __attribute__ ((unused))) {

  printf_async("%s", &buffer[loc]);
  // gps_line_t * gps_line = calloc(1, sizeof(gps_line));
  // gps_line->next = NULL;
  // gps_line->buf = malloc(64);

  // strcpy(gps_line->buf, &buffer[loc]);
  // 
  // if(gps_line_head == NULL){
  //   gps_line_head = gps_line;
  // } else {
  //   gps_line_tail->next = gps_line;
  // }
  // gps_line_tail = gps_line;

  // if(user_cb!=NULL){
  //   (*user_cb)(&buffer[loc]);
  // }
}

int gps_init(gps_cb cb){
  user_cb = cb;
  //gps_line_head = NULL;
  //gps_line_tail = NULL;
  int err;
  //printf("We die if this isn't here???\r\n");
  err = allow(DRIVER_NUM_GPS, READLINE, (void*) buffer, BUFFER_SIZE);
  if (err < 0) return err;

  err = subscribe(DRIVER_NUM_GPS, READLINE, internal_callback, NULL);
  if (err < 0) return err;

  return TOCK_SUCCESS;
}

bool gps_has_some(void){
  return (gps_line_head != NULL);
}

char* gps_pop(void){
  // gps_line_t* cur = gps_line_head;
  // gps_line_head == NULL;
  // gps_line_head = cur->next;
  // char * ret = cur->buf;
  // free(cur);
  // return ret;
  
}
