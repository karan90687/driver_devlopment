## ⏱️ How `delay_ms(50)` Works (TIM2 Blocking Delay)

This driver uses **TIM2 in polling mode** to generate a blocking delay based on the timer **time-base unit**.

### Assumptions
- TIM2 clock is configured so that:
  - **1 timer tick = 1 ms**
- This is achieved by setting the **prescaler (PSC)** appropriately during timer initialization.

---

### Function Call
```c
delay_ms(50);
```
## Step-by-Step Execution

### Reset the counter
```c
TIM2->CNT = 0;
```

### Set auto-reload value 
Ensures the delay always starts from a known state.

```c
TIM2->ARR = 50 - 1;   // ARR = 49
```
### Clear the update flag (UIF)
The timer will count from 0 to 49, which equals 50 timer ticks.
```c
TIM2->SR &= ~(1 << 0);
```
### Start the timer

Clears any previous overflow event.

```c
TIM2->CR1 |= (1 << 0);   // CEN = 1
```

### The timer begins counting.

Timer counting (hardware operation)
```c
With a 1 ms tick:

CNT = 0   → 0 ms
CNT = 1   → 1 ms

CNT = 49  → 49 ms
CNT overflow → 50 ms → UIF set
```

### Polling until delay completes
```c
while (!(TIM2->SR & (1 << 0))) {
    // wait
}
```


The CPU blocks here until 50 ms have elapsed.

### Stop timer and cleanup
```c
TIM2->CR1 &= ~(1 << 0);   // stop timer
TIM2->SR  &= ~(1 << 0);   // clear UIF
```

### Result

CPU is blocked for exactly 50 ms

### Delay accuracy depends on:

- Timer clock source

- Prescaler configuration

- Key Formula
Delay = (ARR + 1) × Timer Tick Period


- For this example:

    Delay = 50 × 1 ms = 50 ms

### Notes

- This is a blocking delay (CPU waits).

- No interrupts or SysTick are used.

Intended for learning and simple timing tasks.


---

