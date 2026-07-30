# Hardware Extension Roadmap

The v1.0 scheduler uses simulated orders and equipment states.

The hardware extension will connect real inputs to the C++ scheduler through an embedded controller.

## Planned inputs

- Manual order entry through UART
- Equipment busy/ready status
- Inventory or sensor alerts

## Planned output

- Scheduler recommendations sent to a serial terminal or display

## First hardware target

An STM32 board sends structured UART messages representing live order and equipment events.