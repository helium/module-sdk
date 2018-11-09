#include "../bosch_port/bosch_port.h"
#include "bme280.h"
#include "bme280_port.h"

#include <stdlib.h>
#include <timer.h>

int8_t bme280_port_init(struct bme280_dev *dev){
  dev->dev_id   = BME280_I2C_ADDR_SEC;
  dev->intf     = BME280_I2C_INTF;
  dev->read     = (bme280_com_fptr_t) tock_i2c_read;
  dev->write    = (bme280_com_fptr_t) tock_i2c_write;
  dev->delay_ms = delay_ms;

  return bme280_init(dev);
}
