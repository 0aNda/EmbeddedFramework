#ifndef __UATT_DMA_H
#define __UATT_DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ring_buffer.h"

typedef bool (*uart_dma_init_fn)(void *context);
typedef size_t (*uart_dma_get_pos_t)(void *context);

typedef struct {
    uart_dma_init_fn init;
    uart_dma_get_pos_t get_pos;   
}uart_dma_ops_t;

typedef struct {
    void *context;
    uart_dma_ops_t *ops;
}uart_dma_dev_t;



#endif
