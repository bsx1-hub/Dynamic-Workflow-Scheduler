# Dynamic Workflow Scheduler

A modular C++ workflow-scheduling system designed to reduce cognitive load and decision latency in high-volume service environments.

## Project Overview

The Dynamic Workflow Scheduler is a C++ prototype inspired by workflow bottlenecks I observed while working in a high-volume coffee shop environment.

During rush periods, workers must process several incoming order streams, recognize urgent tickets, identify similar preparation tasks, monitor equipment availability, and continuously decide what should happen next.

The primary bottleneck is not always the physical speed of production. It can also be the cognitive overhead created by repeatedly sorting, prioritizing, batching, and reorganizing work while completing physical tasks.

This project explores how a software scheduling layer can support workers by:

* Dynamically prioritizing incoming orders
* Preventing lower-priority orders from being indefinitely neglected
* Identifying compatible preparation tasks
* Tracking equipment availability
* Recommending a concrete next action
* Displaying workflow state through a live terminal dashboard
* Supporting manual order and lifecycle commands
* Comparing scheduling strategies through repeatable simulation

## Problem Statement

Service workers may receive orders from several concurrent sources:

* Drive-thru
* Mobile ordering
* Front-counter or eat-in customers

Each source may have a different operational priority. Orders may also require different equipment or preparation processes, including:

* Espresso station
* Brewing station
* Frozen-drink station
* Other preparation stations

Workers must mentally balance:

1. Service urgency
2. Wait time
3. Process similarity
4. Order state
5. Equipment availability
6. Estimated preparation time

A strict first-in, first-out queue does not account for all these workflow constraints.

A rigid priority rule such as:

```text
Drive-Thru > Mobile > Eat-In
```

can also cause lower-priority orders to wait indefinitely during sustained demand.

The Dynamic Workflow Scheduler treats the workflow as a real-time scheduling problem rather than a simple queue.

## Current Features

### Scheduling

* Structured order modeling
* Drive-thru, mobile, and eat-in order sources
* Dynamic urgency scoring
* Aging to reduce starvation
* Deterministic priority tie-breaking
* Process-aware compatibility scoring
* Equipment-based batch selection
* Maximum batch-size enforcement
* Concrete next-action recommendations

### Order Management

* Waiting, in-progress, complete, and cancelled states
* Safe rejection of invalid lifecycle transitions
* Duplicate order ID rejection
* Order lookup by ID
* Manual order creation
* Manual lifecycle control

### Simulation

* Randomized drink, size, temperature, and preparation attributes
* Normal-demand and rush-demand generation
* Simulated order arrival times
* Estimated preparation times
* Equipment availability simulation
* Concurrent equipment operation
* FIFO scheduling simulation
* Dynamic scheduling with batching
* Comparative performance metrics
* Repeatable simulation using fixed random seeds

### Interface

* Live terminal refresh
* Color-coded workflow state
* Simulation timer
* Rush-level indicator
* Next-action display
* Active-equipment display
* Waiting-queue table
* Completed-order and wait-time metrics
* Manual command mode
* Separate demonstration and interactive modes

### Development

* Automated scheduler and lifecycle tests
* Modular C++ architecture
* CMake build support
* Command-line compilation support
* Git version control

## Architecture

The project separates order data, queue management, scheduling policy, demand generation, equipment state, simulation control, terminal presentation, and user commands.

```text
main.cpp
    |
    +-- CommandProcessor
    |       |
    |       +-- parses manual commands
    |       +-- creates orders
    |       +-- controls order lifecycle
    |
    +-- TerminalDisplay
    |       |
    |       +-- renders the live dashboard
    |       +-- color-codes system state
    |       +-- displays metrics and recommendations
    |
    +-- OrderGenerator
    |       |
    |       +-- generates normal and rush demand
    |       +-- randomizes order attributes
    |
    +-- SimulationRunner
    |       |
    |       +-- runs FIFO scheduling
    |       +-- runs dynamic batching
    |       +-- records performance metrics
    |
    +-- EquipmentManager
    |       |
    |       +-- tracks station availability
    |       +-- advances equipment timers
    |
    +-- QueueManager
    |       |
    |       +-- owns Order objects
    |       +-- enforces lifecycle transitions
    |
    +-- Scheduler
            |
            +-- calculates urgency
            +-- prioritizes orders
            +-- scores compatibility
            +-- selects batch candidates

Order
    |
    +-- shared order data model
    +-- source and lifecycle enums
```

