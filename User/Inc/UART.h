#ifndef __UART_H__
#define __UART_H__

#include "usart.h"
#include "string.h"

#define UART_BUFFER_LENGTH 64   //接收缓冲区长度
extern uint8_t ReceiveBuf[UART_BUFFER_LENGTH];

void Task_UART_Send(void *argument);

#endif
