#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Adafruit_GPS/Adafruit_GPS.h"
#include "bmi160/bmi160_port.h"
#include "bosch_port/bosch_port.h"

#include <battery.h>
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
static bool button_fired = false;
static bool in_motion = true;

typedef enum {
  NOT_CONFIGURED,
  MOTION,
  NO_MOTION
} bmi_interrupt_t;

static bmi_interrupt_t motion_int = NOT_CONFIGURED;

struct bmi160_dev bmi160;


int8_t bmi160_setup(struct bmi160_dev *sensor);
int8_t bmi160_get_all_with_time(struct bmi160_dev *sensor, struct bmi160_sensor_data *accel,
                                struct bmi160_sensor_data *gyro);

int8_t set_no_motion_int(struct bmi160_dev *sensor);
int8_t set_motion_int(struct bmi160_dev *sensor);

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


// Callback for button presses.
//   btn_num: The index of the button associated with the callback
//   val: 1 if pressed, 0 if depressed
static void button_callback(__attribute__ ((unused)) int btn_num,
                            __attribute__ ((unused)) int val,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  
  button_fired = true;
  // if (val == 1) {
  //   led_toggle(1);
  // }
  event = true;
}

// Timer callback
tock_timer_t simple_timer;

int main(void) {
  struct GPS gps;
  gps_t gps_reader;
  
  button_subscribe(button_callback, NULL);
  button_enable_interrupt(0);
  printf("[Tracker Demo]\r\n");

  uint32_t device_id;
  rf_get_device_id(&device_id);
  uint8_t id = (uint8_t) device_id;
  printf("Device ID is 0x%lx - ", device_id);
  printf("using only bottom byte %u\r\n", id);

  printf("Battery value is %i mV\r\n", battery_read_mv());
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

  int8_t rslt = bmi160_port_init(&bmi160);
  uint8_t data;
  uint16_t len = 1;
  rslt = bmi160_get_regs(BMI160_CHIP_ID_ADDR, &data, len, &bmi160);
  if (rslt != BMI160_OK) {
    printf_async("BMI160 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf_async("BMI160 Initialized. Address %u\r\n", data);
  }
  bmi160_setup(&bmi160);
  set_no_motion_int(&bmi160);

  uint16_t seq = 0;

  timer_every(200, timer_callback, NULL, &simple_timer);

  while(1){
    yield_for(&event);
    event = false;

    if(button_fired){
      printf_async("Interrupt or Button fired\r\n");
    }

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

      uint8_t buffer[14];

      raw_packet_t pkt = {
        .data = buffer,
        .len = 0
      };
      

      float lat = gps.latitudeDegrees;
      float lon = gps.longitudeDegrees;
      uint8_t speed = (int) gps.speed;
      int16_t elevation = (uint16_t)gps.altitude;

      memcpy(buffer, (void*)&id, sizeof(uint8_t));
      pkt.len += sizeof(uint8_t);
      memcpy(buffer + pkt.len, (void*)&seq, sizeof(uint16_t));
      pkt.len += sizeof(uint16_t);
      memcpy(buffer + pkt.len, (void*)&lat, sizeof(float));
      pkt.len += sizeof(float);
      memcpy(buffer + pkt.len, (void*)&lon, sizeof(float));
      pkt.len += sizeof(float);
      memcpy(buffer + pkt.len, &speed, sizeof(uint8_t));
      pkt.len += sizeof(uint8_t);
      memcpy(buffer + pkt.len, &elevation, sizeof(int16_t));

      //rf_send_raw(&pkt);

      printf_async("\t%02d:%02d:%02d\t", gps.hour, gps.minute, gps.seconds);
      for(uint i=0; i < pkt.len; i++){
        printf_async("%02x ", buffer[i]);
      }
      printf_async("\r\n");
      

      gps.latitudeDegrees = 0;
      gps.longitudeDegrees = 0;
      seq++;

      led_on(0);
      
    }
    else if(timer_fired == 1){
      led_off(0);
    }
  };
 
  return 0;
}

