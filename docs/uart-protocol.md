# STM32 Equipment-Status UART Protocol

## Purpose

This protocol carries manual equipment availability changes from the STM32
button-and-LED controller to the Dynamic Workflow Scheduler. Each valid line
represents the current state of one station.

The STM32 sends a line immediately after a debounced button press changes that
station's state. The C++ side reads lines independently and updates the matching
station state before choosing its next recommendation.

## Serial settings

| Setting | Value |
| --- | --- |
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |
| Line ending | `\n` (an optional preceding `\r` is accepted) |

These settings work in STM32CubeIDE UART configuration and a desktop serial
terminal. The initial C++ demo may read the same lines from a text stream.

## Message format

```text
STATUS,<STATION>,<STATE>\n
```

All protocol keywords, station names, and state names are uppercase ASCII.
There are exactly three comma-separated fields; no spaces are permitted.

| Field | Required values | Meaning |
| --- | --- | --- |
| `STATUS` | Literal `STATUS` | Equipment-status message type |
| `<STATION>` | `ESPRESSO`, `BREW`, `FROZEN` | Physical station whose button was pressed |
| `<STATE>` | `AVAILABLE`, `BUSY` | New current availability reported by the STM32 |

`OTHER` is intentionally excluded: it is a scheduler-only simulated category,
not a physical station in the STM32 MVP.

## Valid examples

```text
STATUS,ESPRESSO,AVAILABLE
STATUS,BREW,BUSY
STATUS,FROZEN,AVAILABLE
```

Example serial log after button presses:

```text
STATUS,ESPRESSO,BUSY
STATUS,BREW,BUSY
STATUS,ESPRESSO,AVAILABLE
```

## STM32 behavior

1. On startup, set every physical station to `AVAILABLE` and send one status
   line for each station.
2. After a valid debounced press, toggle only that station between `AVAILABLE`
   and `BUSY`.
3. Update its LED first, then transmit the resulting `STATUS` line once.
4. Do not transmit a line for bounce, a held button, or an unchanged state.

The LED is the local source-of-truth indicator: LED on means `BUSY`; LED off
means `AVAILABLE`.

## Scheduler behavior

On a valid line, the parser maps the station to `EquipmentType` and stores the
reported state in `EquipmentManager`:

| UART station | C++ equipment type |
| --- | --- |
| `ESPRESSO` | `EquipmentType::Espresso` |
| `BREW` | `EquipmentType::Brew` |
| `FROZEN` | `EquipmentType::Frozen` |

An externally reported `BUSY` station is unavailable for scheduling. The
scheduler must not recommend an order or batch needing that station. A later
`AVAILABLE` report re-enables it. Repeated reports of the same state are valid
and idempotent.

For the MVP, manual UART status takes precedence over simulation timing for the
three physical stations. The scheduler does not send acknowledgements or
commands back to the STM32.

## Invalid input and recovery

The C++ parser must reject an invalid line safely: no exception escaping the
input loop, no equipment-state change, and an optional development log message.

Reject lines that are blank, incomplete, have extra fields, contain spaces,
use an unknown station or state, or use a message type other than `STATUS`.

Examples to reject:

```text
STATUS,ESPRESSO
STATUS,OTHER,BUSY
STATUS,ESPRESSO,OFFLINE
status,ESPRESSO,BUSY
STATUS, ESPRESSO,BUSY
STATUS,ESPRESSO,BUSY,EXTRA
```

If a serial line is interrupted or corrupted, discard only that line and wait
for the next newline-delimited message. Startup status messages and every later
button toggle allow the desktop side to recover without resetting the board.

## Scope boundary

This is a deliberately small one-way MVP protocol. It reports manually chosen
station availability; it does not identify individual orders, report sensor
measurements, synchronize time, or control real coffee equipment.
