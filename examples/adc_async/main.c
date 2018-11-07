#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <timer.h>

#include <adc.h>

bool adc_queue_empty;
void adc_callback(int chan, uint16_t sample, bool queue_empty);
void adc_callback(int chan, uint16_t sample, bool queue_empty){
  printf("[ADC Async] Channel %u Raw: %u\r\n", chan, sample);
  uint32_t voltage =  (((sample * 4300) + 2047) / 4095);
  printf("[ADC Async] Channel %u mV : %lu\r\n", chan, voltage);

  adc_queue_empty = queue_empty;
}


int main(void) {

  printf("[ADC Async] Starting ADC App.\r\n");
  bool adc = driver_exists(DRIVER_NUM_ADC);
  
  if (adc) {
    printf("[ADC Async] ADC Driver Detected\r\n");
  }

  while(true & adc) {
    adc_queue_empty = false;
    for(uint8_t i=0; i<5; i++){
      adc_read_async(i, &adc_callback);
    }
    printf("\r\n");
    yield_for(&adc_queue_empty);
    
    delay_ms(1000);
  }

  
  return 0;
}