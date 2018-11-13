/* vim: set sw=2 expandtab tw=80: */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <timer.h>
#include <uart.h>

#define DEBUG_UART  0

#define NUM_REQUESTS   16

static uint count    = 0;
static uint cb_count = 0;
static bool cb_complete;
void uart_callback(uint8_t uart_num __attribute__ ((unused)),
                   uint8_t * data __attribute__ ((unused)),
                   uint8_t len __attribute__ ((unused)));
void uart_callback(uint8_t uart_num __attribute__ ((unused)),
                   uint8_t * data __attribute__ ((unused)),
                   uint8_t len __attribute__ ((unused))){
  uart_writestrf_async(DEBUG_UART, NULL, "[UART Async] Callback fired %u\r\n", count);
  cb_count++;
  if (cb_count == NUM_REQUESTS) {
    cb_complete = true;
  }
}

int main(void) {
  uart_writestr_async(DEBUG_UART, "\r\n[UART Async] Hello, world!\r\n", NULL);
  while (1) {
    cb_count    = 0;
    cb_complete = false;
    for (uint8_t i = 0; i < NUM_REQUESTS; i++) {
      uart_writestrf_async(DEBUG_UART, &uart_callback, "[UART Async] Message %u\r\n", count++);
    }
    // since we are printing from the callback, it's important to yield and let the queue clear
    yield_for(&cb_complete);
  }

  return 0;
}
