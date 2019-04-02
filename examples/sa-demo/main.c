#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Adafruit_GPS/Adafruit_GPS.h"
#include "bosch_port/bosch_port.h"
#include "opt3001/opt3001.h"
#include "bme280/bme280_port.h"

#include <i2c_master.h>
#include <led.h>
#include <led.h>
#include <rf.h>
#include <timer.h>
#include <uart.h>

static bool event = false;
static bool gps_read_fired = false;
static bool timer_fired = false;

struct opt3001_dev opt3001;
struct bme280_dev bme280;
struct bme280_data bme_data;

int8_t bme280_setup(struct bme280_dev *dev);
int8_t bme280_get_sample_forced_mode(struct bme280_dev *dev, struct bme280_data *data);
int8_t opt3001_get_lux(struct opt3001_dev *dev);

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
  timer_fired = true;
  event = true;
}

// Timer callback
tock_timer_t simple_timer;

int main(void) {
  printf("[SA Demo]\r\n");

  if (!OPT3001_init(&opt3001, NULL)) {
     printf_async("OPT3001 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf_async("OPT3001 Initialized\r\n");
  }

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

  uint8_t id = 0xFF;
  uint16_t seq = 0;

  timer_every(1000, timer_callback, NULL, &simple_timer);

  while(1){
    yield_for(&event);
    event = false;

    if(timer_fired){
      int result;
      
      // Read the Atmospheric sensor
      result = bme280_get_sample_forced_mode(&bme280, &bme_data);
      if (result) {
        printf_async("BME280 Fail");
      }else {
        printf_async("BME280 OK");
      }

      // Read the optical sensor
      float lux_data;
      result = !OPT3001_getLux(&opt3001, &lux_data);
      if (result) {
        printf_async("OPT3001 Lux fail");
      }else {
        printf_async("LUX data OK");
      }

      timer_fired = false;
      raw_packet_t pkt;
      uint8_t buffer[14];
      pkt.len = 14;
      pkt.data = &buffer;

      uint32_t tmp = &bme_data.temperature;
      uint32_t pres = &bme_data.pressure;
      uint8_t lux = (int) lux_data;
      int16_t hum = (uint16_t)&bme_data.humidity;
      uint byte_counter = 0;
      memcpy(buffer, (void*)&id, sizeof(uint8_t));
      byte_counter += sizeof(uint8_t);
      memcpy(buffer + byte_counter, (void*)&seq, sizeof(uint16_t));
      byte_counter += sizeof(uint16_t);
      memcpy(buffer + byte_counter, (void*)&tmp, sizeof(float));
      byte_counter += sizeof(float);
      memcpy(buffer + byte_counter, (void*)&pres, sizeof(float));
      byte_counter += sizeof(float);
      memcpy(buffer + byte_counter, &lux_data, sizeof(uint8_t)); 
      byte_counter += sizeof(uint8_t);
      memcpy(buffer + byte_counter, &hum, sizeof(int16_t));
      rf_send_raw(&pkt);
      printf_async("\r\n");
      seq++;
     
    }
  };
 
  return 0;
}

int8_t bme280_setup(struct bme280_dev *dev)
{
  uint8_t settings_sel;

  /* Recommended mode of operation: Indoor navigation */
  dev->settings.osr_h  = BME280_OVERSAMPLING_1X;
  dev->settings.osr_p  = BME280_OVERSAMPLING_16X;
  dev->settings.osr_t  = BME280_OVERSAMPLING_2X;
  dev->settings.filter = BME280_FILTER_COEFF_16;

  settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

  return bme280_set_sensor_settings(settings_sel, dev);
}

int8_t bme280_get_sample_forced_mode(struct bme280_dev *dev, struct bme280_data *comp_data)
{
  // struct bme280_data comp_data;
  uint8_t rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);

  if (rslt != BME280_OK) {
    printf("Fail on set sensor mode\r\n");
    return rslt;
  }

  /* Wait for the measurement to complete and print data @25Hz */
  dev->delay_ms(40);

  rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);
  if (rslt != BME280_OK) {
    return rslt;
  }
  return rslt;
}

