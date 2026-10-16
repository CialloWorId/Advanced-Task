#include "main.h"
#include "MPU6050.h"

#define MPU6050_DEVADDRESS 0x68     //从机地址
#define PI 3.1415926f               //圆周率
#define DT  0.005f                  //采样间隔（s）
#define DT_MS (uint8_t)(1000 * DT)  //采样间隔（ms）

/**
 *@brief  原始数据
 *@param  ax x轴加速度
 *@param  ay y轴加速度
 *@param  az z轴加速度
 *@param  gx x轴角速度
 *@param  gy y轴角速度
 *@param  gz z轴角速度
 */
struct RawStruct
{
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
} RawData;

/**
 * @brief  角速度和
 * @note   初始化任务中采样1000次角速度求和，用来求陀螺仪零偏
 * @param  gxSum x轴角速度和
 * @param  gySum y轴角速度和
 * @param  gzSum z轴角速度和
 */
struct SumStruct
{
    float gxSum;
    float gySum;
    float gzSum;
} AngleSum = {0};

/** 
 * @brief  卡尔曼滤波涉及的参数
 * @param  ObservedAngle 加速度计解算出的角度
 * @param  ZeroBias 陀螺仪零偏
 * @param  RatelNoise 陀螺仪角速度过程噪声协方差（Q矩阵元素）
 * @param  BiasNoise 陀螺仪零偏过程噪声协方差（Q矩阵元素）
 * @param  MeasureNoise 加速度计测量噪声协方差（R）
 * @param  K[2] 卡尔曼增益
 * @param  P[2][2] 状态估计误差协方差矩阵
 */
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

AngleStruct EulerAngles =
{
    .Roll = 0,
    .Pitch = 0,
    .Yaw = 0
};

float YawZeroBias;  //Yaw角角速度零偏

/**
 * @brief  向MPU6050的指定寄存器写数据
 * @param  Register 寄存器地址
 * @param  Data 向寄存器写入的数据
 * @retval 无
 */
void MPU6050_WriteRegister(uint8_t Register, uint8_t Data)
{
    HAL_I2C_Mem_Write(&hi2c2, (MPU6050_DEVADDRESS << 1), Register, 1, &Data, 1, HAL_MAX_DELAY);
}

/**
 * @brief  从MPU6050中读取陀螺仪和加速度计的原始数据
 * @param  无
 * @retval 无
 */
void MPU6050_ReadData()
{
    uint8_t RawDataString[14];
    xSemaphoreTake(I2C2_IT_LockHandle, pdMS_TO_TICKS(0));   //I2C中断读取前上锁
    HAL_I2C_Mem_Read_IT(&hi2c2, (MPU6050_DEVADDRESS << 1), 0x3B, 1, RawDataString, 14);
    xSemaphoreTake(I2C2_IT_LockHandle, portMAX_DELAY);      //等待I2C中断读取完成
    RawData.ax = (int16_t)((RawDataString[0] << 8) | RawDataString[1]) * 6.1035e-5f; 
    RawData.ay = (int16_t)((RawDataString[2] << 8) | RawDataString[3]) * 6.1035e-5f; 
    RawData.az = (int16_t)((RawDataString[4] << 8) | RawDataString[5]) * 6.1035e-5f; 
    RawData.gx = (int16_t)((RawDataString[8] << 8) | RawDataString[9]) * 6.1035e-2f; 
    RawData.gy = (int16_t)((RawDataString[10] << 8) | RawDataString[11]) * 6.1035e-2f;
    RawData.gz = (int16_t)((RawDataString[12] << 8) | RawDataString[13]) * 6.1035e-2f;
}

