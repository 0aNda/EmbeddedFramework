#include "stm32f10x.h"                  // Device header

#include "uart_port.h"
#include "uart_dma.h"
#include "uart_port.h"

uart_dma_t uart_dma;

#define RING_BUFFER_MAX 1024
uint8_t ring_buffer[RING_BUFFER_MAX];

event_type_t event_dma_transfer_complete;

void protocol(void *data)
{
	//回传
	uint8_t byte = 0;
	while(uart_dma_read(&uart_dma, &byte, 1))
	{
		USART_SendData(USART3, byte);
		while(USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
	}
}

int main(void)
{	
	event_init(&event_dma_transfer_complete, "event_dma_transfer_complete");
	event_subscribe(&event_dma_transfer_complete, protocol);

	uart_dma_init(&uart_dma, &uart_dma_dev, rxBuffer, RX_BUF_SIZE, 
		          ring_buffer, RING_BUFFER_MAX, &event_dma_transfer_complete);

	while (1)
	{
		uart_dma_process(&uart_dma);
	}
}
