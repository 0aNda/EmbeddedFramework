#ifndef __UART_DAMA_H
#define __UART_DAMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ring_buffer.h"
#include "event_bus.h"
#include "uart_dma_hal.h"

typedef struct {
    uart_dma_dev_t *dev;
    uint8_t *buffer;
    size_t buffer_size;
    ring_buffer_t rx_buffer;
    size_t last_pos;

    event_type_t *rx_event;
} uart_dma_t;

bool uart_dma_init(uart_dma_t *uart_dma, uart_dma_dev_t *dev, uint8_t *dma_buffer, size_t dma_buffer_size,
                   uint8_t *ring_buffer, size_t ring_buffer_size, event_type_t *rx_event);

void uart_dma_idle_irq(void *data);
void uart_dma_ht_irq(void *data);
void uart_dma_tc_irq(void *data);

void uart_dma_process(uart_dma_t *uart_dma);
size_t uart_dma_read(uart_dma_t *uart_dma, uint8_t *buffer, size_t len);

#endif
