#include "GPIOWrapper.h"
#include "main.h"

void gpioSetLevel(uint16_t pin, uint32_t port, bool value)
{
    HAL_GPIO_WritePin( (GPIO_TypeDef*)port, pin, (GPIO_PinState)value);
}


bool gpioGetLevel(uint16_t pin, uint32_t port)
{
    return HAL_GPIO_ReadPin((GPIO_TypeDef*)port, pin);
}
