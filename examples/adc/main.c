#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <timer.h>

#include <adc.h>

// #define ALL_FIRED 0b11111
// uint8_t fired_channels;
bool adc_complete;
void adc_callback(int chan, uint16_t sample, bool queue_empty);
void adc_callback(int chan, uint16_t sample, bool queue_empty){
  printf("[ADC Async] Channel %u Raw: %u\r\n", chan, sample);
  uint32_t voltage =  (((sample * 4300) + 2047) / 4095);
  printf("[ADC Async] Channel %u mV : %lu\r\n", chan, voltage);

  adc_complete = queue_empty;
}

// int main(void) {

//   printf("[ADC] Starting ADC App.\r\n");
//   bool adc = driver_exists(DRIVER_NUM_ADC);
  
//   if (adc) {
//     printf("[ADC] ADC Driver Detected\r\n");
//   }

//   while(adc) {
//     all_adc_complete = false;
//     fired_channels = 0;
//     for(uint8_t i=0; i<5; i++){

//       adc_read_async(i, &adc_callback);

//       yield_for(&all_adc_complete);
//     }
//     delay_ms(1000);
//   }

//   printf("[ADC] Complete");

  
//   return 0;
// }

int main(void) {

  printf("[ADC] Starting ADC App.\r\n");
  bool adc = driver_exists(DRIVER_NUM_ADC);
  
  if (adc) {
    printf("[ADC] ADC Driver Detected\r\n");
  }

  while(true & adc) {
    for(uint8_t i=0; i<5; i++){
      adc_read_async(i, &adc_callback);
    }
    yield_for(&adc_complete);
    adc_complete = false;

    // for(uint8_t i=0; i<5; i++){
    //   uint16_t value;
    //   adc_read(i, &value);
    //   printf("[ADC Sync] Channel %u Raw : %u\r\n", i, value);
    //   uint32_t voltage =  (((value * 4300) + 2047) / 4095);
    //   printf("[ADC Sync] Channel %u Volt: %lu\r\n", i, voltage);
    // }
    
    printf("\r\n");
    delay_ms(1000);
  }

  
  return 0;
}