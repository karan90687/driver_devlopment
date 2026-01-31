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


/* 
 * GPIO_RegDef_t defines the register layout.
 * Base address is cast to this struct.
 * Member order = register offsets (computed by compiler).
 * What happens when you write GPIOA->MODER
 * address = GPIOA_BASE + offset_of(MODER)
 * GPIOA is a pointer to GPIO_RegDef_t struct at address 0x40020000
 */
// GPIO register definition structure
#define GPIOA ((GPIO_RegDef_t *) GPIOA_BASE)
#define GPIOB ((GPIO_RegDef_t *) GPIOB_BASE)
#define GPIOC ((GPIO_RegDef_t *) GPIOC_BASE)
#define GPIOD ((GPIO_RegDef_t *) GPIOD_BASE)
#define GPIOE ((GPIO_RegDef_t *) GPIOE_BASE)
#define GPIOF ((GPIO_RegDef_t *) GPIOF_BASE)
#define GPIOG ((GPIO_RegDef_t *) GPIOG_BASE)
#define GPIOH ((GPIO_RegDef_t *) GPIOH_BASE)

typedef struct {
	volatile uint32_t MODER  ;	// gpio mode register at 				0x00
	volatile uint32_t OTYPER ;	// gpio output type register at 		0x04
	volatile uint32_t OSPEEDR;	// gpio output speed register at 		0x08
	volatile uint32_t PUPDR	 ;	// gpio pull up pull down register at 	0x0c
	volatile uint32_t IDR	 ;	// gpio input data register at 			0x10
	volatile uint32_t ODR	 ;	// gpio output data register at 		0x14
	volatile uint32_t BSRR	 ;	// gpio bit set reset regisetr at 		0x18
	volatile uint32_t LCKR	 ;	// gpio lock register at 				0x1c
	volatile uint32_t AFR[2] ;	/* gpio alternate function high and
								   low register at 						0x20 and
																		0x24*/
}GPIO_RegDef_t;