
#pragma once

#include "tock.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SER_MODE_UNINITIALIZED,
    SER_MODE_JSON
} SerializationMode;

typedef enum {
    PACKETIZER_OK = 0,
    PACKETIZER_NO_MEMORY
} PacketizerStatus;

typedef enum {
    UINT = 0,
    INT = 1,
    FLOAT = 2,
    DOUBLE = 3
} DataType;

typedef struct {
    char * label;
    uint8_t index;
    DataType type;
} packet_sensor_t;

typedef enum {
    PACKET_OK = 0,
    PACKET_NO_MEMORY,
    PACKET_VALUE_OVERWRITTEN,
} PacketStatus;

PacketizerStatus packetizer_init(SerializationMode mode);
PacketizerStatus packetizer_add_sensor(packet_sensor_t* sensor, const char * label, DataType data_type);

PacketStatus packet_add_data(packet_sensor_t* sensor, void* data);

typedef struct{
    uint8_t * data;
    uint32_t len;
} packet_t ;

packet_t* packet_assemble(void);
void packet_disassemble(packet_t* packet);
void packet_pretty_print(packet_t* packet);
void packetizer_debug(void);

#ifdef __cplusplus
}
#endif
