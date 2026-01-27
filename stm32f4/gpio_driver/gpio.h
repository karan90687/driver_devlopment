#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

void gpio_init(void);
void set_gpio_pin(void);
void reset_gpio_pin(void);
uint32_t gpio_read_pin(void);

#endif