## Core Components

### Order

The `Order` structure represents one customer order.

Each order stores:

* Order ID
* Drink name
* Size
* Hot or iced state
* Order source
* Placement timestamp
* Preparation build key
* Current lifecycle status
* Estimated preparation time

```cpp
struct Order {
    int id;
    std::string drink;
    std::string size;
    bool hot;
    Source source;
    std::time_t placedAt;
    std::string buildKey;
    Status status;
    int estimatedPrepSeconds;
};
```

### QueueManager

`QueueManager` owns the order collection and provides operations for:

* Adding orders
* Rejecting duplicate IDs
* Finding orders by ID
* Removing orders
* Starting orders
* Completing orders
* Cancelling orders
* Returning waiting orders
* Returning active orders
* Counting completed orders
* Passing order data to the scheduler

### Scheduler

`Scheduler` contains the scheduling policy.

It is responsible for:

* Calculating urgency
* Sorting orders by priority
* Handling deterministic tie-breaking
* Calculating compatibility scores
* Determining whether orders may batch
* Selecting the highest-urgency anchor
* Ranking compatible candidates
* Enforcing maximum batch size
* Returning a structured scheduling decision

### TerminalDisplay

`TerminalDisplay` renders a color-coded live view of the workflow.

The dashboard displays:

* Simulation time
* Current rush level
* Latest system event
* Recommended equipment station
* Anchor order
* Compatible batch orders
* Busy and available equipment
* Waiting and in-progress orders
* Completed-order count
* Average waiting time
* Current recommended batch size

The terminal refreshes after events such as:

* A new order arriving
* An order starting
* An order completing
* An order being cancelled
* The schedule being recalculated

### CommandProcessor

`CommandProcessor` provides an interactive manual-control mode.

Supported commands:

```text
add
start <id>
complete <id>
cancel <id>
find <id>
pause
resume
help
quit
```

The `add` command prompts the user for:

* Drink name
* Size
* Temperature
* Order source
* Required equipment
* Estimated preparation time

The command processor uses `QueueManager` lifecycle rules, so invalid state transitions are rejected safely.

Examples:

```text
start 12
complete 12
cancel 18
find 21
```

While the system is paused, state-changing commands are blocked. Lookup, help, resume, and quit commands remain available.

### OrderGenerator

`OrderGenerator` produces repeatable simulated order streams.

It randomizes:

* Source
* Drink
* Size
* Temperature
* Build key
* Arrival time
* Estimated preparation time

Initial source probabilities:

| Source     | Probability |
| ---------- | ----------: |
| Drive-Thru |         45% |
| Mobile     |         35% |
| Eat-In     |         20% |

Supported demand scenarios:

| Scenario      | Arrival interval |
| ------------- | ---------------: |
| Normal demand |    15–45 seconds |
| Rush demand   |     2–10 seconds |

A fixed random seed can be used to reproduce the same simulation.

### EquipmentManager

`EquipmentManager` tracks four simulated preparation stations:

* Espresso station
* Brew station
* Frozen station
* Other station

Each station has one of two states:

```cpp
enum class EquipmentState {
    Available,
    Busy
};
```

Different stations may operate concurrently.

Initial fixed durations:

| Equipment        | Busy duration |
| ---------------- | ------------: |
| Espresso station |       3 ticks |
| Brew station     |       2 ticks |
| Frozen station   |       4 ticks |
| Other station    |        1 tick |

### SimulationRunner

`SimulationRunner` executes the same order stream using different scheduling strategies.

```cpp
enum class SchedulingStrategy {
    FIFO,
    DynamicBatching
};
```

The runner tracks:

