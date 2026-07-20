#include <stdio.h>
#include "event_bus.h"


event_type_t input_event;

void print(void* data)
{
    printf("print:%s\r\n", data);
}

void cb(void* data)
{
    static uint8_t cnt;
    printf("%d\r\n", cnt++);
}

int main(void)
{
    if(event_init(&input_event, "input_event") == false)
    {
        printf("event_init 错误\r\n");
        return -1;
    }

    if(event_subscribe(&input_event, print) == false)
    {
        printf("event_subcribe 错误\r\n");
        return -1;
    }

    if(event_subscribe(&input_event, cb) == false)
    {
        printf("event_subcribe 错误\r\n");
        return -1;
    }

    char buff[100];

    while(1)
    {
        scanf("%s", buff);
        event_publish(&input_event, buff);
    }


    return 0;
}
