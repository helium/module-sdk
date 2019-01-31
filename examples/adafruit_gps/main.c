#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Adafruit_GPS/Adafruit_GPS.h"
#include <button.h>
#include <led.h>
#include <led.h>
#include <packetizer.h>
#include <rf.h>
#include <timer.h>
#include <uart.h>

packet_sensor_t speed;
packet_sensor_t altitude;
packet_sensor_t lat;
packet_sensor_t lon;

#define GPS_SERIAL 1

bool new_event    = false;
bool gps_event    = false;
bool button_event = false;
char message[125];
unsigned char address[] = "somewhere";
uint8_t buffer [120];
struct GPS gps;

// Button Press callback
static void button_callback(int btn_num,
                            int val,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  if (val == 1) {
    led_on(btn_num);
    button_event = true;
    new_event    = true;
  }
}

// Timer callback
tock_timer_t simple_timer;

static void timer_callback( __attribute__ ((unused)) int arg0,
                            __attribute__ ((unused)) int arg1,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  gps_event = true;
  new_event = true;
}

char* create_payload(void);

int main(void) {
  printf("GPS Demo\r\n");

  packetizer_init(SER_MODE_JSON);
  packetizer_add_sensor(&lat, "lt", UINT);
  packetizer_add_sensor(&lon, "ln", INT);
  packetizer_add_sensor(&speed, "s", INT);
  packetizer_add_sensor(&altitude, "a", INT);

  button_subscribe(button_callback, NULL);

  // Enable interrupts on each button.
  int count = button_count();
  for (int i = 0; i < count; i++) {
    button_enable_interrupt(i);
  }

  // Enable GPS
  GPS_init(&gps, GPS_SERIAL);
  sendCommand(&gps, PMTK_SET_NMEA_OUTPUT_RMCGGA);
  sendCommand(&gps, PMTK_SET_NMEA_UPDATE_1HZ);

  delay_ms(1000);
  if (!rf_driver_check()) {
    printf("Driver check OK\r\n");
  } else {
    printf("Driver check FAIL\r\n");
  }

  if (!rf_set_oui(address)) {
    printf("Set address OK\r\n");
  } else {
    printf("Set address FAIL\r\n");
  }

  if (!rf_enable()) {
    printf("Radio init OK\r\n");
  } else {
    printf("Radio init FAIL\r\n");
  }

  timer_every(1, timer_callback, NULL, &simple_timer);

  while (1) {
    yield_for(&new_event);
    if (gps_event) {
      readGPS(&gps);
      if (gps.recvdflag) {
        char* ret = lastNMEA(&gps);
        printf("Last: %s\r\n", ret);
        parseGPS(&gps, ret);
        if (gps.fix) {
          led_on(1);
          led_on(0);
          printf("LAT: %f\r\n", gps.latitude);
          printf("LON: %f\r\n", gps.longitude);
          packet_add_data(&lat, &gps.latitude);
          packet_add_data(&lon, &gps.longitude);
          packet_add_data(&speed, &gps.speed);
          packet_add_data(&altitude, &gps.altitude);
          packet_t * packet = packet_assemble();
          int res = rf_send(packet);
          if (res != TOCK_SUCCESS) {
            printf("Send Fail\r\n");
          } else {
            printf("Send Success\r\n");
          }

        } else {
          printf("No GPS fix...\r\n");
        }
      }
      gps_event = false;
    }
    if (button_event) {
      printf("LAT: %f\r\n", gps.latitude);
      printf("LON: %f\r\n", gps.longitude);
      packet_add_data(&lat, &gps.latitude);
      packet_add_data(&lon, &gps.longitude);
      packet_add_data(&speed, &gps.speed);
      packet_add_data(&altitude, &gps.altitude);
      packet_t * packet = packet_assemble();
      int res = rf_send(packet);
      if (res != TOCK_SUCCESS) {
        printf("Send Fail\r\n");
      } else {
        printf("Send Success\r\n");
      }
      button_event = false;
    }
    new_event = false;
    led_off(0);
    led_off(1);
  }

  return 0;
}
