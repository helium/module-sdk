#pragma once

#include "tock.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DRIVER_NUM_UART 0x1

// Synchronous API
int uart_read(uint8_t uart_num, uint8_t* buf, size_t len);
int uart_write(uint8_t uart_num, uint8_t* buf, size_t len);
int uart_writestr(uint8_t uart_num, const char* buf);
int uart_writestrf(uint8_t uart_num, const char *fmt, ...) __attribute__((format(printf, 2, 0)));
int uart_read_byte(uint8_t uart_num, uint8_t * data);
// Abort an ongoing receive call.
int uart_read_abort(uint8_t uart_num);

// Asynchronous API
typedef enum  {
  WRITE_COMPLETE,
  READ_COMPLETE
} uart_callback_t;

// data provides pointer to read or write buffer
// complete boolean lets you know that all UART requests on all UARTs are done
typedef void (*uart_cb)(uint8_t uart_num, uint8_t * data, uint8_t len);

// copies and allocates buffer to heap, meaning client can safely call
int uart_write_async(uint8_t uart_num, uint8_t* buf, size_t len, uart_cb cb);

// depends HEAP allocated buf before call, since no copy is made
// WARNING: internal library callback will free buf after calling user callback
int uart_write_async_unsafe(uint8_t uart_num, uint8_t* buf, size_t len, uart_cb cb);

int uart_read_async(uint8_t uart_num, uint8_t* buf, size_t len, uart_cb cb);
// allows writing of null terminated strings
int uart_writestr_async(uint8_t uart_num, const char* buf, uart_cb cb);
int uart_writestrf_async(uint8_t uart_num,  uart_cb cb, const char *format, ...) __attribute__((format(printf, 3, 0)));

// an asynchronous printf - important for printing from within UART callback
int printf_async(const char *format, ...) __attribute__((format(printf, 1, 0)));



#ifdef __cplusplus
}
#endif
