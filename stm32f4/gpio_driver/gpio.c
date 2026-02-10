#include "gpio_hw.h"
#include <stdint.h>

/* Enable GPIO clock based on port */
static void gpio_enable_clock(GPIO_RegDef_t *port)
{
    if (port == GPIOA) {
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    } else if (port == GPIOB) {
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    } else if (port == GPIOC) {
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    } else if (port == GPIOD) {
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    } else if (port == GPIOE) {
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    } else if (port == GPIOF) {
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    } else if (port == GPIOG) {
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOGEN;
    } else if (port == GPIOH) {
        RCC_AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
    }
}

/* Initialize GPIO pin */
void gpio_init(GPIO_RegDef_t *port, uint8_t pin, uint8_t mode)
{
    /* 1. Enable clock */
    gpio_enable_clock(port);

    /* 2. Configure mode */
    port->MODER &= ~(0x3U << (2 * pin));     //clear the bits for pin 3
    port->MODER |=  (mode << (2 * pin));    //set the bits for pin 3 as genral purpose output (01)

    /* 3. Output settings (only meaningful if output) */
    if (mode == GPIO_MODE_OUTPUT) {
        port->OTYPER  &= ~(1U << pin);              // set the input mode to the push pull state at reset use this mode so that no extra regsiter is required
        port->OSPEEDR &= ~(0x3U << (2 * pin));
        port->OSPEEDR |=  (GPIO_SPEED_MED << (2 * pin));
    }
// OTYPER is used only for the output pins it do not have any effect on the input pins 
 // use open drain when the same bus shared by multiple devices like in i2c
 port->OSPEEDR &= ~(0b11 << (2 * 3));
 port->OSPEEDR |= (0b01 << (2 * 3));     // set the output speed of register to medium(01)

 port->PUPDR &= ~(0b11 << (2 * 2));
 port->PUPDR |= (0b00 << (2 * 2));       // set the input mode to no pull up-pull down (0)

}

void gpio_write(GPIO_RegDef_t *port, uint8_t pin, uint8_t value)
{
    port->BSRR = (1 << 3);     // to turn on the led 
}

void gpio_reset(GPIO_RegDef_t *port, uint8_t pin)
{
    port->BSRR = (1U << (pin + 16));   // reset pin
}


uint32_t gpio_read(GPIO_RegDef_t *port, uint8_t pin);
{
    uint32_t value = GPIOA_IDR;
    return (value & (1 << 2)) ? 1 : 0;
}

