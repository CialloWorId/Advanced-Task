#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "i2c.h"
#include "math.h"

/**
 * @brief  欧拉角
 * @param  Roll 翻滚角
 * @param  Pitch 俯仰角
 * @param  Yaw 偏航角
 */
typedef struct
{
    float Roll;
    float Pitch;
    float Yaw;
} AngleStruct;

extern AngleStruct EulerAngles;

void MPU6050_Init();
void MPU6050_Zero();
void Task_MPU6050_DataUpdate(void *argument);
void Task_MPU6050_DataHandle(void *argument);

#endif