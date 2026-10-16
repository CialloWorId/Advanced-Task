#include "main.h"
#include "OLED.h"

#define OLED_DEVADDRESS 0x3C	//从机地址
#define OLED_REFRESHRATE 50		//屏幕刷新率

 /**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
    static uint8_t Command_Package[2] = {0x00, 0x00};
    xSemaphoreTake(I2C1_DMA_LockHandle, portMAX_DELAY);		//等待上次I2C DMA发送完成，并在下次I2C DMA发送前上锁
    Command_Package[1] = Command;
    HAL_I2C_Master_Transmit_DMA(&hi2c1, (OLED_DEVADDRESS << 1), Command_Package, 2);
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
    static uint8_t Data_Package[2] = {0x40, 0x00};
    xSemaphoreTake(I2C1_DMA_LockHandle, portMAX_DELAY);		//等待上次I2C DMA发送完成，并在下次I2C DMA发送前上锁
    Data_Package[1] = Data;
    HAL_I2C_Master_Transmit_DMA(&hi2c1, (OLED_DEVADDRESS << 1), Data_Package, 2);
}

/**
 * @brief  I2C DMA发送完成解锁
 * @param  hi2c I2C外设句柄指针
 * @retval 无
 */
void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C1)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(I2C1_DMA_LockHandle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0-7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0-127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	for (uint8_t j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(uint8_t i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1-4
  * @param  Column 列位置，范围：1-16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, uint8_t Char)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1-4
  * @param  Column 起始列位置，范围：1-16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, uint8_t *String)
{
	for (uint8_t i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
 * @brief  OLED显示欧拉角
 * @param  Line 要显示的欧拉角的行位置，范围：2-4
 * @param  Number 要显示的欧拉角
 * @retval 无
 */
void OLED_ShowAngle(uint8_t Line, float Number)
{
	float absNumber;
	uint8_t ShowString[9] = "        ";
	OLED_ShowString(Line, 9, ShowString);
	uint8_t Temp, i = 7;

	if (Number < 0)
	{
		absNumber = -Number;
	}
	else
	{
		absNumber = Number;
	}

	Temp = (uint8_t)((absNumber - (uint8_t)absNumber) * 1000);
	if (Temp == 0)
	{
		ShowString[7] = '0';
		ShowString[6] = '0';
		ShowString[5] = '0';
		i = 4;
	}
	while (Temp)
	{
		ShowString[i] = Temp % 10 + 48;
		Temp /= 10;
		i--;
	}

	ShowString[i] = '.';
	i--;

	Temp = (uint8_t)absNumber;
	if (Temp == 0)
	{
		ShowString[i] = '0';
		i--;
	}
	while (Temp)
	{
		ShowString[i] = Temp % 10 + 48;
		Temp /= 10;
		i--;
	}

	if (Number > 0)
	{
		ShowString[i] = '+';
		i--;
	}
	if (Number < 0)
	{
		ShowString[i] = '-';
		i--;
	}
	OLED_ShowString(Line, 9, ShowString);
}

/**
 * @brief  OLED屏幕刷新欧拉角数据任务
 * @note   刷新率50Hz，任务优先级8
 * @param  无
 * @retval 无
 */
void Task_OLED_ShowData()
{
	float Roll, Pitch, Yaw;
	portTickType pxPreviousWakeTime = xTaskGetTickCount();
	for(;;)
	{
		Roll = EulerAngles.Roll;
		Pitch = EulerAngles.Pitch;
		Yaw = EulerAngles.Yaw;
		
		OLED_ShowAngle(2, Roll);
		OLED_ShowAngle(3, Pitch);
		OLED_ShowAngle(4, Yaw);

		vTaskDelayUntil(&pxPreviousWakeTime, pdMS_TO_TICKS(1000 / OLED_REFRESHRATE));
	}
}

/**
  *@brief  OLED初始化
  *@param  无
  *@retval 无
  */
void OLED_Init()
{
	HAL_Delay(100);				//上电延时
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}
