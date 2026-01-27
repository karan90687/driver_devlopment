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
    // Turn on user LED (PA3)
    set_gpio_pin();

    // Read button state (PA2)
    if (gpio_read_pin()) {
        // Button pressed
    }
    ```

## License

This project is licensed under the MIT License - see the [LICENSE](../../LICENSE) file for details.
