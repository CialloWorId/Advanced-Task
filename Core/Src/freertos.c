/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for Init */
osThreadId_t InitHandle;
const osThreadAttr_t Init_attributes = {
  .name = "Init",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for UART_Send */
osThreadId_t UART_SendHandle;
const osThreadAttr_t UART_Send_attributes = {
  .name = "UART_Send",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UART_Receive_Size */
osMessageQueueId_t UART_Receive_SizeHandle;
const osMessageQueueAttr_t UART_Receive_Size_attributes = {
  .name = "UART_Receive_Size"
};
/* Definitions for UART_DMA_Lock */
osSemaphoreId_t UART_DMA_LockHandle;
const osSemaphoreAttr_t UART_DMA_Lock_attributes = {
  .name = "UART_DMA_Lock"
};
/* Definitions for I2C1_DMA_Lock */
osSemaphoreId_t I2C1_DMA_LockHandle;
const osSemaphoreAttr_t I2C1_DMA_Lock_attributes = {
  .name = "I2C1_DMA_Lock"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Task_Init(void *argument);
void Task_UART_Send(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of UART_DMA_Lock */
  UART_DMA_LockHandle = osSemaphoreNew(1, 1, &UART_DMA_Lock_attributes);

  /* creation of I2C1_DMA_Lock */
  I2C1_DMA_LockHandle = osSemaphoreNew(1, 1, &I2C1_DMA_Lock_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of UART_Receive_Size */
  UART_Receive_SizeHandle = osMessageQueueNew (8, sizeof(uint16_t), &UART_Receive_Size_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Init */
  InitHandle = osThreadNew(Task_Init, NULL, &Init_attributes);

  /* creation of UART_Send */
  UART_SendHandle = osThreadNew(Task_UART_Send, NULL, &UART_Send_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_Task_Init */
/**
  * @brief  Function implementing the Init thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Task_Init */
__weak void Task_Init(void *argument)
{
  /* USER CODE BEGIN Task_Init */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_Init */
}

/* USER CODE BEGIN Header_Task_UART_Send */
/**
* @brief Function implementing the UART_Send thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Task_UART_Send */
__weak void Task_UART_Send(void *argument)
{
  /* USER CODE BEGIN Task_UART_Send */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Task_UART_Send */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

