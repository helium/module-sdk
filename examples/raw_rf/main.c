#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <button.h>
#include <rf.h>
#include <timer.h>


// Timer callback
tock_timer_t simple_timer;

static bool new_event = false;

static void timer_callback( __attribute__ ((unused)) int arg0,
                            __attribute__ ((unused)) int arg1,
                            __attribute__ ((unused)) int arg2,
                            __attribute__ ((unused)) void *ud) {
  new_event = true;
}

char* create_payload(void);

int main(void) {
  printf("Simple RF Demo\r\n");

  if (!rf_driver_check()) {
    printf("Driver check OK\r\n");
  } else {
    printf("Driver check FAIL\r\n");
  }


  if (!rf_enable()) {
    printf("Radio init OK\r\n");
  } else {
    printf("Radio init FAIL\r\n");
  }

  timer_every(5000, timer_callback, NULL, &simple_timer);

  while (1) {
    yield_for(&new_event);

    raw_packet_t * pkt = calloc(1, sizeof(raw_packet_t));
    pkt->data = malloc(8);
    pkt->len = 8;
    printf("sending\r\n");
    int res = rf_send_raw(pkt);
    printf("sent\r\n");

  }

  return 0;
}
