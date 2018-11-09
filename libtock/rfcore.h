#pragma once

#include "tock.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef enum {
        CAUT_TYPE_NONE = 0,
        CAUT_TYPE_STANDARD = 1,
        CAUT_TYPE_CUSTOM = 2,
    } caut_type_t;

    int helium_init(void);
    
    int helium_driver_check(void);

    int helium_set_address(unsigned char *address);

    int helium_send(unsigned short addr,
            caut_type_t type,
            const char *payload,
            unsigned char len);

#ifdef __cplusplus
}
#endif
