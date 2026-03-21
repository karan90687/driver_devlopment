#ifndef TIMER_HW_H
#define TIMER_HW_H

#include <stdint.h>

// base address of TIM 2
#define TIM2_BASE 0x40000000
// base address of clock 
#define RCC_PERIPH_BASE  0x40023800

// to enable the tim2 register we need to set the bit 0 in the APB1ENR register of RCC
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_PERIPH_BASE + 0x40))
#define RCC_APB1ENR_TIM2EN (1 << 0) 

// tim2 register structure
typedef struct {
    volatile uint32_t CR1;		// control register 1 at 0x00
    volatile uint32_t CR2;		// control register 2 at 0x04
    volatile uint32_t SMCR;		// slave mode control register at 0x08
    volatile uint32_t DIER;		// DMA/Interrupt enable register at 0x0c
    volatile uint32_t SR;		// status register at 0x10
    volatile uint32_t EGR;		// event generation register at 0x14
    volatile uint32_t CCMR1;	// capture/compare mode register 1 at 0x18
    volatile uint32_t CCMR2;	// capture/compare mode register 2 at 0x1c
    volatile uint32_t CCER;		// capture/compare enable register at 0x20
    volatile uint32_t CNT;		// counter at 0x24
    volatile uint32_t PSC;		// prescaler at 0x28
    volatile uint32_t ARR;		// auto-reload register at 0x2c
} TIM2_RegDef_t;

// pointer to the struct of tim2 registers
#define TIM2 ((TIM2_RegDef_t *) TIM2_BASE)


#endif 