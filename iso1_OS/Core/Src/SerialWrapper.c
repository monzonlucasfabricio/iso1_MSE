#include <string.h>

#include "SerialWrapper.h"
#include "usart.h"
#include "osKernel.h"

extern UART_HandleTypeDef huart2;

void uartWriteByte( UART_HandleTypeDef *huart, const uint8_t value)
{
	uint8_t val = value;
	HAL_UART_Transmit(huart, &val, 1, MAX_DELAY);
}

void uartWriteByteArray( UART_HandleTypeDef *huart, char* byteArray, uint32_t byteArrayLen )
{
   uint32_t i = 0;
   for( i=0; i<byteArrayLen; i++ )
   {
	   uartWriteByte(huart, byteArray[i]);
   }
}

void serialPrint(char* buffer)
{
//	ENTER_CRITICAL_SECTION
	uartWriteByteArray(&huart2, buffer, strlen(buffer));
//	EXIT_CRITICAL_SECTION
}