int8_t set_motion_int(struct bmi160_dev *sensor){
  struct bmi160_int_settg int_config;

  /* Select the Interrupt channel/pin */
  int_config.int_channel = BMI160_INT_CHANNEL_1;// Interrupt channel/pin 1

  /* Select the Interrupt type */
  int_config.int_type = BMI160_ACC_ANY_MOTION_INT;// Choosing Any motion interrupt
  /* Select the interrupt channel/pin settings */
  int_config.int_pin_settg.output_en = BMI160_ENABLE;// Enabling interrupt pins to act as output pin
  int_config.int_pin_settg.output_mode = BMI160_DISABLE;// Choosing push-pull mode for interrupt pin
  int_config.int_pin_settg.output_type = BMI160_ENABLE;// Choosing active low output
  int_config.int_pin_settg.edge_ctrl = BMI160_ENABLE;// Choosing edge triggered output
  int_config.int_pin_settg.input_en = BMI160_DISABLE;// Disabling interrupt pin to act as input
  int_config.int_pin_settg.latch_dur = BMI160_LATCH_DUR_NONE;// non-latched output

  /* Select the Any-motion interrupt parameters */
  int_config.int_type_cfg.acc_any_motion_int.anymotion_en = BMI160_ENABLE;// 1- Enable the any-motion, 0- disable any-motion 
  int_config.int_type_cfg.acc_any_motion_int.anymotion_x = BMI160_ENABLE;// Enabling x-axis for any motion interrupt
  int_config.int_type_cfg.acc_any_motion_int.anymotion_y = BMI160_ENABLE;// Enabling y-axis for any motion interrupt
  int_config.int_type_cfg.acc_any_motion_int.anymotion_z = BMI160_ENABLE;// Enabling z-axis for any motion interrupt
  int_config.int_type_cfg.acc_any_motion_int.anymotion_dur = 0;// any-motion duration
  int_config.int_type_cfg.acc_any_motion_int.anymotion_thr = 20;// (2-g range) -> (slope_thr) * 3.91 mg, (4-g range) -> (slope_thr) * 7.81 mg, (8-g range) ->(slope_thr) * 15.63 mg, (16-g range) -> (slope_thr) * 31.25 mg 

  /* Set the Any-motion interrupt */
  return bmi160_set_int_config(&int_config, sensor); /* sensor is an instance of the structure bmi160_dev  */
}

int8_t set_no_motion_int(struct bmi160_dev *sensor){
  struct bmi160_int_settg int_config;

  /* Select the Interrupt channel/pin */
  int_config.int_channel = BMI160_INT_CHANNEL_1;// Interrupt channel/pin 1

  /* Select the Interrupt type */
  int_config.int_type = BMI160_ACC_SLOW_NO_MOTION_INT;// Choosing Any motion interrupt
  /* Select the interrupt channel/pin settings */
  int_config.int_pin_settg.output_en = BMI160_ENABLE;// Enabling interrupt pins to act as output pin
  int_config.int_pin_settg.output_mode = BMI160_DISABLE;// Choosing push-pull mode for interrupt pin
  int_config.int_pin_settg.output_type = BMI160_ENABLE;// Choosing active low output
  int_config.int_pin_settg.edge_ctrl = BMI160_ENABLE;// Choosing edge triggered output
  int_config.int_pin_settg.input_en = BMI160_DISABLE;// Disabling interrupt pin to act as input
  int_config.int_pin_settg.latch_dur = BMI160_LATCH_DUR_NONE;// non-latched output

  /* Select the Any-motion interrupt parameters */
  int_config.int_type_cfg.acc_any_motion_int.anymotion_en = BMI160_ENABLE;// 1- Enable the any-motion, 0- disable any-motion 
  int_config.int_type_cfg.acc_any_motion_int.anymotion_x = BMI160_ENABLE;// Enabling x-axis for any motion interrupt
  int_config.int_type_cfg.acc_any_motion_int.anymotion_y = BMI160_ENABLE;// Enabling y-axis for any motion interrupt
  int_config.int_type_cfg.acc_any_motion_int.anymotion_z = BMI160_ENABLE;// Enabling z-axis for any motion interrupt
  int_config.int_type_cfg.acc_any_motion_int.anymotion_dur = 0;// any-motion duration
  int_config.int_type_cfg.acc_any_motion_int.anymotion_thr = 20;// (2-g range) -> (slope_thr) * 3.91 mg, (4-g range) -> (slope_thr) * 7.81 mg, (8-g range) ->(slope_thr) * 15.63 mg, (16-g range) -> (slope_thr) * 31.25 mg 

  /* Set the Any-motion interrupt */
  return bmi160_set_int_config(&int_config, sensor); /* sensor is an instance of the structure bmi160_dev  */
}

int8_t bmi160_setup(struct bmi160_dev *sensor){
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