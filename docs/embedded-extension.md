# STM32 Embedded Extension

## Purpose

This document defines a future embedded extension for the Dynamic Workflow Scheduler.

The goal is not to move the full scheduling algorithm onto a microcontroller. Instead, the STM32 acts as a hardware interface that observes equipment state and sends that information to the C++ scheduler.

This keeps the project modular:

- The STM32 handles physical inputs and low-level hardware communication.
- The desktop C++ application handles queue management, urgency scoring, batching, and scheduling decisions.
- UART connects the embedded hardware layer to the scheduling software.

The first prototype should remain intentionally simple and demonstrate the hardware/software architecture without overbuilding the system.

## What the STM32 Would Do

The STM32 would monitor the availability of the main preparation stations:

- Espresso station
- Brew station
- Frozen-drink station

For the first prototype, each station can be represented by a push button, toggle switch, or simple digital sensor.

The STM32 would:

1. Read the state of each equipment input.
2. Debounce button or switch signals when necessary.
3. Detect equipment-state changes.
4. Convert the hardware state into a serial message.
5. Send the equipment state to the C++ scheduler through UART.
6. Optionally receive acknowledgement or status messages from the scheduler.

The STM32 would not calculate urgency, reorder the queue, or select batches in the first version.

## Input Signals

The initial embedded prototype uses three digital inputs.

| Input | Meaning | Example Hardware |
| --- | --- | --- |
| Espresso status | Indicates whether the espresso station is available | Push button, toggle switch, limit switch, or digital sensor |
| Brew status | Indicates whether the brew station is available | Push button, toggle switch, limit switch, or digital sensor |
| Frozen status | Indicates whether the frozen-drink station is available | Push button, toggle switch, limit switch, or digital sensor |

A simple active-high design could use:

```text
GPIO HIGH = AVAILABLE
GPIO LOW  = BUSY
```

The exact polarity may be changed depending on the selected circuit. Internal pull-up or pull-down resistors can be used to keep each input in a defined state.

### Suggested STM32 GPIO Mapping

Example assignments for an STM32 development board:

| Signal | Example Pin |
| --- | --- |
| Espresso available | PA0 |
| Brew available | PA1 |
| Frozen available | PA4 |

These pins are placeholders. Final pin assignments should be selected based on the specific STM32 board and the UART pins already in use.

## Output Signals

The primary STM32 output is a UART serial message sent to the desktop application.

Optional physical outputs could be added later:

- Status LEDs for each equipment station
- A communication activity LED
- An error indicator
- A small OLED display showing the transmitted state

For the minimum prototype, UART is the only required output.

## Communication Interface

### UART

UART is the proposed communication interface between the STM32 and the C++ scheduler.

UART is appropriate because it is:

- Simple to configure
- Supported by STM32 hardware
- Easy to inspect in a serial terminal
- Suitable for low-bandwidth equipment-state messages
- Compatible with a USB-to-serial connection to a computer

A common initial configuration would be:

```text
Baud rate: 115200
Data bits: 8
Parity: None
Stop bits: 1
Flow control: None
```

### Serial Message Format

The first message format uses one line per equipment update:

```text
ESPRESSO:AVAILABLE
BREW:BUSY
FROZEN:AVAILABLE
```

Each message ends with a newline character:

```text
\n
```

The scheduler can process incoming data one line at a time.

Valid equipment names:

```text
ESPRESSO
BREW
FROZEN
```

Valid state values:

```text
AVAILABLE
BUSY
```

### Example State Change

If the frozen station becomes busy, the STM32 sends:

```text
FROZEN:BUSY
```

When it becomes available again:

```text
FROZEN:AVAILABLE
```

### Optional Combined Message

A later version could send the complete state in one message:

```text
ESPRESSO:AVAILABLE;BREW:BUSY;FROZEN:AVAILABLE
```

The line-based single-update format is easier to implement and debug for the first prototype.

## Proposed Sensors

The first version should use simple manually controlled inputs before attempting automatic sensing.

### Phase 1: Buttons or Toggle Switches

Recommended for the first embedded demonstration:

- Three push buttons or toggle switches
- One input per equipment station
- Internal pull-up or pull-down resistors
- Software debouncing

This demonstrates the complete communication path without requiring modifications to real equipment.

### Phase 2: Non-Invasive Sensors

Possible later sensors include:

#### Current Sensor

A current sensor could detect whether a machine is drawing power.

Possible use:

- Detect when a blender motor is active
- Detect when a brewer or heating system is operating

Limitations:

- Electrical isolation is required
- Power consumption may not map directly to availability
- Mains-powered equipment introduces safety concerns

#### Vibration Sensor

A vibration sensor or accelerometer could detect active equipment.

Possible use:

- Detect blender operation
- Detect mechanical machine activity

Limitations:

- Environmental vibration may cause false readings
- Thresholds require calibration

#### Limit Switch or Reed Switch

A switch could detect a physical position or machine state.

Possible use:

- Detect whether a component is inserted
- Detect whether a lid, tray, or handle is in a usable position

#### Optical or Proximity Sensor

An optical, infrared, or proximity sensor could detect whether an equipment area is occupied.

Possible use:

- Detect cup placement
- Detect whether a station is currently being used

Limitations:

- Object presence does not always indicate equipment availability
- Placement and calibration affect reliability

### Recommended First Prototype

Use three buttons or toggle switches.

This avoids electrical and mechanical integration risks while still demonstrating:

