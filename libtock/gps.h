#pragma once

#include "tock.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_NUM_GPS 0x80005

// data provides pointer to read buffer
typedef void (*gps_cb)(void);


typedef struct gps_line {
  char* buf;
  struct gps_line* next;
} gps_line_t;

typedef struct {
	gps_cb user_cb;
} gps_t;

int gps_init(gps_t* gps, gps_cb cb);

bool gps_has_some(gps_t* gps);

char* gps_pop(gps_t* gps);

#ifdef __cplusplus
}
#endif