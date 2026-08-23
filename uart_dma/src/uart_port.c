#include "uart_port.h"
#include "uart_dma.h"

void UART3_Init(void *context)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	USART_InitTypeDef USART_InitStructure;					//定义结构体变量
	USART_InitStructure.USART_BaudRate = 115200;				//波特率
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;	//硬件流控制，不需要
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;	//模式，发送模式和接收模式均选择
	USART_InitStructure.USART_Parity = USART_Parity_No;		//奇偶校验，不需要
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//停止位，选择1位
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		//字长，选择8位
	USART_Init(USART3, &USART_InitStructure);	
	
	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);			//开启串口接收数据的中断

	NVIC_InitTypeDef NVIC_InitStructure;					
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;		
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;		
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		
	NVIC_Init(&NVIC_InitStructure);							

	USART_Cmd(USART3, ENABLE);
}

uint8_t rxBuffer[RX_BUF_SIZE];

void DMA_RX_Config(void *context)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_InitTypeDef DMA_InitStruct = {0};
    DMA_DeInit(DMA1_Channel3);                                // USART3_RX -> 通道3

    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&(USART3->DR);  // 不是 USART1
    DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)rxBuffer;
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStruct.DMA_BufferSize = RX_BUF_SIZE;
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStruct.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel3, &DMA_InitStruct);
	
	DMA_ITConfig(DMA1_Channel3, DMA_IT_TC | DMA_IT_HT, ENABLE);
	
	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannel = DMA1_Channel3_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStruct);
	
    USART_DMACmd(USART3, USART_DMAReq_Rx, ENABLE);
    DMA_Cmd(DMA1_Channel3, ENABLE);	
}


size_t dma_get_pos(void *contex)
{
    return RX_BUF_SIZE - DMA_GetCurrDataCounter(((stm32_uart_dma_context_t *)contex)->DMAy_Channelx);
}

bool dma_start(void *context, uint8_t *buffer, size_t length)
{
    DMA_Cmd(((stm32_uart_dma_context_t *)context)->DMAy_Channelx, DISABLE);

    ((stm32_uart_dma_context_t *)context)->DMAy_Channelx->CMAR = (uint32_t)buffer;
    ((stm32_uart_dma_context_t *)context)->DMAy_Channelx->CNDTR = length;

    DMA_Cmd(((stm32_uart_dma_context_t *)context)->DMAy_Channelx, ENABLE);
	
	return true;
}

static stm32_uart_dma_context_t uart_dma_context = {
	.DMAy_Channelx = DMA1_Channel3,
	.USARTx = USART3
};

static uart_dma_ops_t uart_dma_ops = {
	.get_pos = dma_get_pos,
	.init = uart_dma_port_init,
};

uart_dma_dev_t uart_dma_dev = {
    .context = &uart_dma_context,
    .ops = &uart_dma_ops
};

bool uart_dma_port_init(void *data)
{
	UART3_Init(data);
    DMA_RX_Config(data);
	
	return true;
}

extern uart_dma_t uart_dma;

void DMA1_Channel3_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC3) != RESET) {
        DMA_ClearITPendingBit(DMA1_IT_TC3);
        uart_dma_tc_irq(&uart_dma);
    }

    if (DMA_GetITStatus(DMA1_IT_HT3) != RESET) {
        DMA_ClearITPendingBit(DMA1_IT_HT3);
        uart_dma_ht_irq(&uart_dma);
    }

}

void USART3_IRQHandler(void) 
{
    if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET) {
        uart_dma_idle_irq(&uart_dma);
        USART_ReceiveData(USART3); // 清除空闲中断标志
    }
}

