#include "main.h"
#include "UART.h"

void Task_Init(void *argument)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, ReceiveBuf, UART_BUFFER_LENGTH);
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
    vTaskDelete(NULL);
}
