#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

void gpio_init(GPIO_RegDef_t *port, uint8_t pin, uint8_t mode);
void gpio_write(GPIO_RegDef_t *port, uint8_t pin, uint8_t value);
uint8_t gpio_read(GPIO_RegDef_t *port, uint8_t pin);

#endif
