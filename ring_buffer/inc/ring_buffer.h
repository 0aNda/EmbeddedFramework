#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct{
    uint32_t    in;
    uint32_t    out;
    uint32_t    mask;
    uint8_t     *data;
    uint32_t    e_size;
}ring_buffer_t;

bool ring_buffer_init(ring_buffer_t* ring_buf, void* buffer, uint32_t buff_size, uint32_t e_size);
bool ring_buffer_is_empty(const ring_buffer_t* ring_buf);
bool ring_buffer_is_full(const ring_buffer_t* ring_buf);
uint32_t ring_buffer_write(ring_buffer_t* ring_buf, const void* src, uint32_t len);
uint32_t ring_buffer_read(ring_buffer_t* ring_buf, void* buffer, uint32_t len);
uint32_t ring_buffer_data_len(const ring_buffer_t* ring_buf);
uint32_t ring_buffer_free_len(const ring_buffer_t* ring_buf);


#endif