/**
 * @brief  I2C中断读取完成解锁
 * @param  hi2c I2C外设句柄指针
 * @retval 无
 */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(I2C2_IT_LockHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief  MPU6050初始化
 * @param  无
 * @retval 无
 */
void MPU6050_Init()
{
    MPU6050_WriteRegister(0x6B, 0x80);  //复位
    HAL_Delay(100);                     //等待复位完成
    MPU6050_WriteRegister(0x6B, 0x00);  //关闭睡眠模式
    MPU6050_WriteRegister(0x1B, 0x18);  //设置陀螺仪量程为正负2000度每秒
    MPU6050_WriteRegister(0x1C, 0x00);  //设置加速度计量程为正负2g
}

/**
 * @brief  MPU6050陀螺仪调零
 * @note   初始化任务中采样1000次角速度求平均值作为陀螺仪零偏
 * @param  无
 * @retval 无
 */
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

/**
 * @brief  读取MPU6050更新数据任务
 * @note   采样率200Hz，任务优先级40
 * @param  无
 * @retval 无
 */
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

/**
 * @brief  卡尔曼滤波器
 * @note   二维状态的卡尔曼滤波器，用加速度计解算出的欧拉角修正角速度积分出的欧拉角
 * @param  KalmanData 对应欧拉角的卡尔曼参数
 * @param  LastAngle 上次采样解算出的欧拉角
 * @param  Rate 本次采样的该欧拉角的角速度
 * @retval 本次解算出的欧拉角
 */
float MPU6050_KalmanUpdate(KalmanStruct *KalmanData, float LastAngle, float Rate)
{
    float NewAngle;
    /* 状态预测 (先验估计) */
    NewAngle = LastAngle + (Rate - KalmanData->ZeroBias) * DT;
    /* 误差协方差预测 (先验协方差) */
    KalmanData->P[0][0] += DT * (DT * KalmanData->P[1][1] - KalmanData->P[0][1] - KalmanData->P[1][0] + KalmanData->RatelNoise);
    KalmanData->P[0][1] -= DT * KalmanData->P[1][1];
    KalmanData->P[1][0] -= DT * KalmanData->P[1][1];
    KalmanData->P[1][1] += KalmanData->BiasNoise * DT;
    /* 计算卡尔曼增益 */
    KalmanData->K[0] = KalmanData->P[0][0] / (KalmanData->P[0][0] + KalmanData->MeasureNoise);
    KalmanData->K[1] = KalmanData->P[1][0] / (KalmanData->P[0][0] + KalmanData->MeasureNoise);
    /* 状态更新 (后验估计) */
    NewAngle += KalmanData->K[0] * (KalmanData->ObservedAngle - LastAngle);
    KalmanData->ZeroBias += KalmanData->K[1] * (KalmanData->ObservedAngle - NewAngle);
    /* 误差协方差更新 (后验协方差) */
    float P00 = KalmanData->P[0][0];
    float P01 = KalmanData->P[0][1];
    KalmanData->P[0][0] -= KalmanData->K[0] * P00;
    KalmanData->P[0][1] -= KalmanData->K[0] * P01;
    KalmanData->P[1][0] -= KalmanData->K[1] * P00;
    KalmanData->P[1][1] -= KalmanData->K[1] * P01;

    return NewAngle;
}

/**
 * @brief  MPU6050原始数据处理任务
 * @note   任务优先级40，接收到读取MPU6050更新数据任务发送的RawDataReady信号量开始运行
 * @param  无
 * @retval 无
 */
void Task_MPU6050_DataHandle(void *argument)
{
    for(;;)
    {
        xSemaphoreTake(RawDataReadyHandle, portMAX_DELAY);

        RollKalman.ObservedAngle = atan2f(RawData.ay, RawData.az) * 180.0 / PI;
        PitchKalman.ObservedAngle = -atan2f(RawData.ax, sqrtf(RawData.ay * RawData.ay + RawData.az * RawData.az)) * 180.0 / PI;

        EulerAngles.Roll = MPU6050_KalmanUpdate(&RollKalman, EulerAngles.Roll, RawData.gx);
        EulerAngles.Pitch = MPU6050_KalmanUpdate(&PitchKalman, EulerAngles.Pitch, RawData.gy);
        EulerAngles.Yaw += (RawData.gz - YawZeroBias) * DT;
    }
}