- GPIO input
- Debouncing
- State tracking
- UART communication
- Serial parsing
- Hardware-aware scheduling

## Hardware/Software Boundary

The project should maintain a clear boundary between the embedded hardware and the scheduler.

### STM32 Responsibilities

The STM32 handles:

- Reading GPIO signals
- Debouncing input signals
- Tracking equipment-state changes
- Formatting UART messages
- Transmitting equipment state
- Reporting basic communication or sensor errors

### C++ Scheduler Responsibilities

The desktop C++ application handles:

- Receiving UART data
- Parsing equipment-state messages
- Updating equipment availability
- Managing orders
- Calculating urgency
- Selecting anchor orders
- Selecting compatible batches
- Avoiding unavailable equipment
- Updating the terminal dashboard
- Recording metrics

### Shared Data Contract

Both sides must agree on:

- Equipment names
- State names
- Message delimiters
- Line termination
- Baud rate
- Error behavior

The communication format acts as the interface contract between hardware and software.

## Proposed C++ Integration

The desktop application could add a serial-input component:

```text
SerialInterface
    |
    +-- opens the UART/COM port
    +-- reads complete lines
    +-- validates incoming messages
    +-- converts text into equipment state
    +-- updates EquipmentManager
```

Example parsed update:

```cpp
equipmentManager.setExternalState(
    EquipmentType::Brew,
    EquipmentState::Busy
);
```

Before assigning a new order, the scheduler would check the required station:

```cpp
if (!equipmentManager.isAvailable(requiredEquipment)) {
    // Do not assign this order yet.
}
```

The scheduler should then select the highest-urgency order whose required equipment is currently available.

## Error Handling

The interface should reject malformed messages safely.

Examples of invalid messages:

```text
ESPRESSO:READY
OVEN:AVAILABLE
BREW
FROZEN-BUSY
```

Possible behavior:

1. Ignore the invalid message.
2. Log an error in the terminal.
3. Preserve the last known valid equipment state.
4. Avoid changing scheduler state from malformed input.

If communication is lost, the scheduler should not automatically assume that all equipment is available.

A safe future policy could mark equipment state as:

```text
UNKNOWN
```

until communication is restored.

## Future Closed-Loop Behavior

The first prototype is an open-loop monitoring system:

```text
Equipment input
      ↓
STM32
      ↓ UART
C++ scheduler
      ↓
Updated recommendation
```

A future closed-loop system could also send commands or acknowledgements back to the STM32:

```text
Physical equipment state
          ↓
        STM32
          ↓
    C++ scheduler
          ↓
Scheduling decision
          ↓
 Worker or actuator response
          ↓
Updated equipment state
```

Possible future behaviors include:

- Automatically detecting when a machine begins operating
- Automatically detecting when preparation finishes
- Marking equipment busy without manual input
- Marking equipment available after a sensor-confirmed completion
- Preventing the scheduler from assigning work to unavailable stations
- Lighting an LED for the recommended station
- Displaying the next order on an embedded screen
- Sending an acknowledgement when the scheduler receives a state update
- Detecting communication loss
- Logging physical equipment utilization
- Comparing planned versus actual preparation time
- Adjusting estimated preparation times from observed data
- Recalculating the schedule whenever equipment state changes

## Example Closed-Loop Scenario

1. The frozen station is available.
2. The scheduler assigns a frozen-drink order.
3. The worker begins using the blender.
4. A switch or sensor changes the frozen station state to busy.
5. The STM32 sends:

```text
FROZEN:BUSY
```

6. The C++ scheduler updates the equipment state.
7. The scheduler avoids assigning additional frozen work.
8. The blender finishes.
9. The STM32 detects that the station is available and sends:

```text
FROZEN:AVAILABLE
```

10. The scheduler recalculates the queue and may assign the next frozen order.

## Minimum Embedded Demonstration

The minimum demonstration should include:

### Hardware

- STM32 development board
- Three buttons or toggle switches
- Optional status LEDs
- USB connection to a computer
- UART or virtual COM port

### Firmware

- GPIO input configuration
- Input debouncing
- Equipment-state tracking
- UART initialization
- State-change message transmission

### Desktop Software

- Serial-port reader
- Message parser
- Equipment-state update
- Scheduler availability check
- Terminal event display

### Demonstration Sequence

1. Start the C++ scheduler.
2. Connect the STM32 serial interface.
3. Mark all three stations available.
4. Change the brew input to busy.
5. Confirm that the STM32 sends:

```text
BREW:BUSY
```

6. Confirm that the terminal dashboard marks Brew as busy.
7. Confirm that the scheduler does not recommend a brew order.
8. Change Brew back to available.
9. Confirm that the scheduler recalculates and may select a brew order.

## Scope Decision

The first embedded extension should not include:

- Direct connection to commercial coffee equipment
- Mains-voltage sensing
- Motor control
- Robotic actuators
- Wireless networking
- Cloud communication
- Complex sensor fusion
- Automatic recipe execution
- Full touchscreen interface

The goal is to demonstrate a credible embedded architecture, not to build the complete production system.

## Summary

The proposed STM32 extension uses three equipment-status inputs and UART communication to connect physical equipment state to the C++ scheduling engine.

The STM32 acts as the hardware interface, while the desktop application retains responsibility for scheduling, batching, lifecycle management, metrics, and visualization.

This design demonstrates how the current software prototype could evolve into a closed-loop embedded workflow assistant without requiring full sensor integration in the first hardware version.
