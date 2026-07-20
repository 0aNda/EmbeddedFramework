#include <stdio.h>
#include "ring_buffer.h"

struct buff_t
{
    uint16_t num;
    uint8_t cnt;
};

struct buff_t buff[8];

ring_buffer_t rb;

int main(void)
{
    if(ring_buffer_init(&rb, buff, sizeof(buff), sizeof(struct buff_t)) == false)
    {
        printf("init falut\r\n");
        return -1;
    }

    struct buff_t buf1 = {.cnt = 5, .num = 6};
    uint8_t w = 0;
    w = ring_buffer_write(&rb, &buf1, 1);
    printf("w: %d\r\n", w);

    printf("num:%d cnt:%d\r\n", buff[0].num, buff[0].cnt);
    printf("num:%d cnt:%d\r\n", buff[1].num, buff[1].cnt);

    uint8_t r = 0;
    struct buff_t buf2 = {0};
    r = ring_buffer_read(&rb, &buf2, 1);
    printf("r: %d\r\n", r);
    printf("buf2_num: %d  cnt: %d\r\n", buf2.num, buf2.cnt);

    return 0;
}