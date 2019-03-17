#pragma once

#include "tock.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_NUM_GPS 0x80005

// data provides pointer to read buffer
typedef void (*gps_cb)(char * data);

int gps_init(gps_cb cb);

bool gps_has_some(void);

char* gps_pop(void);

#ifdef __cplusplus
}
#endif