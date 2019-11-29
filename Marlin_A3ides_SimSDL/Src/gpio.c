// gpio.c (fake source for simulator)

#include "gpio.h"

GPIO_TypeDef* gpio_port(uint8_t pin8)
{
    return 0;
}

int gpio_get(uint8_t pin8)
{
    return 0;
}

void gpio_set(uint8_t pin8, int state)
{
}

void gpio_init(uint8_t pin8, uint32_t mode, uint32_t pull, uint32_t speed)
{
}
