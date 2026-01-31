#include "gpio_hw.h"
#include "gpio.h"
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
 GPIOA_OSPEEDR &= ~(0b11 << (2 * 3));
 GPIOA_OSPEEDR |= (0b01 << (2 * 3));     // set the output speed of register to medium(01)

 GPIOA_PUPDR &= ~(0b11 << (2 * 2));
 GPIOA_PUPDR |= (0b00 << (2 * 2));       // set the input mode to no pull up-pull down (0)

}

void set_gpio_pin(){
    GPIOA_BSRR = (1 << 3);     // to turn on the led 
}

void reset_gpio_pin(){
     GPIOA_BSRR = (1 << (3 + 16));      // to turn off the led 

}

uint32_t gpio_read_pin(void)
{
    uint32_t value = GPIOA_IDR;
    return (value & (1 << 2)) ? 1 : 0;
}


#include "gpio_hw.h"
#include <stdint.h>

void gpio_init(void) {
    // Enable GPIOA clock on AHB1
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // Configure PA3 as output (General Purpose Output)
    GPIOA_MODER &= ~(0b11 << 6);      // Clear PA3 MODER bits [7:6]
    GPIOA_MODER |= (0b01 << 6);       // Set PA3 to output mode (01)

    // Configure PA2 as input (default, already 00 after reset)
    GPIOA_MODER &= ~(0b11 << 4);      // Clear PA2 MODER bits [5:4]
    GPIOA_MODER |= (0b00 << 4);       // Set PA2 to input mode (00)

    // PA3: Push-pull output type (default at reset, but explicit)
    GPIOA_OTYPER &= ~(1 << 3);        // Push-pull for PA3

    // PA3: Medium speed output
    GPIOA_OSPEEDR &= ~(0b11 << 6);    // Clear PA3 OSPEEDR bits
    GPIOA_OSPEEDR |= (0b01 << 6);     // Medium speed (01)

    // PA2: No pull-up/pull-down (default for input button)
    GPIOA_PUPDR &= ~(0b11 << 4);      // Clear PA2 PUPDR bits
    GPIOA_PUPDR |= (0b00 << 4);       // No pull (00)
}

void set_gpio_pin(void) {
    GPIOA_BSRR = (1 << 3);            // Set PA3 high (LED on)
}

void reset_gpio_pin(void) {
    GPIOA_BSRR = (1 << 19);           // Reset PA3 low (LED off) - bit (3 + 16)
}

while(1) {
    if (!gpio_read_pin()) {  // Button pressed (active LOW)
        set_gpio_pin();      // Turn on LED
    } else {
        reset_gpio_pin();    // Turn off LED
    }
}