#include "../bosch_port/bosch_port.h"
#include "bmi160.h"
#include "bmi160_port.h"

#include <stdlib.h>
#include <timer.h>

int8_t bmi160_port_init(struct bmi160_dev *dev){
  dev->id        = BMI160_I2C_ADDR | 0b1;
  dev->interface = BMI160_I2C_INTF;
  dev->read      = (bmi160_com_fptr_t) tock_i2c_read;
  dev->write     = (bmi160_com_fptr_t) tock_i2c_write;
  dev->delay_ms  = delay_ms;

  return bmi160_init(dev);
}
