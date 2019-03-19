#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gps.h"

typedef enum {
  READLINE = 1 
} cmd_t;

#define BUFFER_SIZE (512)
char buffer[BUFFER_SIZE];

static void internal_callback(int _x __attribute__ ((unused)),
                              int loc,
                              int _z __attribute__ ((unused)),
                              void* gps_client __attribute__ ((unused))) {

  gps_t* gps = gps_client;
  
  gps_line_t * gps_line = calloc(1, sizeof(gps_line));
  gps_line->next = NULL;
  gps_line->buf = malloc(strlen(&buffer[loc]));

  strcpy(gps_line->buf, &buffer[loc]);
  
  if(gps->head == NULL){
    gps->head = gps_line;
  } else {
    gps->tail->next = gps_line;
  }
  gps->tail = gps_line;

  if(gps->user_cb!=NULL){
    (*gps->user_cb)();
  }
}

int gps_init(gps_t* gps, gps_cb cb){
  gps->user_cb = cb;
  int err;
  err = allow(DRIVER_NUM_GPS, READLINE, (void*) buffer, BUFFER_SIZE);
  if (err < 0) return err;

  err = subscribe(DRIVER_NUM_GPS, READLINE, internal_callback, gps);
  if (err < 0) return err;

  return TOCK_SUCCESS;
}

bool gps_has_some(gps_t* gps){
  return (gps->head != NULL);
}

char* gps_pop(gps_t* gps){
  gps_line_t* cur = gps->head;
  gps->head = cur->next;
  char * ret = cur->buf;
  free(cur);
  return ret;
}
