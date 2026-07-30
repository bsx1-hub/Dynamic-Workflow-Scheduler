# Hardware MVP: Live Station-State Interface

## Purpose

The first hardware prototype connects manual station-status controls to the
Dynamic Workflow Scheduler. It demonstrates that a physical event can change
the scheduler's recommended next action in real time.

This MVP uses an STM32L476 Nucleo board, three pushbuttons, three LEDs, and the
board's USB virtual COM port. It supports the same three equipment categories
already used by the C++ scheduler:

- Espresso
- Brew
- Frozen

## Demonstration

1. Start the C++ scheduler with waiting orders that require espresso, brew, and
   frozen equipment.
2. The STM32 reports all three stations as `AVAILABLE` over UART.
3. Press a station button. The STM32 debounces the input, changes that station
   to `BUSY`, turns on its LED, and sends an updated status message.
4. The desktop application updates its equipment model and calculates a new
   recommendation that avoids the busy station.
5. Press the same button again. The station returns to `AVAILABLE`, its LED
   turns off, and the scheduler may recommend it again.

```text
Espresso button ─┐
Brew button     ├─> STM32L476 ── USB UART ──> C++ scheduler ──> terminal recommendation
Frozen button   ┘       │
                        └─> three status LEDs
```

## State Model

| State | Meaning | LED | Scheduler behavior |
| --- | --- | --- | --- |
| `AVAILABLE` | Station can accept work. | Off | Eligible for recommendations. |
| `BUSY` | Station is occupied. | On | Scheduler must not assign a new batch there. |
| `OFFLINE` | Station is unavailable because of a fault, cleaning, or manual shutdown. | Slow blink | Scheduler must not assign work there. |

### Input behavior

- One debounced **short press** toggles a station between `AVAILABLE` and
  `BUSY`.
- `OFFLINE` is reserved for a later control path (for example, a long press or
  an incoming diagnostic message). It is included in the protocol now so the
  desktop model does not need a redesign later.
- On reset, the firmware reports every station as `AVAILABLE`.

## STM32L476 Pin Assignment

Assumption: STM32 Nucleo-L476RG. These pin names follow its Arduino-compatible
headers. Do not wire the LED anode directly to a GPIO pin without a resistor.

| Function | STM32 pin | Header label | Connection |
| --- | --- | --- | --- |
| Espresso button input | `PA10` | `D2` | Button to GND; internal pull-up enabled. |
| Brew button input | `PB3` | `D3` | Button to GND; internal pull-up enabled. |
| Frozen button input | `PB5` | `D4` | Button to GND; internal pull-up enabled. |
| Espresso status LED | `PB4` | `D5` | GPIO -> 220–330 ohm resistor -> LED anode; LED cathode -> GND. |
| Brew status LED | `PB10` | `D6` | GPIO -> 220–330 ohm resistor -> LED anode; LED cathode -> GND. |
| Frozen status LED | `PA8` | `D7` | GPIO -> 220–330 ohm resistor -> LED anode; LED cathode -> GND. |
| UART to USB virtual COM port | `PA2` / `PA3` | `USART2_TX` / `USART2_RX` | Use the Nucleo ST-LINK USB connector; 115200 8-N-1. |
| Optional OLED I2C clock | `PB8` | `D15` | OLED SCL. |
| Optional OLED I2C data | `PB9` | `D14` | OLED SDA. |

`PA2` and `PA3` are already connected to the Nucleo board's ST-LINK virtual COM
port, so no separate USB-to-UART adapter is needed for this MVP.

## Breadboard Wiring Plan

Use the Nucleo `3V3` and `GND` pins to power the breadboard rails.

### Buttons

For each of the three buttons:

1. Place the tactile switch across the breadboard center gap.
2. Connect one side to the common ground rail.
3. Connect the opposite side to its assigned GPIO pin.
4. Configure that GPIO as `INPUT_PULLUP` in firmware.

The unpressed input reads logic high; a press reads logic low. No external
resistor is necessary because the STM32's internal pull-up is used.

### LEDs

For each station LED:

1. Connect the assigned GPIO pin to a 220–330 ohm resistor.
2. Connect the resistor to the LED's long leg (anode).
3. Connect the LED's short leg (cathode) to the ground rail.

Drive the GPIO high to turn the LED on.

### Optional OLED

Use a 3.3 V I2C OLED module:

| OLED pin | Nucleo connection |
| --- | --- |
| VCC | `3V3` |
| GND | `GND` |
| SCL | `PB8` / `D15` |
| SDA | `PB9` / `D14` |

The OLED is not required for the first end-to-end UART demonstration. When
added, it should show the desktop scheduler's latest recommendation returned
over UART, such as `NEXT: ESPRESSO / ORDER #18`.

## UART Protocol

Use newline-delimited ASCII at 115200 baud so the messages are easy to inspect
in a terminal and parse in C++.

### STM32 -> scheduler

```text
STATION,ESPRESSO,AVAILABLE\n
STATION,BREW,BUSY\n
STATION,FROZEN,OFFLINE\n
```

The firmware sends one message immediately after a state changes and sends all
three messages once at startup.

### Scheduler -> STM32 (optional display phase)

```text
RECOMMEND,ESPRESSO,18\n
RECOMMEND,NONE,-1\n
```

The first hardware MVP only requires STM32 -> scheduler communication. The
return message is reserved for the OLED enhancement.

## C++ Integration Boundary

The desktop application should parse each `STATION` line, translate its station
name to the existing `EquipmentType`, and update the equipment state before
calling the normal scheduler decision function. The scheduling policy itself
does not change.

```text
UART line -> serial parser -> EquipmentManager state -> Scheduler::makeDecision()
          -> terminal display -> optional RECOMMEND line to STM32/OLED
```

## Parts List

| Quantity | Item | Notes |
| ---: | --- | --- |
| 1 | STM32 Nucleo-L476RG board | Includes ST-LINK USB virtual COM port. |
| 1 | USB data cable | Board power, programming, and UART connection. |
| 1 | Solderless breadboard | Half-size is sufficient. |
| 3 | Tactile pushbuttons | Momentary, normally open. |
| 3 | LEDs | Any distinct colors make station states easier to read. |
| 3 | 220–330 ohm resistors | One series resistor per LED. |
| 12–18 | Male-to-male jumper wires | Breadboard connections. |
| 1 (optional) | 0.96-inch I2C OLED, SSD1306 | Confirm 3.3 V-compatible logic. |

## Scope and Limitations

This is a proof of the data path, not a production coffee-shop controller.

- Button presses are manual stand-ins for actual machine telemetry.
- `BUSY` does not yet include automatic timing or sensor verification.
- `OFFLINE` is protocol-supported but has no dedicated physical control in the
  first build.
- The computer runs the scheduling algorithm; the STM32 only handles input,
  local indication, and serial communication.
- UART is a local USB serial link; there is no network, cloud service, order
  system integration, or persistent data logging.
- The prototype monitors three stations only. The existing software's `Other`
  category remains simulation-only during this MVP.

## Acceptance Check

The milestone is complete when you can show and explain this path:

```text
Button press -> debounced STM32 GPIO event -> STATION UART message
-> C++ equipment update -> recomputed schedule -> updated terminal recommendation
```
