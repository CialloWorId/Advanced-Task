#include "main.h"
#include "UART.h"
#include "OLED.h"
#include "MPU6050.h"

/**
 * @brief  初始化任务
 * @note   打开串口DMA、初始化MPU6050和OLED屏幕、对MPU6050的陀螺仪调零并创建工作任务
 * @note   CubeMX自动生成的默认任务，任务优先级48，防止被打断
 * @param  无
 * @retval 无
 */
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
