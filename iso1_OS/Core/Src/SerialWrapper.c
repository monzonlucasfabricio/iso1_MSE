#include <string.h>

#include "SerialWrapper.h"
#include "usart.h"

void uartWriteByte( UART_HandleTypeDef *huart, const uint8_t value)
{
	uint8_t val = value;
	HAL_UART_Transmit(huart, &val, 1, HAL_MAX_DELAY);
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
	uint32_t size = sizeof(buffer);
	for (uint32_t i = 0; i < size; i++)
	{
		uartWriteByteArray(&huart2,(uint8_t* )&buffer[i], sizeof(buffer[0]));
	}
}

