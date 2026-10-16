#include "main.h"
#include "UART.h"

uint8_t ReceiveBuf[UART_BUFFER_LENGTH] = "";    //接收缓冲区
uint8_t ReceiveBufPointer = 0;                  //接收数据在缓冲区内的起始位置指针
uint8_t SendBuf[UART_BUFFER_LENGTH] = "";       //发送数据

/**
 * @brief  串口回显任务
 * @note   任务优先级24，从队列UART_Receive_Size读取到接收到的数据长度后开始运行
 * @note   任务中以换行符\\n验证数据完整性，请在vofa发送区中选择追加\\n
 * @param  无
 * @retval 无
 */
void Task_UART_Send(void *argument)
{
    uint16_t Size;
    for(;;)
    {
        xQueueReceive(UART_Receive_SizeHandle, &Size, portMAX_DELAY);
        xSemaphoreTake(UART_DMA_LockHandle, portMAX_DELAY);     //等待上次UART DMA发送完成，并在下次UART DMA发送前上锁
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
            xSemaphoreGive(UART_DMA_LockHandle);    //若未检测到\n判断未接收完成，解锁继续接收
        }
    }
}

/**
 * @brief  UART事件中断回调函数
 * @note   检测到USART1的空闲事件或传输完成事件时通过队列UART_Receive_Size通知串口回显任务接收完成
 * @param  huart 触发回调的UART外设句柄指针
 * @param  Size 接收缓冲区填充的数据总长度
 * @retval 无
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1 && (huart->RxEventType == HAL_UART_RXEVENT_IDLE || huart->RxEventType == HAL_UART_RXEVENT_TC))
    { 
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendToBackFromISR(UART_Receive_SizeHandle, &Size, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief  UART DMA发送完成解锁
 * @param  huart UART外设句柄指针
 * @retval 无
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(UART_DMA_LockHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}