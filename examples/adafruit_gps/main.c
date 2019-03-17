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

static bool event = false;
static bool gps_read_fired = false;
static bool timer_fired = false;

void gps_read_cb(char * newline){
  gps_read_fired = true;
  event = true;
}

static void timer_callback( __attribute__ ((unused)) int arg0,
                            __attribute__ ((unused)) int arg1,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  timer_fired = true;
}

// Timer callback
tock_timer_t simple_timer;

int main(void) {
  struct GPS gps;
  gps_t gps_reader;

  printf("[GPS Demo]\r\n");
  gps_init(&gps_reader, &gps_read_cb);
  GPS_init(&gps);

  timer_every(1000, timer_callback, NULL, &simple_timer);

  while(1){
    yield_for(&event);
    event = false;


    while(gps_has_some(&gps_reader)){
      char * line = gps_pop(&gps_reader);
      parseGPS(&gps, line);
      free(line);
    }
    delay_ms(10);


    if(timer_fired){
      timer_fired = false;
      printf_async("%02d:%02d:%02d: lat: %d lon: %d\r\n", 
        gps.hour, gps.minute, gps.seconds, gps.latitudeDegrees, gps.longitudeDegrees);
    }


  };
 
  return 0;
}
