#include "main.h"
#include "UART.h"

uint8_t ReceiveBuf[UART_BUFFER_LENGTH] = "";
uint8_t ReceiveBufPointer = 0;
uint8_t SendBuf[UART_BUFFER_LENGTH] = "";

void Task_UART_Send(void *argument)
{
    uint16_t Size;
    for(;;)
    {
        xQueueReceive(UART_Receive_SizeHandle, &Size, portMAX_DELAY);
        xSemaphoreTake(UART_DMA_LockHandle, portMAX_DELAY);
        if (ReceiveBuf[Size - 1] == '\n')
        {
            if (Size > ReceiveBufPointer)
            {
                Size -= ReceiveBufPointer;
            }
            else
            {
                Size += (UART_BUFFER_LENGTH - ReceiveBufPointer);
            }
            for (uint8_t i = 0; i < Size; i++)
            {
                SendBuf[i] = ReceiveBuf[ReceiveBufPointer];
                ReceiveBufPointer++;
                if (ReceiveBufPointer == UART_BUFFER_LENGTH)
                {
                    ReceiveBufPointer = 0;
                }
            }
            HAL_UART_Transmit_DMA(&huart1, (uint8_t *)SendBuf, Size);
        }
        else
        {
            xSemaphoreGive(UART_DMA_LockHandle);
        }
    }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendToBackFromISR(UART_Receive_SizeHandle, &Size, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(UART_DMA_LockHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}