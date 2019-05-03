#include "battery.h"

#define DRIVER_NUM_BATTERY 0xB

typedef enum {
  READ = 1,
} cmd_t;

int battery_read_mv(void){
  return command(DRIVER_NUM_BATTERY, READ, 0, 0);
}


