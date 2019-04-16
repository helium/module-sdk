#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Adafruit_GPS/Adafruit_GPS.h"
#include "bmi160/bmi160_port.h"
#include "bosch_port/bosch_port.h"

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
static uint timer_fired = 0;

struct bmi160_dev bmi160;


int8_t bmi160_setup(struct bmi160_dev *sensor);
int8_t bmi160_get_all_with_time(struct bmi160_dev *sensor, struct bmi160_sensor_data *accel,
                                struct bmi160_sensor_data *gyro);

void gps_read_cb(void
  );
void gps_read_cb(void){
  gps_read_fired = true;
  event = true;
}

static void timer_callback( __attribute__ ((unused)) int arg0,
                            __attribute__ ((unused)) int arg1,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  timer_fired++;
  event = true;
}

// Timer callback
tock_timer_t simple_timer;

int main(void) {
  struct GPS gps;
  gps_t gps_reader;
  bool button_fired = false;

  printf("[Tracker Demo]\r\n");

  uint32_t device_id;
  rf_get_device_id(&device_id);
  uint8_t id = (uint8_t) device_id;
  printf("Device ID is 0x%x - ", (uint32_t) device_id);
  printf("using only bottom byte %u\r\n", id);

  printf("Battery value is %i mV\r\n", battery_read_mv());
  gps_init(&gps_reader, &gps_read_cb);
  GPS_init(&gps);
  // struct bmi160_sensor_data bmi_accel;
  // struct bmi160_sensor_data bmi_gyro;

  uint rslt;
  rslt = bmi160_port_init(&bmi160);
  if (rslt != BMI160_OK) {
    printf_async("BMI160 Initialization Failed\r\n");
  }else {
    printf_async("BMI160 Initialized\r\n");
  }
  bmi160_setup(&bmi160);

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

  uint16_t seq = 0;

  timer_every(1000, timer_callback, NULL, &simple_timer);

  while(1){
    yield_for(&event);
    event = false;

    while(gps_has_some(&gps_reader)){
      char * line = gps_pop(&gps_reader);
      if(line!= NULL){
        parseGPS(&gps, line);
        printf_async("%s",line);
        free(line);
      }
    }

    if(timer_fired == 5){

      timer_fired = 0;

      raw_packet_t pkt;
      uint8_t buffer[14];
      pkt.len = 14;
      pkt.data = &buffer;

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
      
      rf_send_raw(&pkt);

      gps.latitudeDegrees = 0;
      gps.longitudeDegrees = 0;

      printf_async("\t%02d:%02d:%02d\t", gps.hour, gps.minute, gps.seconds);
      for(uint i=0; i < 14; i++){
        printf_async("%02x ", buffer[i]);
      }
      printf_async("\r\n");
      seq++;

      led_on(0);
      
    }
    else if(timer_fired == 1){
      led_off(0);
    }
  };
 
  return 0;
}

int8_t bmi160_setup(struct bmi160_dev *sensor){

  int8_t rslt = BMI160_OK;

  /* Select the Output data rate, range of accelerometer sensor */
  sensor->accel_cfg.odr   = BMI160_ACCEL_ODR_1600HZ;
  sensor->accel_cfg.range = BMI160_ACCEL_RANGE_2G;
  sensor->accel_cfg.bw    = BMI160_ACCEL_BW_NORMAL_AVG4;

  /* Select the power mode of accelerometer sensor */
  sensor->accel_cfg.power = BMI160_ACCEL_NORMAL_MODE;

  /* Select the Output data rate, range of Gyroscope sensor */
  sensor->gyro_cfg.odr   = BMI160_GYRO_ODR_3200HZ;
  sensor->gyro_cfg.range = BMI160_GYRO_RANGE_2000_DPS;
  sensor->gyro_cfg.bw    = BMI160_GYRO_BW_NORMAL_MODE;

  /* Select the power mode of Gyroscope sensor */
  sensor->gyro_cfg.power = BMI160_GYRO_NORMAL_MODE;
  return bmi160_set_sens_conf(sensor);
}