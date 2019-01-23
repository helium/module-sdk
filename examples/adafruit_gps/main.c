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

packet_sensor_t pressure;
packet_sensor_t temperature;
packet_sensor_t lat;
packet_sensor_t lon;

#define GPS_SERIAL 1

bool new_event = true;
char message[125];
unsigned char address[] = "somewhere";
struct GPS gps;

// Button Press callback
static void button_callback(int btn_num,
                            int val,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  if (val == 1) {
    led_on(btn_num);
    new_event = true;
  }
}

// Timer callback
tock_timer_t simple_timer;

static void timer_callback( __attribute__ ((unused)) int arg0,
                            __attribute__ ((unused)) int arg1,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  led_on(1);
  delay_ms(1000);
  led_off(1);
  new_event = true;
}

char* create_payload(void);

int main(void) {
  printf("Simple RF Demo\r\n");

  packetizer_init(SER_MODE_JSON);
  packetizer_add_sensor(&pressure, "pressure", UINT);
  packetizer_add_sensor(&temperature, "temperature", INT);

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

  timer_every(30000, timer_callback, NULL, &simple_timer);

  while (1) {
    yield_for(&new_event);

    if (waitForSentence(&gps, "$GPRMC")) {
      packet_add_data(&lat, &gps.latitude);
      packet_add_data(&lon, &gps.longitude);

    }

    uint32_t pressure_data   = 1234567890;
    int32_t temperature_data = 1234567890;
    packet_add_data(&pressure, &pressure_data);
    packet_add_data(&temperature, &temperature_data);

    packet_t * packet = packet_assemble();
    printf("sending\r\n");
    int res = rf_send(packet);
    if (res != TOCK_SUCCESS) {
      printf("\r\nSend Fail\r\n");
    } else {
      printf("\r\nSend SUCCESS\r\n");
    }
    new_event = false;
    led_off(0);
    led_off(1);
  }

  return 0;
}
