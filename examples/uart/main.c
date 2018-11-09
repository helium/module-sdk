#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <timer.h>
#include <uart.h>

#define DEBUG_UART  0

int main(void) {
  uint count = 0;
  uart_writestr(DEBUG_UART, "\r\n[UART Sync] Hello, world!\r\n");
  while (1) {
    for (uint8_t i = 0; i < 4; i++) {
      uart_writestrf(DEBUG_UART, "[UART Sync] Message %u\r\n", count++);
    }
  }
  return 0;
}
