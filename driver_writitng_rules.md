# **Non-negotiable driver-writing rules (memorize these):**

1. **Read the Reference Manual first.**
   If you can’t point to a page/offset, don’t write the code.

2. **Separate roles strictly.**

   * `*_hw.h` → hardware facts only (bases, offsets).
   * `*.c` → all register writes and logic.
   * `*.h` → public API only.

3. **Enable the clock before touching registers.**
   Peripheral does not exist until its RCC bit is set.

4. **Know the register type before writing.**

   * Multi-bit field (state) → **clear then set**.
   * Single-bit flag → set/clear directly.
   * Write-only/action (e.g., BSRR) → write once, no RMW.
   * W1C flags → write 1 to clear (read manual).

5. **Never overwrite shared registers.**
   Use read-modify-write when required; touch only your bits.

6. **Configure once, use forever.**
   Set MODER/OTYPER/OSPEEDR/PUPDR once during init; don’t change at runtime.

7. **Use the right register for runtime actions.**

   * Output → **BSRR** (atomic).
   * Input → **IDR** (read-only).

8. **No logic in hardware headers.**
   No `if`, no comparisons, no decisions in `*_hw.h`.

9. **Avoid magic numbers in logic.**
   Offsets and bit positions come from headers, not inline literals.

10. **Assume concurrency.**
    Write code that remains correct if an interrupt runs.

11. **Validate on hardware methodically.**
    Check clock → mode → data register → physical pin, in that order.

12. **Freeze working code.**
    When a driver works, stop touching it and move on.

If you follow these, your drivers will be correct, portable, and professional.


| Value | Meaning                |
| ----- | ---------------------- |
| `00`  | Input                  |
| `01`  | General purpose output |
| `10`  | Alternate function     |
| `11`  | Analog                 |

## WARNING : 
- Clear bits before writing ONLY when the field has more than one bit.
- For the sigle bit the second 
- Because there is no “previous 2-bit state” to erase.