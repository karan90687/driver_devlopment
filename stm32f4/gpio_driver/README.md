# STM32F4 GPIO Driver

A simple, bare-metal GPIO driver for STM32F4 series microcontrollers, written in C. This project demonstrates direct register manipulation for GPIO configuration and control without relying on HAL or Standard Peripheral Libraries.

## Features

- **Bare Metal Implementation**: Direct access to hardware registers for maximum efficiency and understanding of the hardware.
- **GPIO Configuration**:
  - Configures **GPIOA Pin 3** as an **Output** (Push-Pull, Medium Speed).
  - Configures **GPIOA Pin 2** as an **Input** (No Pull-up/Pull-down).
- **Control Functions**:
  - `gpio_init()`: Initializes the GPIO peripheral and pin modes.
  - `set_gpio_pin()`: Sets GPIOA Pin 3 High.
  - `reset_gpio_pin()`: Sets GPIOA Pin 3 Low.
  - `gpio_read_pin()`: Reads the state of GPIOA Pin 2.

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
### main.c 
1. Include the header file in your main application:
    ```c
    #include "gpio.h"
    ```
2. Initialize the GPIO driver:
    ```c
    gpio_init();
    ```
3. Control the pins:
    ```c
    while(1) {
        if (!gpio_read_pin()) {  // Button pressed (PA2 pulled LOW)
            set_gpio_pin();      // Turn on LED (PA3)
        } else {
            reset_gpio_pin();    // Turn off LED (PA3)
        }
    }

## Power Efficiency

This design uses **external pull-down resistor** instead of internal pull-up to minimize power consumption. The pin only draws current when the button is actively pressed, not in idle state.

## License

This project is licensed under the MIT License - see the [LICENSE](../../LICENSE) file for details.
