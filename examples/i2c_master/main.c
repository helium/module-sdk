#include <stdio.h>
#include <stdlib.h>

#include <i2c_master.h>
#include <timer.h>

#define BOSCH_BME_ADDR   0x77
#define BOSCH_BME_ID_REG 0xD0
#define BOSCH_BME_ID     0x60

#define BOSCH_BME_CTRL_MEAS_REG 0xF4

int main(void) {
  printf("[I2C Master] Example started\r\n");

  uint8_t response;
  i2c_master_read_register(BOSCH_BME_ADDR, BOSCH_BME_ID_REG, &response);

  if (response == BOSCH_BME_ID) {
    printf("[I2C Master] Bosch BME280 Detected\r\n");
  }else  {
    printf("[I2C Master] Bosch BME280 Not Detected! ID reg read: %x\r\n", response);
    return 0;
  }

  uint8_t arbitrary_config = 0xAB;
  i2c_master_write_register(BOSCH_BME_ADDR, BOSCH_BME_CTRL_MEAS_REG, arbitrary_config);
  uint8_t read_config;
  i2c_master_read_register(BOSCH_BME_ADDR, BOSCH_BME_CTRL_MEAS_REG, &read_config);

  if (arbitrary_config == read_config) {
    printf("[I2C Master] Writing to Bosch Sensor OK\r\n");
  }else  {
    printf("[I2C Master] Writing to Bosch Sensor Failed. Wrote %x and read %x.\r\n", arbitrary_config, read_config);
  }

}