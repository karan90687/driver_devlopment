#include <stdint.h>
#include <stdio.h>

// Base address are found in the memory map of the board in reference manual 
#define AHB1_PERIPH_BASE 0x40020000
#define APB2_PERIPH_BASE 0x40010000
#define APB1_PERIPH_BASE 0x40000000
#define RCC_PERIPH_BASE  0x40023800
// GPIO base address
#define GPIOA_BASE (AHB1_PERIPH_BASE + 0x0000)
#define GPIOB_BASE (AHB1_PERIPH_BASE + 0x0400)
#define GPIOC_BASE (AHB1_PERIPH_BASE + 0x0800)
#define GPIOD_BASE (AHB1_PERIPH_BASE + 0x0C00)
#define GPIOE_BASE (AHB1_PERIPH_BASE + 0x1000)
#define GPIOF_BASE (AHB1_PERIPH_BASE + 0x1400)
#define GPIOG_BASE (AHB1_PERIPH_BASE + 0x1800)
#define GPIOH_BASE (AHB1_PERIPH_BASE + 0x1C00)

// RCC_AHB1ENR bits to enable the GPIO ports
#define RCC_AHB1ENR_GPIOAEN (1 << 0)
#define RCC_AHB1ENR_GPIOBEN (1 << 1)
#define RCC_AHB1ENR_GPIOCEN (1 << 2)
#define RCC_AHB1ENR_GPIODEN (1 << 3)
#define RCC_AHB1ENR_GPIOEEN (1 << 4)
#define RCC_AHB1ENR_GPIOFEN (1 << 5)
#define RCC_AHB1ENR_GPIOGEN (1 << 6)
#define RCC_AHB1ENR_GPIOHEN (1 << 7)


// clock enable register for GPIO ports name RCC_AHB1ENR
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_PERIPH_BASE + 0x30))
#define GPIOA_MODER (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_OTYPER (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_OSPEEDR (*(volatile uint32_t *)(GPIOA_BASE + 0x08))
#define GPIOA_PUPDR (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))
#define GPIOA_BSRR (*(volatile uint32_t *)(GPIOA_BASE + 0x18))
#define GPIOA_IDR (*(volatile uint32_t *)(GPIOA_BASE + 0x10))
