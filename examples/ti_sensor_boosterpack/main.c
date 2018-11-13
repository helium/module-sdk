/* vim: set sw=2 expandtab tw=80: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bme280/bme280_port.h"
#include "bmi160/bmi160_port.h"
#include "bmm150/bmm150_port.h"
#include "opt3001/opt3001.h"

#include <button.h>
#include <i2c_master.h>
#include <led.h>
#include <led.h>
#include <packetizer.h>
#include <rf.h>
#include <timer.h>
#include <uart.h>

int8_t bme280_setup(struct bme280_dev *dev);
int8_t bme280_get_sample_forced_mode(struct bme280_dev *dev, struct bme280_data *data);

int8_t bmi160_setup(struct bmi160_dev *sensor);
int8_t bmi160_get_all_with_time(struct bmi160_dev *sensor, struct bmi160_sensor_data *accel,
                                struct bmi160_sensor_data *gyro);

/* wrapper functions s.t. you can talk to bmm150 via bmi160 */
int8_t user_aux_read(uint8_t id, uint8_t reg_addr, uint8_t *aux_data, uint16_t len);
int8_t user_aux_write(uint8_t id, uint8_t reg_addr, uint8_t *aux_data, uint16_t len);
int8_t bmm150_setup(struct bmm150_dev *dev);
int8_t bmm150_get_data(struct bmm150_dev *dev);

struct bme280_dev bme280;
struct bmi160_dev bmi160;
struct bmm150_dev bmm150;
struct opt3001_dev opt3001;
struct bme280_data bme_data;
struct bmi160_sensor_data bmi_accel;
struct bmi160_sensor_data bmi_gyro;

packet_sensor_t bme280_pressure;
packet_sensor_t bme280_temperature;
packet_sensor_t bme280_humidity;

packet_sensor_t bmi160_accel_x;
packet_sensor_t bmi160_accel_y;
packet_sensor_t bmi160_accel_z;

packet_sensor_t bmm150_gyro_x;
packet_sensor_t bmm150_gyro_y;
packet_sensor_t bmm150_gyro_z;

packet_sensor_t opt3001_lux;


int8_t opt3001_get_lux(struct opt3001_dev *dev);

bool new_event = true;
unsigned char address[] = "invisleash";

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
tock_timer_t sensor_sample_timer;

static void timer_callback( __attribute__ ((unused)) int arg0,
                            __attribute__ ((unused)) int arg1,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  new_event = true;
}

