#ifndef __OLED_H__
#define __OLED_H__

#include "i2c.h"
#include "OLED_D.h"
#include "MPU6050.h"

void OLED_Init();
void OLED_ShowString(uint8_t Line, uint8_t Column, uint8_t *String);
void Task_OLED_ShowData();

#endif