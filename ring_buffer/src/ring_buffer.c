#include "ring_buffer.h"

bool ring_buffer_init(ring_buffer_t* ring_buf, void* buffer, uint32_t buff_size, uint32_t e_size)
{
    if(ring_buf == NULL || buffer == NULL)
        return false;

    if(e_size == 0)
        return false;

    uint32_t esize_num = buff_size / e_size;

    if((esize_num & (esize_num - 1)) != 0)
        return false;

    ring_buf->in = 0;
    ring_buf->out = 0;
    ring_buf->mask = esize_num - 1;
    ring_buf->data = buffer;
    ring_buf->e_size = e_size;

    return true;
}

bool ring_buffer_is_empty(const ring_buffer_t* ring_buf)
{
    return ring_buf->in == ring_buf->out;
}

bool ring_buffer_is_full(const ring_buffer_t* ring_buf)
{
    return (ring_buf->in - ring_buf->out) == (ring_buf->mask + 1);
}

static void ring_buffer_copy_in(ring_buffer_t* ring_buf, const void* buffer, uint32_t len)
{
    uint32_t index = (ring_buf->in & ring_buf->mask) * ring_buf->e_size;
    uint32_t size = (ring_buf->mask + 1) * ring_buf->e_size;
    uint32_t l =  len;

    if(len > size - index)
        l = size - index;

    memcpy(ring_buf->data + index, (uint8_t *)buffer, l);
    memcpy(ring_buf->data, (uint8_t *)buffer + l, len - l);
}

uint32_t ring_buffer_write(ring_buffer_t* ring_buf, const void* src, uint32_t len)
{
    if(ring_buffer_is_full(ring_buf))
        return 0;

    uint32_t l = ring_buffer_free_len(ring_buf);

    if(len > l)
        len = l;
 
    ring_buffer_copy_in(ring_buf, src, len * ring_buf->e_size);
    ring_buf->in += len;

    return len;
}

static void ring_buffer_copy_out(ring_buffer_t* ring_buf, void* buffer, uint32_t len)
{
    uint32_t index = (ring_buf->out & ring_buf->mask) * ring_buf->e_size;
    uint32_t size = (ring_buf->mask + 1) * ring_buf->e_size;
    uint32_t l = len;

    if(len > size - index)
        l = size - index;

    memcpy((uint8_t *)buffer, ring_buf->data + index, l);
    memcpy((uint8_t *)buffer + l, ring_buf->data, len - l);

}

uint32_t ring_buffer_read(ring_buffer_t* ring_buf, void* buffer, uint32_t len)
{
    if(ring_buffer_is_empty(ring_buf))
        return 0;

    if(buffer == NULL)
        return 0;

    uint32_t l = ring_buffer_data_len(ring_buf);
    
    if(len > l)
        len = l;

    ring_buffer_copy_out(ring_buf, buffer, len * ring_buf->e_size);
    ring_buf->out += len;

    return len;
}

uint32_t ring_buffer_data_len(const ring_buffer_t* ring_buf)
{
    return ring_buf->in - ring_buf->out;
}

uint32_t ring_buffer_free_len(const ring_buffer_t* ring_buf)
{
    return (ring_buf->mask + 1) - ring_buffer_data_len(ring_buf);
}
