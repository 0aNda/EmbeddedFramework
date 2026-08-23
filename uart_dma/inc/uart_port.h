#ifndef __UATT_PORT_H
#define __UATT_PORT_H

#include "stm32f10x.h"


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "uart_dma.h"

#define RX_BUF_SIZE 512
extern uint8_t rxBuffer[RX_BUF_SIZE];

typedef struct {
    USART_TypeDef *USARTx;
    DMA_Channel_TypeDef *DMAy_Channelx;
} stm32_uart_dma_context_t;

extern uart_dma_dev_t uart_dma_dev;

bool uart_dma_port_init(void *data);
size_t dma_get_pos(void *contex);
bool dma_start(void *context, uint8_t *buffer, size_t length);

#endif
