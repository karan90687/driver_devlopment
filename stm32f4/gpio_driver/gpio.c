#include "gpio_hw.h"
#include <stdint.h>
#include <gpio.h>


void gpio_init(){
    // enable the clock first for the register first 
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // setting the mode of the pin 
 GPIOA_MODER &= ~(0b11 << (2 * 3));        //clear the bits for pin 3
 GPIOA_MODER |= (0b01 << (2 * 3));         //set the bits for pin 3 as genral purpose output 

 GPIOA_MODER &= ~(0b11 << (2 * 2));        //clear the bits for pin 2
 GPIOA_MODER |= (0b00 << (2 * 2));        //set the bits for pin 2 as general purpose input 

GPIOA_OTYPER &= ~(1 << 2);        // set the input mode to the push pull state at reset use this mode so that no extra regsiter is required

 // use open drain when the same bus shared by multiple devices like in i2c
 GPIOA_OSPEEDR &= ~(0b11 << (2 * 3));
 GPIOA_OSPEEDR |= (0b01 << (2 * 3));     // set the output speed of register to medium 

 GPIOA_PUPDR &= ~(0b11 << (2 * 2));
 GPIOA_PUPDR |= (0b00 << (2 * 2));       // set the input mode to no pull up-pull down

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