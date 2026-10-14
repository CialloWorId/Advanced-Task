#include "main.h"
#include "MPU6050.h"
#include "usart.h"

struct DataStruct
{
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} RawData;

struct AngleStruct
{
    float Roll;
    float Pitch;
    float Yaw;
    uint8_t Tail[4];
} EulerAngles = {.Roll = 0, .Pitch = 0, .Yaw = 0, .Tail = {0x00, 0x00, 0x80, 0x7f}};

struct SumStruct
{
    float gxSum;
    float gySum;
    float gzSum;
} AngleSum = {0};


typedef struct
{
    float ObservedAngle;
    float ZeroBias;
    float RatelNoise;
    float BiasNoise;
    float MeasureNoise;
    float K[2];
    float P[2][2];
} KalmanStruct;

KalmanStruct RollKalman =
{
    .RatelNoise = 0.001f,
    .BiasNoise = 0.003f,
    .MeasureNoise = 0.5f,
    .P = {1.0f, 0.0f, 0.0f, 0.1f}
};

KalmanStruct PitchKalman =
{
    .RatelNoise = 0.001f,
    .BiasNoise = 0.003f,
    .MeasureNoise = 0.5f,
    .P = {1.0f, 0.0f, 0.0f, 0.1f}
};

float YawZeroBias;

void MPU6050_WriteRegister(uint8_t Register, uint8_t Data)
{
    HAL_I2C_Mem_Write(&hi2c2, (MPU6050_DEVADDRESS << 1), Register, 1, &Data, 1, HAL_MAX_DELAY);
}

void MPU6050_ReadData()
{
    uint8_t RawDataString[14];
    xSemaphoreTake(I2C2_IT_LockHandle, pdMS_TO_TICKS(0));
    HAL_I2C_Mem_Read_IT(&hi2c2, (MPU6050_DEVADDRESS << 1), 0x3B, 1, RawDataString, 14);
    xSemaphoreTake(I2C2_IT_LockHandle, portMAX_DELAY);
    RawData.ax = (int16_t)((RawDataString[0] << 8) | RawDataString[1]) * 6.1035e-5f; 
    RawData.ay = (int16_t)((RawDataString[2] << 8) | RawDataString[3]) * 6.1035e-5f; 
    RawData.az = (int16_t)((RawDataString[4] << 8) | RawDataString[5]) * 6.1035e-5f; 
    RawData.gx = (int16_t)((RawDataString[8] << 8) | RawDataString[9]) * 6.1035e-2f; 
    RawData.gy = (int16_t)((RawDataString[10] << 8) | RawDataString[11]) * 6.1035e-2f;
    RawData.gz = (int16_t)((RawDataString[12] << 8) | RawDataString[13]) * 6.1035e-2f;
}


void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(I2C2_IT_LockHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

void MPU6050_Init()
{
    MPU6050_WriteRegister(0x6B, 0x80);  //复位
    HAL_Delay(100);                     //等待复位完成
    MPU6050_WriteRegister(0x6B, 0x00);  //关闭睡眠模式
    MPU6050_WriteRegister(0x1B, 0x18);  //设置陀螺仪量程为正负2000度每秒
    MPU6050_WriteRegister(0x1C, 0x00);  //设置加速度计量程为正负2g
}

void MPU6050_Zero()
{
    for (uint16_t i = 0; i < 1000; i++)
    {
        MPU6050_ReadData();
        AngleSum.gxSum += RawData.gx;
        AngleSum.gySum += RawData.gy;
        AngleSum.gzSum += RawData.gz;
        HAL_Delay(DT_MS);
    }
    RollKalman.ZeroBias = AngleSum.gxSum / 1000.0f;
    PitchKalman.ZeroBias = AngleSum.gySum / 1000.0f;
    YawZeroBias = AngleSum.gzSum / 1000.0f;
}

void Task_MPU6050_DataUpdate(void *argument)
{
    portTickType pxPreviousWakeTime = xTaskGetTickCount();
    for(;;)
    {
        MPU6050_ReadData();
        xSemaphoreGive(RawDataReadyHandle);
        vTaskDelayUntil(&pxPreviousWakeTime, pdMS_TO_TICKS(DT_MS));
    }
}

float MPU6050_KalmanUpdate(KalmanStruct *KalmanData, float LastAngle, float Rate)
{
    float NewAngle;
    /* 预测步 */
    NewAngle = LastAngle + (Rate - KalmanData->ZeroBias) * DT;

    KalmanData->P[0][0] += DT * (DT * KalmanData->P[1][1] - KalmanData->P[0][1] - KalmanData->P[1][0] + KalmanData->RatelNoise);
    KalmanData->P[0][1] -= DT * KalmanData->P[1][1];
    KalmanData->P[1][0] -= DT * KalmanData->P[1][1];
    KalmanData->P[1][1] += KalmanData->BiasNoise * DT;

    /* 更新步 */
    KalmanData->K[0] = KalmanData->P[0][0] / (KalmanData->P[0][0] + KalmanData->MeasureNoise);
    KalmanData->K[1] = KalmanData->P[1][0] / (KalmanData->P[0][0] + KalmanData->MeasureNoise);

    NewAngle += KalmanData->K[0] * (KalmanData->ObservedAngle - LastAngle);
    KalmanData->ZeroBias += KalmanData->K[1] * (KalmanData->ObservedAngle - LastAngle);

    float P00 = KalmanData->P[0][0];
    float P01 = KalmanData->P[0][1];
    KalmanData->P[0][0] -= KalmanData->K[0] * P00;
    KalmanData->P[0][1] -= KalmanData->K[0] * P01;
    KalmanData->P[1][0] -= KalmanData->K[1] * P00;
    KalmanData->P[1][1] -= KalmanData->K[1] * P01;

    return NewAngle;
}

void Task_MPU6050_DataHandle(void *argument)
{
    for(;;)
    {
        xSemaphoreTake(RawDataReadyHandle, portMAX_DELAY);

        RollKalman.ObservedAngle = atan2f(RawData.ay, RawData.az) * 180.0 / PI;
        PitchKalman.ObservedAngle = atan2f(RawData.ax, sqrtf(RawData.ay * RawData.ay + RawData.az * RawData.az)) * 180.0 / PI;

        EulerAngles.Roll = MPU6050_KalmanUpdate(&RollKalman, EulerAngles.Roll, RawData.gx);
        EulerAngles.Pitch = MPU6050_KalmanUpdate(&PitchKalman, EulerAngles.Pitch, RawData.gy);
        EulerAngles.Yaw += (RawData.gz - YawZeroBias) * DT;

        HAL_UART_Transmit(&huart1, (uint8_t *)&EulerAngles, sizeof(EulerAngles), HAL_MAX_DELAY);
    }
}
