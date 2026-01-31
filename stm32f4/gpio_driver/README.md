# STM32F4 GPIO Driver (Register-Level)

A **bare-metal, register-level GPIO driver** for STM32F4 series microcontrollers, written in C.  
This project focuses on **understanding GPIO hardware deeply** by using the STM32 reference manual directly — **no HAL, no StdPeriph, no SDK abstractions**.

The driver is **configurable**: the user selects the **GPIO port and pin at runtime**.

---

## 🎯 Purpose

- Learn how GPIO works at the **register level**
- Understand **clock enable, bit fields, and register layout**
- Write drivers using **professional CMSIS-style struct mapping**
- Build strong foundations before using HAL or RTOS

---

## ✨ Features

- **Bare-metal implementation**
- **Struct-based register mapping** (`GPIO_RegDef_t`)
- **Configurable GPIO port and pin**
- Safe GPIO output control using **BSRR**
- Input reading via **IDR**
- Clean separation of:
  - Hardware description (`gpio_hw.h`)
  - Driver logic (`gpio.c`)
  - Public API (`gpio.h`)

---

## 🧩 Driver Capabilities

### GPIO Configuration
- Configure any GPIO pin as:
  - Input
  - Output (push-pull)
- Configure:
  - Mode
  - Output speed
  - Pull-up / pull-down

### GPIO Control
- Set GPIO pin HIGH
- Set GPIO pin LOW
- Read GPIO pin state

---

## Hardware Support

- **Target MCU**: STM32F4 Series (Registers based on STM32F4 memory map: AHB1 base `0x40020000`).
- **Pins Used**:
  - **PA3**: Output (e.g., LED)
  - **PA2**: Input (e.g., Button)

## File Structure

- `gpio.h`: Function prototypes and API declarations.
- `gpio_hw.h`: Hardware register definitions and memory map addresses.
- `gpio.c`: Implementation of the driver logic.

## Usage
### Hardware Setup

- **PA3**: Output connected to LED
  - `set_gpio_pin()` → PA3 = HIGH → LED ON
  - `reset_gpio_pin()` → PA3 = LOW → LED OFF

- **PA2**: Input connected to Button with **external pull-down resistor** (10kΩ to GND)
  - Button NOT pressed → PA2 = HIGH (logic 1)
  - Button pressed → PA2 = LOW (logic 0)
 
 ---
 ## Usage 

### main.c
    ```c
    #include "gpio.h"

    int main(void)
    {
        // PA3 as output (LED)
        gpio_init(GPIOA, 3, GPIO_MODE_OUTPUT);

        // PA2 as input (Button)
        gpio_init(GPIOA, 2, GPIO_MODE_INPUT);

        while (1)
        {
            if (!gpio_read(GPIOA, 2)) {
                gpio_write(GPIOA, 3, 1);   // LED ON
            } else {
                gpio_write(GPIOA, 3, 0);   // LED OFF
            }
        }
    }
    ```

## Power Efficiency

This design uses **external pull-down resistor** instead of internal pull-up to minimize power consumption. The pin only draws current when the button is actively pressed, not in idle state.

## License

This project is licensed under the MIT License - see the [LICENSE](../../LICENSE) file for details.
