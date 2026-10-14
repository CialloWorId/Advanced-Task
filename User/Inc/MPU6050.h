#ifndef __MPU6050_H__
#define __MPU6050_H__

#include "i2c.h"
#include "math.h"

#define MPU6050_DEVADDRESS 0x68
#define PI 3.1415926f
#define DT  0.005f
#define DT_MS 5

void MPU6050_Init();
void MPU6050_Zero();

#endif