* Orders completed
* Total simulation ticks
* Average wait time
* Maximum wait time
* Equipment operations
* Orders incorporated into batches

## Scheduling Logic

The scheduler assigns each waiting order an urgency score:

```text
urgency = source rate × seconds waiting
```

Initial source rates:

| Order Source | Urgency Rate |
| ------------ | -----------: |
| Drive-Thru   |          1.0 |
| Mobile       |          0.6 |
| Eat-In       |          0.4 |

Drive-thru orders accumulate urgency more quickly because they are modeled as more time-sensitive.

Urgency also increases with wait time. Therefore, an older mobile or eat-in order can eventually overtake a newer drive-thru order.

This aging behavior reduces the risk of starvation.

### Urgency Example

An eat-in order waiting for 200 seconds receives:

```text
0.4 × 200 = 80
```

A drive-thru order waiting for 30 seconds receives:

```text
1.0 × 30 = 30
```

The scheduler selects the older eat-in order because its urgency score is higher.

### Priority Tie-Breaking

When two waiting orders have equal urgency, the scheduler uses:

1. Earlier placement time
2. Lower order ID

This produces deterministic results.

## Process-Aware Batching

The dynamic scheduler selects the highest-urgency waiting order as the anchor.

It then searches for compatible waiting orders that can be prepared alongside the anchor without causing excessive priority inversion.

### Build Key

`buildKey` represents the primary equipment or preparation process required by an order.

Current values:

* `espresso`
* `brew`
* `frozen`
* `other`

Example:

```text
Latte          → espresso
Cappuccino     → espresso
Frozen Matcha  → frozen
```

A latte and cappuccino may be included in one espresso batch, while frozen matcha is excluded.

### Compatibility Score

```text
Same buildKey:     +40
Same temperature:  +10
Same drink:        +10
```

| Compatibility                        | Score |
| ------------------------------------ | ----: |
| Different process                    |     0 |
| Same process                         |    40 |
| Same process and temperature         |    50 |
| Same process, temperature, and drink |    60 |

Compatible candidates are ranked by:

1. Higher compatibility score
2. Higher urgency
3. Lower order ID

### Priority Protection

A candidate is rejected when:

* It uses a different build key
* It is not waiting
* It is the same order as the anchor
* Its compatibility score is too low
* It is more urgent than the anchor
* Its urgency gap exceeds the permitted threshold

Batching fills around the priority system rather than replacing it.

### Batch Size

The default maximum batch size is three total orders:

```text
1 anchor order
2 compatible batch candidates
```

## Order Lifecycle

The current lifecycle is:

```text
Waiting → InProgress → Complete
    |
    └──────────────→ Cancelled
```

Valid transitions:

```text
Waiting → InProgress
InProgress → Complete
Waiting → Cancelled
InProgress → Cancelled
```

Completed and cancelled orders are terminal and are excluded from future scheduling decisions.

Invalid examples include:

```text
Waiting → Complete
Complete → InProgress
Cancelled → InProgress
Complete → Cancelled
```

## Program Modes

When the program starts, the user can select a mode:

```text
1. Live demonstration
2. Manual command mode
3. Quit
```

### Live Demonstration

The live demonstration:

* Loads example orders
* Calculates the initial schedule
* Adds a new order
* Starts a recommended batch
* Updates equipment state
* Completes the batch
* Recalculates the schedule
* Displays each event through the live dashboard

### Manual Command Mode

Manual mode allows the user to add and control orders directly.

Example session:

```text
> add
Drink name: Latte
Size (S/M/L): M
Temperature (hot/iced): hot
Source (drive/mobile/eatin): drive
Equipment (espresso/brew/frozen/other): espresso
Estimated preparation seconds: 90

> find 1
> start 1
> complete 1
> quit
```

## Scheduling Strategy Comparison

Week 3 introduced a controlled experiment comparing two strategies using the same generated rush-demand stream.

### Strategy A: FIFO

FIFO processes the oldest available order requiring a free equipment station.

It does not combine compatible orders into shared batches.

### Strategy B: Dynamic Scheduling with Batching

