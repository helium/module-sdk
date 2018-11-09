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
#include <console.h>
#include <i2c_master.h>
#include <led.h>
#include <led.h>
#include <rfcore.h>
#include <timer.h>

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

int8_t opt3001_get_lux(struct opt3001_dev *dev);

bool new_event = true;
char message[125];
const char address[] = "mmc";

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

  printf("TI Sensor BoosterPack with Helium");

  button_subscribe(button_callback, NULL);

  // Enable interrupts on each button.
  int count = button_count();
  for (int i = 0; i < count; i++) {
    button_enable_interrupt(i);
  }

  rslt = bme280_port_init(&bme280);
  if (rslt != BME280_OK) {
    printf("BME280 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf("BME280 Initialized\r\n");
  }
  bme280_setup(&bme280);

  rslt = bmi160_port_init(&bmi160);
  if (rslt != BMI160_OK) {
    printf("BMI160 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf("BMI160 Initialized\r\n");
  }
  bmi160_setup(&bmi160);

  rslt = bmm150_port_init(&bmm150);
  if (rslt != BMM150_OK) {
    printf("BMM150 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf("BMM150 Initialized\r\n");
  }
  bmm150_setup(&bmm150);

  // optionally pass custom parameters instead of NULL
  if (!OPT3001_init(&opt3001, NULL)) {
    printf("OPT3001 Initialization Failed\r\n");
    while (1) ;
  }else {
    printf("OPT3001 Initialized\r\n");
  }

  if (!helium_driver_check()) {
    printf("Driver check OK\r\n");
  } else {
    printf("Driver check FAIL\r\n");
  }

  if (!helium_init()) {
    printf("Helium init OK\r\n");
  } else {
    printf("Helium init FAIL\r\n");
  }

  timer_every(30000, timer_callback, NULL, &sensor_sample_timer);

  while (1) {
    yield_for(&new_event);
    bme280_get_sample_forced_mode(&bme280, &bme_data);
    bmi160_get_all_with_time(&bmi160, &bmi_accel, &bmi_gyro);
    bmm150_get_data(&bmm150);
    bool lux = opt3001_get_lux(&opt3001);

    snprintf(message, 125,
             "{\"team\":\"%3s\",\"payload\":{\"bme\":%ld,\"bmi\":{\"x\":%d,\"y\":%d,\"z\":%d},\"lux\":%d,\"bmm\":{\"x\":%d,\"y\":%d,\"z\":%d}}}\0",
             address, bme_data.temperature, bmi_gyro.x,bmi_gyro.y,bmi_gyro.z,lux,bmm150.data.x,bmm150.data.y,
             bmm150.data.z);
    printf("Message [%s]", message);

    int res = helium_send(0x0000, CAUT_TYPE_NONE, message, strlen(message));
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

  /* Read status byte to verify */
  uint8_t data[1];
  do {
    data[0] = 0xF3;
    i2c_master_write_read(0x77, data, 1, 1);
  }
  while (data[0] != 0);

  rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);
  if (rslt != BME280_OK) {
    return rslt;
  }

  print_sensor_data(comp_data);
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
    char debug[] = "Configured BMM160 as aux device to BMI160\r\n";
    putnstr(debug, sizeof(debug));
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

int8_t bmi160_get_all_with_time(struct bmi160_dev *sensor, struct bmi160_sensor_data *accel,
                                struct bmi160_sensor_data *gyro){
  int8_t rslt;
  // struct bmi160_sensor_data accel;
  // struct bmi160_sensor_data gyro;

  /* To read both Accel and Gyro data along with time*/
  rslt = bmi160_get_sensor_data((BMI160_ACCEL_SEL | BMI160_GYRO_SEL | BMI160_TIME_SEL), accel, gyro, sensor);
  bmi160_print_data(accel, gyro);
  return rslt;
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

int8_t bmm150_get_data(struct bmm150_dev *dev){
  int8_t rslt;
  /* Mag data for X,Y,Z axis are stored inside the
     bmm150_dev structure in int16_t format */
  rslt = bmm150_read_mag_data(dev);
  bmm150_print_data(&dev->data);
  return rslt;
}

void opt3001_print_lux(float lux);
void opt3001_print_lux(float lux){
  char beg[] = "OPT3001 Lux ";
  putnstr(beg, sizeof(beg));

  char lux_output[] = "                                  ";
  itoa (lux, lux_output, 10);
  putnstr(lux_output, sizeof(lux_output));

  putnstr("\r\n", 2);
}

int8_t opt3001_get_lux(struct opt3001_dev *dev){
  float data;
  bool res = OPT3001_getLux(dev, &data);

  opt3001_print_lux(data);

  return res != true;
}
