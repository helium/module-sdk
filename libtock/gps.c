#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gps.h"

typedef enum {
  READLINE = 1 
} cmd_t;

#define BUFFER_SIZE (512)
char buffer[BUFFER_SIZE];

#define NUM_GPS_LINES (24)
gps_line_t gps_line_buffer[NUM_GPS_LINES];

static uint cur_write_line = 0;
static uint cur_read_line = 0;

static void internal_callback(int _x __attribute__ ((unused)),
                              int loc,
                              int _z __attribute__ ((unused)),
                              void* gps_client __attribute__ ((unused))) {

  gps_t* gps = gps_client;
  gps_line_t * gps_line = &gps_line_buffer[cur_write_line++];
  if (cur_write_line == NUM_GPS_LINES){
    cur_write_line = 0;
  }
  gps_line->next = NULL;
  gps_line->buf = malloc(strlen(&buffer[loc]));

  if (gps_line->buf != NULL) {
    strcpy(gps_line->buf, &buffer[loc]);
  }
  
  
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
  return (
    cur_read_line != cur_write_line 
  );
}

char* gps_pop(gps_t* gps){
  gps_line_t* cur = &gps_line_buffer[cur_read_line++];
  if (cur_read_line == NUM_GPS_LINES){
    cur_read_line = 0;
  }
  char * ret = cur->buf;
  return ret;
}