Dynamic scheduling:

1. Selects the highest-urgency eligible order
2. Checks the required equipment
3. Searches for compatible waiting orders
4. Pulls qualified candidates into the batch
5. Models shared preparation time
6. Protects more urgent orders from displacement

### Simulation Assumption

The initial batching model estimates batch duration as:

```text
batch duration =
longest individual preparation time
+ one tick per additional order
```

This represents shared setup and partially overlapping preparation.

It is an initial simulation assumption and has not yet been validated in a production environment.

## Week 3 Experimental Results

A fixed-seed rush scenario generated 20 orders and passed the identical stream through both strategies.

### FIFO Results

```text
Orders completed: 20
Total simulation ticks: 78
Average wait: 17.80 ticks
Maximum wait: 57 ticks
Equipment operations: 20
Orders pulled into batches: 0
```

### Dynamic Scheduling Results

```text
Orders completed: 20
Total simulation ticks: 59
Average wait: 12.35 ticks
Maximum wait: 45 ticks
Equipment operations: 13
Orders pulled into batches: 7
```

### Improvement Summary

| Metric                |        FIFO |     Dynamic |                      Change |
| --------------------- | ----------: | ----------: | --------------------------: |
| Average wait          | 17.80 ticks | 12.35 ticks |            5.45 fewer ticks |
| Maximum wait          |    57 ticks |    45 ticks |              12 fewer ticks |
| Total simulation time |    78 ticks |    59 ticks |              19 fewer ticks |
| Equipment operations  |          20 |          13 |          7 fewer operations |
| Orders batched        |           0 |           7 | 7 additional batched orders |

For this simulated rush scenario, dynamic scheduling produced approximately:

* 31% lower average wait
* 21% lower maximum wait
* 24% shorter total simulation time
* 35% fewer equipment operations

These results demonstrate improvement within the current simulation model. They do not represent measured performance in a real store.

## Project Structure

```text
Dynamic-Workflow-Scheduler/
├── main.cpp
├── Order.h
├── Order.cpp
├── QueueManager.h
├── QueueManager.cpp
├── Scheduler.h
├── Scheduler.cpp
├── TerminalDisplay.h
├── TerminalDisplay.cpp
├── CommandProcessor.h
├── CommandProcessor.cpp
├── OrderGenerator.h
├── OrderGenerator.cpp
├── EquipmentManager.h
├── EquipmentManager.cpp
├── SimulationRunner.h
├── SimulationRunner.cpp
├── test_scheduler.cpp
├── CMakeLists.txt
└── README.md
```

## Build Instructions

### Requirements

* A C++17-compatible compiler
* GCC, MinGW, Clang, or another supported compiler
* A terminal that supports ANSI colors

### Compile the Interactive Program

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp Order.cpp QueueManager.cpp Scheduler.cpp TerminalDisplay.cpp CommandProcessor.cpp -o scheduler.exe
```

### Run on Windows PowerShell

```powershell
.\scheduler.exe
```

### Compile the Simulation Version

When using the order generator and simulation runner:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp Order.cpp OrderGenerator.cpp EquipmentManager.cpp QueueManager.cpp Scheduler.cpp SimulationRunner.cpp TerminalDisplay.cpp CommandProcessor.cpp -o scheduler.exe
```

