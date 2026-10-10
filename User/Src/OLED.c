#include "main.h"
#include "OLED.h"

void OLED_WriteCommand(uint8_t Command)
{
    uint8_t Send_Package[2] = {0x00, Command};
    HAL_I2C_Master_Transmit(&hi2c1, (DEV_ADDRESS << 1), Send_Package, 1, HAL_MAX_DELAY);
}



void OLED_Init()
{

}