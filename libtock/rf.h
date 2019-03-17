#pragma once

#include "tock.h"
#include "packetizer.h"
#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct raw_packet {
	  uint8_t * data;
	  uint32_t len;
	} raw_packet_t;
	
	enum payload_type {
	  PL_TYPE_NONE = 0,
	  PL_TYPE_PACKETIZER = 1,
	  PL_TYPE_CAUTERIZE = 2,
	};
	
	typedef struct payload {
	  packet_t* packet;
	  raw_packet_t* raw_packet;
	  enum payload_type type;
	} payload_t;


	typedef void (*rf_cb)(int result, payload_t* payload);

	typedef struct rf_req {
	  payload_t* payload;	  
	  rf_cb user_cb;
	  struct rf_req* next;
	} rf_req_t;
		
	int rf_enable(void);
    
    int rf_driver_check(void);

    int rf_set_oui(unsigned char *address);

    // sends the packet and frees memory allocated to packet
    int rf_send(packet_t* packet);
	
	int rf_send_raw(raw_packet_t* packet);

	int rf_send_async(packet_t* packet, rf_cb cb);

	int rf_send_raw_async(raw_packet_t* packet, rf_cb cb);
#ifdef __cplusplus
}
#endif
