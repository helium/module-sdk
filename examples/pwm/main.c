#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <pwm.h>
#include <timer.h>

int main(void) {
  printf("[PWM] Example started\r\n");

  // set frequency to 60kHz
  pwm_set_frequency(0, 60000);

  float value = 0.0;
  bool dir_up = true;
  while(1){

    // increment or decrement value
    if(dir_up){
        value += 1.0;
    }
    else{
        value -= 1.0;
    }
    pwm_set_duty_cycle(1, value);
    
    // change direction depending on where we are
    if(value >= 100.0){
        dir_up = false;
    }
    else if (value <= 0.0){
        dir_up = true;
    }
    
    delay_ms(1);
  }

  return 0;
}