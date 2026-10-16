#include "main.h"
#include "UART.h"
#include "OLED.h"
#include "MPU6050.h"

void Task_Init(void *argument)
{
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, ReceiveBuf, UART_BUFFER_LENGTH);

    OLED_Init();

    MPU6050_Init();
    MPU6050_Zero();

    OLED_ShowString(1, 1, "IMU Attitude");
    OLED_ShowString(2, 1, "Roll:");
    OLED_ShowString(3, 1, "Pitch:");
    OLED_ShowString(4, 1, "Yaw:");

    xTaskCreate(Task_UART_Send, "UART_Send", 128, NULL, osPriorityNormal, NULL);
    xTaskCreate(Task_MPU6050_DataUpdate, "MPU6050_Read", 128, NULL, osPriorityHigh, NULL);
    xTaskCreate(Task_MPU6050_DataHandle, "MPU6050_Handle", 128, NULL, osPriorityHigh, NULL);
    xTaskCreate(Task_OLED_ShowData, "OLED_ShowData", 128, NULL, osPriorityLow, NULL);

    vTaskDelete(NULL);
}