### Compile the Tests

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic test_scheduler.cpp Order.cpp QueueManager.cpp Scheduler.cpp -o scheduler_tests.exe
```

### Run the Tests

```powershell
.\scheduler_tests.exe
```

### Build with CMake

```powershell
cmake -S . -B build
cmake --build build
```

Run the generated executable from the build directory.

## Testing

The automated test suite covers:

* Source urgency rates
* Aging and starvation prevention
* Empty queue handling
* Future timestamp handling
* Invalid source handling
* Deterministic tie-breaking
* Compatibility scoring
* Batch eligibility
* Priority-inversion protection
* Batch-size limits
* Empty scheduling decisions
* Completed-order exclusion
* Cancelled-order exclusion
* In-progress-order exclusion
* Duplicate order ID rejection
* Valid lifecycle transitions
* Invalid lifecycle transition rejection

Current scheduler, batching, prioritization, and lifecycle tests pass successfully.

The interactive mode has also been manually tested using:

```text
add → find → start → complete → quit
```

Additional command cases tested include:

```text
pause → blocked state-changing command → resume
find unknown ID
complete waiting order
start completed order
cancel terminal order
```

## Current Limitations

The current version is a prototype rather than a production-ready system.

Current limitations include:

* Manual mode and random simulation currently run as separate modes
* Manual commands are line-based rather than graphical
* The live demonstration uses simplified timing
* Equipment durations use simplified fixed or estimated values
* Batching-time savings are based on an initial assumption
* No persistent order history
* No automatic archival of completed orders
* No physical equipment-status input
* No sensor or STM32 integration
* Source urgency rates are initial assumptions
* Compatibility weights are initial assumptions
* Simulation parameters have not been calibrated using store data
* The system does not yet model worker capacity
* The system does not yet model machine failures or cleaning
* Drink modifiers and inventory constraints are not yet modeled
* Manual commands and automatic equipment timing are not yet fully integrated
* No graphical interface

## Development Roadmap

### Completed

* [x] Identify the workflow bottleneck
* [x] Define the `Order` data model
* [x] Implement `QueueManager`
* [x] Separate scheduling logic into `Scheduler`
* [x] Implement dynamic urgency scoring
* [x] Add aging to reduce starvation
* [x] Implement dynamic prioritization
* [x] Add deterministic tie-breaking
* [x] Reject duplicate order IDs
* [x] Add process-aware compatibility scoring
* [x] Implement process-aware batch selection
* [x] Enforce maximum batch size
* [x] Add order lifecycle transitions
* [x] Reject invalid lifecycle transitions safely
* [x] Add automated scheduler tests
* [x] Build a randomized order generator
* [x] Create normal-demand and rush-demand scenarios
* [x] Add estimated preparation times
* [x] Model equipment availability
* [x] Add fixed-tick equipment busy states
* [x] Implement FIFO simulation
* [x] Implement dynamic batching simulation
* [x] Compare both strategies using the same order stream
* [x] Measure wait time and equipment operations
* [x] Add command-line random seed support
* [x] Make simulation runs reproducible
* [x] Add safe handling for invalid seed input
* [x] Add live terminal visualization
* [x] Add terminal colors
* [x] Display next action and batch recommendation
* [x] Display active equipment state
* [x] Display live queue metrics
* [x] Add interactive manual mode
* [x] Add order creation command
* [x] Add start, complete, and cancel commands
* [x] Add order lookup by ID
* [x] Add pause and resume commands
* [x] Separate demonstration and manual program modes

### In Progress

* [ ] Add automated tests for command parsing
* [ ] Add simulation-specific automated tests
* [ ] Run comparisons across multiple random seeds
* [ ] Improve manual-mode input validation
* [ ] Integrate all program targets into CMake
* [ ] Refine scheduling parameters through customer discovery

### Planned

* [ ] Integrate random arrivals into the live dashboard
* [ ] Retry blocked orders automatically
* [ ] Record per-source wait-time metrics
* [ ] Compare FIFO, urgency-only, and dynamic batching
* [ ] Measure equipment utilization
* [ ] Add automatic archival and order history
* [ ] Model workers and parallel preparation capacity
* [ ] Model equipment failures and cleaning delays
* [ ] Add drink modifiers and inventory constraints
* [ ] Create a graphical interface
* [ ] Add STM32-based equipment or sensor inputs
* [ ] Explore a closed-loop embedded workflow assistant

## Project Goal

The long-term goal is to create a workflow-assistance system that reduces cognitive friction rather than replacing workers.

By externalizing routine prioritization, equipment tracking, batching, and task-sequencing decisions, the scheduler is intended to help workers focus more attention on production, communication, and customer service.

The project also demonstrates how scheduling algorithms, simulation, embedded-system concepts, software architecture, and human-centered design can be applied to a real operational problem.



