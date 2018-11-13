// driver for collecting analog samples
#pragma once

#include <stdint.h>

#include "tock.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_NUM_ADC 0x5

// get a single adc reading on a selected channel
int adc_read(uint8_t channel, uint16_t* sample);

typedef void (*adc_read_cb)(int chan, uint16_t sample, bool queue_empty);
int adc_read_async(uint8_t channel, adc_read_cb cb);


#ifdef __cplusplus
}
#endif
