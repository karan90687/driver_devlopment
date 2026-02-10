#include "delay_hw.h"

void delay_init(void){
    // enable clock for the timer 
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;
    // setting the prescaler for the timer
    TIM2->PSC = 84000 - 1;   // 1 ms tick
}

void delay_ms(uint32_t ms)
{
    /* Reset counter */
    TIM2->CNT = 0;

    /* Set auto-reload for required delay */
    TIM2->ARR = ms - 1;

    /* Clear update flag (UIF) */
    TIM2->SR &= ~(1 << 0);

    /* Start timer (CEN = 1) */
    TIM2->CR1 |= (1 << 0);

    /* Wait until timer overflows */
    while (!(TIM2->SR & (1 << 0))) {
        /* blocking wait */
    }

    /* Stop timer */
    TIM2->CR1 &= ~(1 << 0);

    /* Clear update flag again */
    TIM2->SR &= ~(1 << 0);
}
