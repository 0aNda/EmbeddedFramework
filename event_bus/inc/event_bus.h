#ifndef __EVENT_BUS_H
#define __EVENT_BUS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define CB_NUM_MAX 8


typedef void (*event_callback_t)(void* data);

typedef struct{
    const char* name;
    void* data;
    event_callback_t callback[CB_NUM_MAX];
    uint8_t cb_index;
}event_type_t;


bool event_init(event_type_t* event_type, const char* name);
bool event_subscribe(event_type_t* event_type, void (*callback)(void* data));
void event_publish(event_type_t* type, void* data);


#endif


