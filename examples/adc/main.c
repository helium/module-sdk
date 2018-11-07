#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <timer.h>

#include <adc.h>

int main(void) {
  printf("[ADC Sync] Starting ADC App.\r\n");
  bool adc = driver_exists(DRIVER_NUM_ADC);
  
  if (adc) {
    printf("[ADC Sync] ADC Driver Detected\r\n");
  }

  while(adc) {
    for(uint8_t i=0; i<5; i++){
      uint16_t value;
      adc_read(i, &value);
      printf("[ADC Sync]  Channel %u Raw : %u\r\n", i, value);
      uint32_t voltage =  (((value * 4300) + 2047) / 4095);
      printf("[ADC Sync]  Channel %u Volt: %lu\r\n", i, voltage);
    }
    printf("\r\n");
    delay_ms(1000);
  }

  printf("[ADC Sync] Application Terminated");

  return 0;
}