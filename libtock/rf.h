#pragma once

#include "tock.h"

#ifdef __cplusplus
extern "C" {
#endif

    int rf_enable(void);
    
    int rf_driver_check(void);

    int rf_set_oui(unsigned char *address);

    // sends the packet and frees memory allocated to packet
    int rf_send(packet_t* packet);

    typedef void (*rf_cb)(int result, packet_t* packet);
    int rf_send_async(packet_t* packet, rf_cb cb);

#ifdef __cplusplus
}
#endif
