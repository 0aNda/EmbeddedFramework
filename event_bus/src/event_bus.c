    #include "event_bus.h"

    #define RECORD_NUM_MAX 10

    static event_type_t* record[RECORD_NUM_MAX];
    static  uint8_t record_index;

    bool event_init(event_type_t* event_type, const char* name)
    {
        if(event_type == NULL)
            return false;

        if(name == NULL)
            return false;

        if(record_index >= RECORD_NUM_MAX)
            return false;

        memset(event_type, 0, sizeof(event_type_t));

        event_type->name = name;

        record[record_index++] = event_type;

        return true;
    }


    bool event_subscribe(event_type_t* event_type, void (*callback)(void* data))
    {
        if(event_type == NULL)
            return false;

        if(callback == NULL)
            return false;   

        if(event_type->cb_index >= CB_NUM_MAX)
            return false;

        event_type->callback[event_type->cb_index] = callback;
        event_type->cb_index++;

        return true;
    }

    void event_publish(event_type_t* type, void* data)
    {
        uint8_t subscribe_index = 0;

        if(type == NULL || data == NULL)
            return;

        for(subscribe_index = 0; subscribe_index < type->cb_index; subscribe_index++)
        {
            type->callback[subscribe_index](data);
        }

    }


