#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <button.h>
#include <console.h>
#include <led.h>
#include <led.h>
#include <rfcore.h>
#include <timer.h>

bool new_event = true;
char message[125];
unsigned char address[] = "somewhere";

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
tock_timer_t simple_timer;

static void timer_callback( __attribute__ ((unused)) int arg0,
                            __attribute__ ((unused)) int arg1,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  new_event = true;
}

char* create_payload(void);

int main(void) {
  printf("Simple RF Demo!");

  button_subscribe(button_callback, NULL);

  // Enable interrupts on each button.
  int count = button_count();
  for (int i = 0; i < count; i++) {
    button_enable_interrupt(i);
  }

  if (!helium_driver_check()) {
    printf("Driver check OK\r\n");
  } else {
    printf("Driver check FAIL\r\n");
  }

  if (!helium_set_address(address)) {
    printf("Set address OK\r\n");
  } else {
    printf("Set address FAIL\r\n");
  }

  if (!helium_init()) {
    printf("Helium init OK\r\n");
  } else {
    printf("Helium init FAIL\r\n");
  }

  timer_every(30000, timer_callback, NULL, &simple_timer);

  while (1) {
    yield_for(&new_event);

    snprintf(message, 125,
             "{\"team\":\"%9s\",\"payload\":\"Hello World!\"}\0",
             address);
    printf("Message [%s]", message);

    int res = helium_send(0x0000, CAUT_TYPE_NONE, message, sizeof(message));
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
