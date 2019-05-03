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

bool gps_has_some(void);
char* gps_pop(void);

int gps_wake(void);
int gps_sleep(void);


#ifdef __cplusplus
}
#endif