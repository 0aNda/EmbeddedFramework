#include "uart_dma.h"

bool uart_dma_init(uart_dma_t *uart_dma, uart_dma_dev_t *dev, uint8_t *dma_buffer, size_t dma_buffer_size,
                   uint8_t *ring_buffer, size_t ring_buffer_size, event_type_t *rx_event)
{
    if (uart_dma == NULL || dev == NULL || dma_buffer == NULL ||
        ring_buffer == NULL || rx_event == NULL || dma_buffer_size == 0 || ring_buffer_size == 0) 
    {
        return false;
    }

    uart_dma->dev = dev;

    uart_dma->buffer = dma_buffer;
    uart_dma->buffer_size = dma_buffer_size;

    ring_buffer_init(&uart_dma->rx_buffer, ring_buffer, dma_buffer_size, 1);

    uart_dma->last_pos = 0;

    uart_dma->rx_event = rx_event;

    return uart_dma->dev->ops->init(uart_dma->dev->context);
}

static volatile uint8_t uart_dma_ht_flag = 0;
static volatile uint8_t uart_dma_tc_flag = 0;
static volatile uint8_t uart_dma_idle_flag = 0;

static volatile uint8_t uart_dma_overflow_flag = 0;

void uart_dma_idle_irq(void *data)
{
    uart_dma_idle_flag = 1;
}

void uart_dma_ht_irq(void *data)
{
    uart_dma_ht_flag = 1;
}

void uart_dma_tc_irq(void *data)
{
    uart_dma_tc_flag = 1;
}

void uart_dma_process(uart_dma_t *uart_dma)
{
    if(uart_dma_idle_flag || uart_dma_ht_flag || uart_dma_tc_flag)
    {
        size_t cur_pos = uart_dma->dev->ops->get_pos(uart_dma->dev->context);
        size_t last_pos = uart_dma->last_pos;

        if(cur_pos > last_pos)
        {
            ring_buffer_write(&uart_dma->rx_buffer, &uart_dma->buffer[uart_dma->last_pos], cur_pos - uart_dma->last_pos);
        }
        else if(cur_pos < last_pos)
        {
            ring_buffer_write(&uart_dma->rx_buffer, &uart_dma->buffer[uart_dma->last_pos], uart_dma->buffer_size - last_pos);
            ring_buffer_write(&uart_dma->rx_buffer, &uart_dma->buffer[0], cur_pos);
        }

        uart_dma_idle_flag = 0;
        uart_dma_ht_flag = 0;
        uart_dma_tc_flag = 0;

        uart_dma->last_pos = cur_pos;

        event_publish(uart_dma->rx_event, NULL);
    }
}

size_t uart_dma_read(uart_dma_t *uart_dma, uint8_t *buffer, size_t len)
{
    return ring_buffer_read(&uart_dma->rx_buffer, buffer, len);
}
