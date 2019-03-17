#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Adafruit_GPS/Adafruit_GPS.h"
#include <button.h>
#include <gps.h>
#include <led.h>
#include <led.h>
#include <packetizer.h>
#include <rf.h>
#include <timer.h>
#include <uart.h>

static bool gps_read_fired = false;

void gps_read_cb(char * newline){
  gps_read_fired = true;
}

int main(void) {
  struct GPS gps;
  gps_t gps_reader;

  printf("[GPS Demo]\r\n");
  gps_init(&gps_reader, &gps_read_cb);
  GPS_init(&gps);

  while(1){
    yield_for(&gps_read_fired);
    gps_read_fired = false;
    while(gps_has_some(&gps_reader)){
      char * line = gps_pop(&gps_reader);
      printf("%s", line);
      parseGPS(&gps, line);

      free(line);
    }

    printf_async("%02d:%02d:%02d\r\n", gps.hour, gps.minute, gps.seconds);
    printf_async("lat: %d lon: %d\r\n", gps.latitude, gps.longitude);
  };
 
  return 0;
}
