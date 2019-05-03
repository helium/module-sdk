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

struct bmi160_dev bmi160;


int8_t bmi160_setup(struct bmi160_dev *sensor);
int8_t bmi160_get_all_with_time(struct bmi160_dev *sensor, struct bmi160_sensor_data *accel,
                                struct bmi160_sensor_data *gyro);

int8_t set_no_motion_int(struct bmi160_dev *sensor);
int8_t set_motion_int(struct bmi160_dev *sensor);
int8_t disable_motion_int(struct bmi160_dev *sensor);
int8_t disable_no_motion_int(struct bmi160_dev *sensor);

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
  event = true;
}

// Timer callback
tock_timer_t simple_timer;

#define SLEEP_LED 0
#define SEND_LED  1
#define SLEEP_THRESHOLD 60

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
  set_motion_int(&bmi160);
  //set_no_motion_int(&bmi160);

  uint16_t seq = 0;

  int16_t no_motion_counter = 0;
  timer_every(200, timer_callback, NULL, &simple_timer);

  while(1){
    yield_for(&event);
    event = false;

    if(button_fired){
      button_fired = false;

      union bmi160_int_status interrupt;
      enum bmi160_int_status_sel int_status_sel;

      /* Interrupt status selection to read all interrupts */
      int_status_sel = BMI160_INT_STATUS_ALL;
      rslt = bmi160_get_int_status(int_status_sel, &interrupt, &bmi160);
      no_motion_counter = 0;
      gps_wake();
      led_on(SLEEP_LED);
    }

    while(gps_has_some()){
      char * line = gps_pop();
      if(line!= NULL){
        parseGPS(&gps, line);
        printf_async("%s",line);
        free(line);
      }
    }

    if(timer_fired >= 5){
      if(no_motion_counter <= SLEEP_THRESHOLD){
        no_motion_counter++;

        timer_fired = 0;

        raw_packet_t pkt;
        uint8_t buffer[14];
        pkt.len = 14;
        pkt.data = buffer;

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

        // printf_async("\t%02d:%02d:%02d\t", gps.hour, gps.minute, gps.seconds);
        // for(uint i=0; i < 14; i++){
        //   printf_async("%02x ", buffer[i]);
        // }
        // printf_async("\r\n");

/*       
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


        // printf_async("\r\n\t%02d:%02d:%02d\t", gps.hour, gps.minute, gps.seconds);
        // for(uint i=0; i < pkt.len; i++){
        //   printf_async("%02x ", buffer[i]);
        // }
        // printf_async("\r\n");

        rf_send_raw(&pkt);
*/

        
        gps.latitudeDegrees = 0;
        gps.longitudeDegrees = 0;
        seq++;
        led_on(SEND_LED);

      }
      if (no_motion_counter == SLEEP_THRESHOLD){
        printf("no move!\r\n");
        led_off(SLEEP_LED);
        led_off(SEND_LED);

        gps_sleep();
      }
    }
    else if(timer_fired == 1){
        led_off(SEND_LED);
    }
  };
 
  return 0;
}

int8_t set_motion_int(struct bmi160_dev *sensor){
  struct bmi160_int_settg int_config;

  /* Select the Interrupt channel/pin */
  int_config.int_channel = BMI160_INT_CHANNEL_1;// Interrupt channel/pin 1

  /* Select the Interrupt type */
  int_config.int_type = BMI160_ACC_SIG_MOTION_INT;// Choosing Any motion interrupt
  /* Select the interrupt channel/pin settings */
  int_config.int_pin_settg.output_en = BMI160_ENABLE;// Enabling interrupt pins to act as output pin
  int_config.int_pin_settg.output_mode = BMI160_DISABLE;// Choosing push-pull mode for interrupt pin
  int_config.int_pin_settg.output_type = BMI160_ENABLE;// Choosing active low output
  int_config.int_pin_settg.edge_ctrl = BMI160_ENABLE;// Choosing edge triggered output
  int_config.int_pin_settg.input_en = BMI160_DISABLE;// Disabling interrupt pin to act as input
  int_config.int_pin_settg.latch_dur = BMI160_LATCH_DUR_160_MILLI_SEC;//BMI160_LATCHED;// non-latched output

  /* Select the Any-motion interrupt parameters */
  int_config.int_type_cfg.acc_sig_motion_int.sig_en = 1;
  int_config.int_type_cfg.acc_sig_motion_int.sig_mot_skip = 0x0;
  int_config.int_type_cfg.acc_sig_motion_int.sig_mot_proof = 0x01;
  int_config.int_type_cfg.acc_sig_motion_int.sig_data_src = 0;
  int_config.int_type_cfg.acc_sig_motion_int.sig_mot_thres = 10;


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