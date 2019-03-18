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
  event = true;
}

// Timer callback
tock_timer_t simple_timer;

int main(void) {
  struct GPS gps;
  gps_t gps_reader;

  printf("[GPS Demo]\r\n");
  gps_init(&gps_reader, &gps_read_cb);
  GPS_init(&gps);


  if (!rf_driver_check()) {
    printf("Driver check OK\r\n");
  } else {
    printf("Driver check FAIL\r\n");
  }

  if (!rf_enable()) {
    printf("Radio init OK\r\n");
  } else {
    printf("Radio init FAIL\r\n");
  }

  uint8_t id = 5;
  uint16_t seq = 0;

  timer_every(1000, timer_callback, NULL, &simple_timer);

  while(1){
    yield_for(&event);
    event = false;

    while(gps_has_some(&gps_reader)){
      char * line = gps_pop(&gps_reader);
      printf_async(" %s\r", line); 

      parseGPS(&gps, line);
      free(line);
    }
    delay_ms(10);


    if(timer_fired){
      timer_fired = false;

      if(gps.fix) {
        raw_packet_t pkt;
        uint8_t buffer[128];

        float lat = gps.latitudeDegrees;
        float lon = gps.longitudeDegrees;
        uint8_t speed = (int) gps.speed;
        int16_t elevation = (uint16_t)gps.altitude;
        uint byte_counter = 0;
        memcpy(buffer, (void*)&id, sizeof(uint8_t));
        byte_counter += sizeof(uint8_t);
        memcpy(buffer + byte_counter, (void*)&seq, sizeof(uint16_t));
        byte_counter += sizeof(uint16_t);
        memcpy(buffer + byte_counter, (void*)&lat, sizeof(float));
        byte_counter += sizeof(float);
        memcpy(buffer + byte_counter, (void*)&lon, sizeof(float));
        byte_counter += sizeof(float);
        memcpy(buffer + byte_counter, &speed, sizeof(uint8_t));
        byte_counter += sizeof(uint8_t);
        memcpy(buffer + byte_counter, &elevation, sizeof(int16_t));
        //byte_counter += sizeof(int16_t);
        //raw_packet_t * pkt = calloc(1, sizeof(raw_packet_t));

        pkt.data = buffer;
        pkt.len = 14;
        rf_send_raw(&pkt);
        seq++;
        printf_async("%02d:%02d:%02d: lat: %u lon: %u\r\n", 
         gps.hour, gps.minute, gps.seconds, (uint) gps.latitudeDegrees, (uint) gps.longitudeDegrees);
      } else {
        printf_async("%02d:%02d:%02d: no fix\r\n", gps.hour, gps.minute, gps.seconds);
      }
      
    }


  };
 
  return 0;
}
