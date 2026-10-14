#ifndef __OLED_H__
#define __OLED_H__

#include "i2c.h"
#include "OLED_D.h"

#define OLED_DEVADDRESS 0x3C

void OLED_Init();
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);

#endif