int main(void) {
  int8_t rslt;

  printf_async("TI Sensor BoosterPack with Rf\r\n");
  button_subscribe(button_callback, NULL);

  // we will serlialize packets as human-readable JSON as ASCII
  packetizer_init(SER_MODE_JSON);

  packetizer_add_sensor(&bme280_pressure, "bme280.pressure", UINT);
  packetizer_add_sensor(&bme280_temperature, "bme280.temp", INT);
  packetizer_add_sensor(&bme280_humidity, "bme280.humidity", UINT);

  packetizer_add_sensor(&bmi160_accel_x, "bmi160.accel.x", INT);
  packetizer_add_sensor(&bmi160_accel_y, "bmi160.accel.y", INT);
  packetizer_add_sensor(&bmi160_accel_z, "bmi160.accel.z", INT);

  packetizer_add_sensor(&bmm150_gyro_x, "bmm150.x", INT);
  packetizer_add_sensor(&bmm150_gyro_y, "bmm150.y", INT);
  packetizer_add_sensor(&bmm150_gyro_z, "bmm150.z", INT);

  packetizer_add_sensor(&opt3001_lux, "lux", INT);

  // Enable interrupts on each button.
  int count = button_count();
  for (int i = 0; i < count; i++) {
    button_enable_interrupt(i);
  }

  rslt = bme280_port_init(&bme280);
  if (rslt != BME280_OK) {
    printf_async("BME280 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf_async("BME280 Initialized\r\n");
  }
  bme280_setup(&bme280);

  rslt = bmi160_port_init(&bmi160);
  if (rslt != BMI160_OK) {
    printf_async("BMI160 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf_async("BMI160 Initialized\r\n");
  }
  bmi160_setup(&bmi160);

  rslt = bmm150_port_init(&bmm150);
  if (rslt != BMM150_OK) {
    printf_async("BMM150 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf_async("BMM150 Initialized\r\n");
  }
  bmm150_setup(&bmm150);

  // optionally pass custom parameters instead of NULL
  if (!OPT3001_init(&opt3001, NULL)) {
    printf_async("OPT3001 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf_async("OPT3001 Initialized\r\n");
  }

  if (!rf_driver_check()) {
    printf_async("Driver check OK\r\n");
  } else {
    printf_async("Driver check FAIL\r\n");
  }

  if (!rf_set_oui(address)) {
    printf_async("Address set OK\r\n");
  } else {
    printf_async("Address set FAIL\r\n");
  }

  if (!rf_enable()) {
    printf_async("Radio init OK\r\n");
  } else {
    printf_async("Radio init FAIL\r\n");
  }

  timer_every(6000, timer_callback, NULL, &sensor_sample_timer);

  uint packet_count = 0;
  while (1) {
    int result;
    yield_for(&new_event);

    result = bme280_get_sample_forced_mode(&bme280, &bme_data);
    if (result) {
      printf_async("BME280 Fail");
    }else {
      packet_add_data(&bme280_pressure, &bme_data.pressure);
      packet_add_data(&bme280_temperature, &bme_data.temperature);
      packet_add_data(&bme280_humidity, &bme_data.humidity);
    }

    result = bmi160_get_sensor_data((BMI160_ACCEL_SEL | BMI160_GYRO_SEL | BMI160_TIME_SEL), &bmi_accel, &bmi_gyro,
                                    &bmi160);
    if (result) {
      printf_async("BMI160 Fail");
    }else {
      packet_add_data(&bmi160_accel_x, &bmi_accel.x);
      packet_add_data(&bmi160_accel_y, &bmi_accel.y);
      packet_add_data(&bmi160_accel_z, &bmi_accel.z);
    }

    result = bmm150_read_mag_data(&bmm150);
    if (result) {
      printf_async("BMM150 Fail");
    }else {
      packet_add_data(&bmm150_gyro_x, &bmm150.data.x);
      packet_add_data(&bmm150_gyro_y, &bmm150.data.y);
      packet_add_data(&bmm150_gyro_z, &bmm150.data.z);
    }

    float lux_data;
    result = !OPT3001_getLux(&opt3001, &lux_data);
    if (result) {
      printf_async("OPT3001 Lux fail");
    }else {
      int truncated = (int) lux_data;
      packet_add_data(&opt3001_lux, &truncated);
    }

    packet_t * packet = packet_assemble();
    packet_pretty_print(packet);
    int res = rf_send(packet);

    packet_count++;
    if (res != TOCK_SUCCESS) {
      printf_async("\r\nSend Fail %u\r\n", packet_count);
    } else {
      printf_async("\r\nSend Success %u\r\n", packet_count);
    }

    new_event = false;
    led_off(0);
    led_off(1);
  }

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

int8_t bmi160_setup(struct bmi160_dev *sensor){

  int8_t rslt = BMI160_OK;

  /* Configure device structure for auxiliary sensor parameter */
  sensor->aux_cfg.aux_sensor_enable = 1; // auxiliary sensor enable
  sensor->aux_cfg.aux_i2c_addr      = BMM150_I2C_ADDRESS_CSB_HIGH_SDO_HIGH; // auxiliary sensor address
  sensor->aux_cfg.manual_enable     = 1; // setup mode enable
  sensor->aux_cfg.aux_rd_burst_len  = 2;// burst read of 2 byte

  /* Initialize the auxiliary sensor interface */
  rslt = bmi160_aux_init(sensor);
  if (rslt != BMI160_OK) {
    return rslt;
  }else {
    printf_async("Configured BMM160 as aux device to BMI160\r\n");
  }

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

  /* Set the sensor configuration */
  return bmi160_set_sens_conf(sensor);
}

int8_t bmm150_port_init(struct bmm150_dev *dev){

  /* Sensor interface over I2C */
  dev->dev_id   = BMM150_I2C_ADDRESS_CSB_HIGH_SDO_HIGH;
  dev->intf     = BMM150_I2C_INTF;
  dev->read     = user_aux_read;
  dev->write    = user_aux_write;
  dev->delay_ms = delay_ms;

  return bmm150_init(dev);
}

/*wrapper function to match the signature of bmm150.read */
int8_t user_aux_read(uint8_t id, uint8_t reg_addr, uint8_t *aux_data, uint16_t len)
{
  (void) id;
  int8_t rslt;

  /* Discarding the parameter id as it is redundant*/
  rslt = bmi160_aux_read(reg_addr, aux_data, len, &bmi160);

  return rslt;
}

/*wrapper function to match the signature of bmm150.write */
int8_t user_aux_write(uint8_t id, uint8_t reg_addr, uint8_t *aux_data, uint16_t len)
{
  (void) id;
  int8_t rslt;

  /* Discarding the parameter id as it is redundant */
  rslt = bmi160_aux_write(reg_addr, aux_data, len, &bmi160);

  return rslt;
}

int8_t bmm150_setup(struct bmm150_dev *dev)
{
  int8_t rslt;

  /* Setting the power mode as normal */
  dev->settings.pwr_mode = BMM150_NORMAL_MODE;
  rslt = bmm150_set_op_mode(dev);

  /* Setting the preset mode as Low power mode
     i.e. data rate = 10Hz XY-rep = 1 Z-rep = 2*/
  dev->settings.preset_mode = BMM150_PRESETMODE_LOWPOWER;
  rslt = bmm150_set_presetmode(dev);

  return rslt;
}
