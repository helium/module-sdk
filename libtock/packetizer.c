#include "packetizer.h"
#include <stdlib.h>
#include <uart.h>

typedef struct {
  char* label;
  uint8_t len;
  DataType type;
} json_field_t;

struct Packetizer {
  // pointer to array of fields (which are char * themselves)
  json_field_t* fields;
  uint8_t num;
};

static SerializationMode mode       = SER_MODE_UNINITIALIZED;
static struct Packetizer packetizer = { .fields =  NULL, .num = 0};


PacketizerStatus packetizer_init(SerializationMode mode_set) {
  // free the memory used if it exists
  // deinit call most likely which goes thru the struct frees it all
  mode = mode_set;

  if (mode != SER_MODE_JSON) {
    printf("Only JSON mode is currently supported by packetizer!");
    while (1) {};
  }

  return PACKETIZER_OK;
}

PacketizerStatus packetizer_add_sensor(packet_sensor_t* sensor, const char * label, DataType data_type){
  // allocate for the label
  uint8_t len      = strlen(label);
  char * new_label = malloc(len + 1);
  memcpy(new_label, label, len + 1);

  // allocate a new array for the json_fields
  json_field_t* new_array = malloc(sizeof(json_field_t) * ++packetizer.num);

  for (uint8_t i = 0; i < packetizer.num; i++) {
    if (i == packetizer.num - 1) {
      new_array[i].label = new_label;
      new_array[i].len   = len;
      new_array[i].type  = data_type;
      // printf("%u\r\n", new_array[i].type );
      sensor->index = i;
      sensor->type  = data_type;
    } else {
      // copy over the old fields to the new block
      memcpy((new_array + i), &packetizer.fields[i], sizeof(json_field_t));
    }
  }
  packetizer.fields = new_array;
  return PACKETIZER_OK;
};

typedef struct  {
  char * data;
  int len;
} datapoint_t;

static datapoint_t* current_packet;

PacketStatus packet_add_data(packet_sensor_t* sensor, void* data) {
  // printf("crash\r\n");
  PacketStatus ret = PACKET_OK;
  // if there is no active packet being assembled,
  // allocate one
  if (current_packet == NULL) {
    current_packet = calloc(1, sizeof(datapoint_t) * packetizer.num);
  }

  // if there is already stuff here, free it
  if ( current_packet[sensor->index].data != NULL) {
    free( current_packet[sensor->index].data );
    ret = PACKET_VALUE_OVERWRITTEN;
  }

  char * label = packetizer.fields[sensor->index].label;
  char * value;
  int bytes_required;
  switch (sensor->type) {
    case UINT:
      bytes_required = 3 + packetizer.fields[sensor->index].len + 11 + 1;
      value = malloc(bytes_required * 2);
      bytes_required = snprintf(value, bytes_required, "\"%s\":%u", label, *((uint*)data)) + 1;
      if (value != NULL) {
        current_packet[sensor->index].data = value;
        current_packet[sensor->index].len  = bytes_required - 2;
      }
      break;
    case INT:
      bytes_required = 3 + packetizer.fields[sensor->index].len + 12 + 1;
      value = malloc(bytes_required * 2);
      bytes_required = snprintf(value, bytes_required, "\"%s\":%i", label, *((int*)data)) + 1;
      if (value != NULL) {
        current_packet[sensor->index].data = value;
        current_packet[sensor->index].len  = bytes_required;
      }
      break;
    case FLOAT:
      break;
    case DOUBLE:
      break;
  }
  return ret;
};

void packetizer_debug(void){
  for (uint8_t i = 0; i < packetizer.num; i++) {
    uart_writestr(0, packetizer.fields[i].label);
    // uart_write(0, packetizer.fields[i].label, packetizer.fields[i].len);
    uart_writestr(0, "\r\n");
  }
};

packet_t* packet_assemble(void){
  int count         = 0;
  char prefix[]     = "{";
  int prefix_len    = strlen(prefix);
  int packet_length = prefix_len;

  // calculate space needed for assembled packet
  for (uint8_t i = 0; i < packetizer.num; i++) {
    if (current_packet[i].data != NULL) {
      packet_length += current_packet[i].len + 1;       // length plus comma
    }
  }

  // allocate space for assembled packet
  char * payload = malloc(packet_length + 1);
  // set prefix
  memcpy(payload, &prefix, prefix_len);
  count += prefix_len;

  // copy over strings into payload
  for (uint8_t i = 0; i < packetizer.num; i++) {
    if (current_packet[i].data != NULL) {
      memcpy(payload + count, current_packet[i].data, current_packet[i].len);
      count += current_packet[i].len;
      payload[count - 1] = ',';
      free(current_packet[i].data);
    }
  }
  free(current_packet);
  current_packet = NULL;

  payload[count - 1] = '}';
  payload[count]     = '\0';

  packet_t* packet = malloc(sizeof(packet_t));

  packet->data = (unsigned char*) payload;
  packet->len  = count;

  return (packet_t*) packet;
};

void packet_pretty_print(packet_t* packet){
  uint pointer_last = 1;
  uint pointer      = 1;
  // start the printing
  uart_writestr_async(0, "{\r\n  ", NULL);
  while (packet->data[pointer] != '}' && pointer < packet->len) {
    pointer++;
    // find next comma
    while (packet->data[pointer] != ':') {
      pointer++;
    }
    int len = pointer - pointer_last + 1;
    uart_write_async(0, &packet->data[pointer_last], len, NULL);
    uart_writestr_async(0, " ", NULL);

    pointer_last = ++pointer;
    // find next comma
    while (packet->data[pointer] != ',' && packet->data[pointer] != '}') {
      pointer++;
    }
    len = pointer - pointer_last;
    uart_write_async(0, &packet->data[pointer_last], len, NULL);

    if (packet->data[pointer] != '}') {
      uart_writestr_async(0, ",\r\n  ", NULL);
    }else {
      uart_writestr_async(0, "\r\n}\r\n", NULL);
    }

    pointer_last = ++pointer;
  }
}

void packet_disassemble(packet_t* packet){
  free(packet->data);
  free(packet);
}