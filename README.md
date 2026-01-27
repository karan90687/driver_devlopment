# MCU Drivers (Low-Level Learning Repository)

This repository contains **low-level peripheral drivers** written from scratch for learning embedded systems **at the register level**.

The goal of this repo is **not reuse**, **not production**, and **not abstraction** —  
it is to deeply understand **how MCUs work internally** by writing drivers directly from the **Reference Manual**.

---

## 🎯 Objectives

- Learn how to write **bare-metal drivers**
- Understand **register maps, bit fields, and clock control**
- Build strong fundamentals before using HAL/SDKs
- Develop **driver discipline and structure**
- Create a personal reference for future embedded work

---

## 🚧 Current Status

- ✅ A simple GPIO driver implemented (STM32F446)
- ⏳ Timer driver — planned
- ⏳ UART driver — planned
- ⏳ DMA and advanced peripherals — planned

This repository is **actively evolving** as part of a structured learning process.

---

## 🧠 Learning Philosophy

- No HAL, no Arduino, no SDK abstractions
- Only **Reference Manual + Datasheet**
- One peripheral at a time
- Start simple → then generalize
- Correct structure > clever code

---

## 📂 Repository Structure

```text
mcu-drivers/
├── README.md
├── LICENSE
├── DRIVER_RULES.md
│
├── stm32f4/
│   ├── README.md
│   │
│   ├── gpio/
│   │   ├── gpio_hw.h
│   │   ├── gpio.h
│   │   └── gpio.c
│   │
│   ├── timer/
│   │   ├── tim_hw.h
│   │   ├── tim.h
│   │   └── tim.c
│   │
│   └── uart/
│       ├── uart_hw.h
│       ├── uart.h
│       └── uart.c
│
├── esp32/
│   ├── README.md
│   └── gpio/
│       ├── gpio_hw.h
│       ├── gpio.h
│       └── gpio.c
│
└── docs/
    ├── register_notes.md
    └── learning_log.md
```

---

## 🧩 Driver Design Rules

All drivers follow these **non-negotiable rules**:

- `*_hw.h` → **hardware only** (base addresses, registers, offsets)
- `*.c` → **all register writes and logic**
- `*.h` → **public API only**
- Clock enabled before touching registers
- Multi-bit fields → clear then set
- Runtime GPIO output → **BSRR only**
- Input read → **IDR only**


---

## 🚧 Scope of This Repository

What this repo **includes**:
- GPIO drivers
- Timer drivers
- UART drivers
- Register-level notes
- Learning documentation

What this repo **does NOT include**:
- HAL / CubeMX / Arduino
- Application logic
- `main.c`
- Build systems
- IDE configuration files

---

## 📖 How to Use This Repo

- Read the MCU-specific `README.md`
- Open the Reference Manual
- Study `*_hw.h` first
- Then follow logic in `*.c`
- Cross-verify with the datasheet

---

## 📜 License

This project is licensed under the **MIT License**.  
See the [LICENSE](LICENSE) file for details.

---

## ✍️ Author

**Karan Rajput**  
Embedded Systems & Low-Level Driver Learning

---

## ⚠️ Disclaimer

These drivers are written **for learning purposes only**.  
They are **not intended for direct production use** without review and validation.
