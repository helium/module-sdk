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

static void gps_read_cb(char * newline){
  gps_read_fired = true;
}

int main(void) {
  struct GPS gps;
  printf("[GPS Demo]\r\n");
  gps_init(&gps_read_cb);
  GPS_init(&gps);


  while(1){
    yield_for(&gps_read_fired);
    printf_async("Received\r\n");
    gps_read_fired = false;
    // while(gps_has_some()){
    //   /printf_async("has some\r\n");
    //   char * line = gps_pop();
    //   //free(line);
    // }
  };
 
  return 0;
}
