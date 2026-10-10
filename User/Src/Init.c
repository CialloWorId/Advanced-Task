#include "main.h"
#include "UART.h"
#include "OLED.h"

void Task_Init(void *argument)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, ReceiveBuf, UART_BUFFER_LENGTH);
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

    OLED_Init();
    OLED_ShowString(1, 1, "IMU Attitude");
    OLED_ShowString(2, 1, "Roll:");
    OLED_ShowString(3, 1, "Pitch:");
    OLED_ShowString(4, 1, "Yaw:");

    vTaskDelete(NULL);
}
