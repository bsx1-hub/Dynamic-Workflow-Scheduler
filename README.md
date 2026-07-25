# Dynamic Workflow Scheduler

A modular C++ scheduling system designed to reduce cognitive load and decision latency in high-volume service workflows.

## Project Overview

The Dynamic Workflow Scheduler is a C++ prototype inspired by workflow bottlenecks I observed while working in a high-volume coffee shop environment.

During rush periods, workers must process several incoming order streams, identify urgent tickets, recognize similar preparation tasks, and continuously decide what should happen next.

The main bottleneck is not always the physical speed of production. It is often the cognitive overhead created by repeatedly sorting, prioritizing, batching, and reorganizing work while completing physical tasks.

This project explores how a software scheduling layer can support workers by dynamically prioritizing orders, identifying compatible preparation tasks, and recommending a concrete next action.

## Problem Statement

Service workers may receive orders from several concurrent sources:

* Drive-thru
* Mobile ordering
* Front-counter or eat-in customers

Each source may have a different operational priority. At the same time, orders may require different equipment or preparation processes, including:

* Espresso machine
* Brewing station
* Blender
* Other preparation stations

Workers must mentally balance:

1. Service urgency
2. Wait time
3. Process similarity
4. Order state
5. Equipment requirements

A strict first-in, first-out queue does not account for these workflow constraints.

A rigid rule such as:

```text
Drive-Thru > Mobile > Eat-In
```

can also cause lower-priority orders to wait indefinitely during sustained demand.

The Dynamic Workflow Scheduler treats this as a real-time scheduling problem rather than a simple queue.

## Current Features

* Structured order modeling
* Drive-thru, mobile, and eat-in order sources
* Order timestamps
* Dynamic urgency scoring
* Aging to reduce starvation
* Urgency-based prioritization
* Deterministic tie-breaking
* Process-aware compatibility scoring
* Equipment-based batch selection
* Structured scheduling decisions
* Maximum batch-size enforcement
* Order lifecycle finite-state machine
* Safe rejection of invalid state transitions
* Completed and cancelled order filtering
* Duplicate order ID rejection
* Queue-state lookup and management
* Terminal-based demonstration
* Automated scheduler and lifecycle tests
* Modular C++ architecture

## Architecture

The project separates order data, queue management, scheduling policy, and system coordination.

```text
main.cpp
    |
    +-- QueueManager
    |       |
    |       +-- owns Order objects
    |       +-- enforces lifecycle transitions
    |       +-- provides waiting and active order views
    |
    +-- Scheduler
            |
            +-- calculates urgency
            +-- prioritizes orders
            +-- scores compatibility
            +-- selects batch candidates
            +-- returns ScheduleDecision

Order
    |
    +-- shared order data model
    +-- source and lifecycle enums
```

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

### QueueManager

`QueueManager` owns the order collection and provides operations for:

* Adding orders
* Rejecting duplicate IDs
* Finding orders by ID
* Removing orders
* Scheduling orders
* Starting orders
* Completing orders
* Cancelling orders
* Returning waiting orders
* Returning active orders
* Displaying the active queue
* Passing order data to the scheduler

`QueueManager` also enforces the order lifecycle state machine so invalid transitions fail safely.

### Scheduler

`Scheduler` contains the scheduling policy.

It is responsible for:

* Calculating urgency
* Sorting orders by priority
* Handling deterministic tie-breaking
* Calculating compatibility scores
* Determining whether two orders may batch
* Selecting a highest-urgency anchor order
* Ranking compatible batch candidates
* Enforcing a maximum batch size
* Returning a structured scheduling decision

### ScheduleDecision

The scheduler returns a `ScheduleDecision` containing:

```cpp
struct ScheduleDecision {
    int anchorOrderId;
    std::vector<int> batchedOrderIds;
    std::string equipment;
    double anchorUrgency;
};
```

This separates scheduling policy from queue ownership.

The scheduler recommends what should happen next, while `QueueManager` remains responsible for changing order state.

### main.cpp

`main.cpp` acts as a demonstration program.

It:

* Creates sample orders
* Displays the original queue
* Runs dynamic prioritization
* Requests the next scheduling decision
* Displays the recommended equipment station
* Displays the anchor order
* Displays compatible batch candidates
* Demonstrates valid lifecycle transitions
* Demonstrates cancellation behavior
* Reports waiting and active order counts

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

However, urgency also increases with wait time. This means an older mobile or eat-in order can eventually overtake a newer drive-thru order.

This aging behavior helps prevent lower-priority order streams from being indefinitely starved.

### Urgency Example

An eat-in order waiting for 200 seconds receives:

```text
0.4 × 200 = 80
```

A drive-thru order waiting for 30 seconds receives:

```text
1.0 × 30 = 30
```

The scheduler prioritizes the older eat-in order because its urgency score is higher.

### Priority Tie-Breaking

When two waiting orders have equal urgency, the scheduler uses:

1. Earlier placement time
2. Lower order ID

This produces deterministic output.

## Process-Aware Batching

The scheduler selects the highest-urgency waiting order as the anchor.

It then searches for compatible waiting orders that can be prepared alongside the anchor without causing excessive priority inversion.

### buildKey

`buildKey` represents the primary equipment or preparation process required by an order.

Current values include:

* `espresso`
* `brew`
* `frozen`
* `other`

Orders with different build keys cannot batch.

For example:

```text
Latte       → espresso
Cappuccino  → espresso
Frozen Matcha → frozen
```

A latte and cappuccino may be considered together, while a frozen matcha is excluded from the espresso batch.

### Compatibility Score

The current compatibility model is:

```text
Same buildKey:     +40
Same temperature:  +10
Same drink:        +10
```

Possible compatibility scores include:

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

Batching cannot override the main priority system.

A candidate is rejected when:

* It uses a different build key
* It is not waiting
* It is the same order as the anchor
* Its compatibility score is too low
* It is more urgent than the anchor
* Its urgency gap from the anchor exceeds the allowed threshold

This prevents batching from causing severe priority inversion.

### Batch Size

The default maximum batch size is three total orders:

```text
1 anchor order
2 compatible batch candidates
```

The limit is configurable when requesting a scheduling decision.

## Order Lifecycle State Machine

Each order moves through an explicit lifecycle.

```text
Waiting → Scheduled → InProgress → Completed
    │           │
    └──────────→ Cancelled
```

### Valid Transitions

```text
Waiting → Scheduled
Scheduled → InProgress
InProgress → Completed
Waiting → Cancelled
Scheduled → Cancelled
```

### Invalid Transitions

Invalid transitions return `false` and leave the order unchanged.

Examples include:

```text
Waiting → InProgress
Waiting → Completed
Completed → InProgress
Completed → Scheduled
Cancelled → Scheduled
Cancelled → InProgress
InProgress → Cancelled
```

`Completed` and `Cancelled` are terminal states.

## Eligibility Rules

Only orders with the `Waiting` status are considered for new scheduling decisions.

Orders with these statuses are excluded:

* Scheduled
* InProgress
* Completed
* Cancelled

Completed and cancelled orders are also hidden from the active queue display.

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
├── test_scheduler.cpp
└── README.md
```

## Build Instructions

### Requirements

* A C++17-compatible compiler
* GCC, MinGW, Clang, or another supported compiler

### Compile the Demo

From the project directory:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g main.cpp Order.cpp QueueManager.cpp Scheduler.cpp -o scheduler.exe
```

### Compile the Tests

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g test_scheduler.cpp Order.cpp QueueManager.cpp Scheduler.cpp -o scheduler_tests.exe
```

## Run Instructions

### Windows PowerShell

Run the demo:

```powershell
.\scheduler.exe
```

Run the tests:

```powershell
.\scheduler_tests.exe
```

### Linux or macOS

Run the demo:

```bash
./scheduler
```

Run the tests:

```bash
./scheduler_tests
```

## Example Scheduling Output

```text
NEXT ACTION

Equipment: Espresso Station
Anchor Order: #12 Drive-Thru Latte
Anchor Urgency: 70.0
Batch With:
- #18 Mobile Latte (compatibility 60)
- #15 Mobile Cappuccino (compatibility 50)
Reason: shared espresso preparation
```

The exact decision depends on:

* Order source
* Waiting time
* Lifecycle status
* Build key
* Drink temperature
* Drink type
* Maximum batch size

## Testing

The project currently includes automated tests for urgency, prioritization, batching, scheduling decisions, and lifecycle behavior.

The test suite covers:

* Drive-thru, mobile, and eat-in urgency rates
* Aging and starvation prevention
* Empty queue handling
* Future timestamp handling
* Invalid source handling
* Deterministic tie-breaking
* Compatibility scoring
* Matching espresso orders batching together
* Frozen orders being excluded from espresso batches
* Completed order exclusion
* Cancelled order exclusion
* In-progress order exclusion
* Batch-size limits
* Priority-inversion protection
* Anchor-only decisions
* Empty scheduling decisions
* Duplicate order ID rejection
* Valid lifecycle transitions
* Invalid lifecycle transition rejection
* Terminal-state behavior

Current result:

```text
53/53 tests passed
```

## Current Limitations

The current version is still an early scheduling prototype.

Current limitations include:

* Orders are hard-coded in `main.cpp`
* No real-time order generator
* No interactive user input
* No equipment availability tracking
* No estimated preparation times
* No automatic archival of completed orders
* No performance metrics
* No graphical interface
* No persistent storage
* No sensor or STM32 integration
* Source urgency rates are initial assumptions
* Compatibility weights are initial assumptions
* The urgency-gap threshold is not yet validated through customer discovery
* The scheduler recommends batches but does not model simultaneous physical execution

The project demonstrates scheduling logic rather than a production-ready service system.

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
* [x] Return structured scheduling decisions
* [x] Enforce maximum batch size
* [x] Add finite-state-machine order transitions
* [x] Reject invalid lifecycle transitions safely
* [x] Exclude completed and cancelled orders
* [x] Add automated batching and FSM tests
* [x] Build and debug the terminal prototype
* [x] Document the project on GitHub

### In Progress

* [ ] Add CMake test integration
* [ ] Improve terminal visualization
* [ ] Refine scheduling parameters using customer discovery
* [ ] Add scheduling and lifecycle metrics

### Planned

* [ ] Generate orders at random intervals
* [ ] Refresh the terminal display in real time
* [ ] Track wait times and throughput
* [ ] Compare FIFO, urgency-only, and batching strategies
* [ ] Model equipment availability
* [ ] Add estimated preparation times
* [ ] Add automatic archival and order history
* [ ] Create a live terminal visualization
* [ ] Add STM32-based sensor or equipment inputs
* [ ] Explore a closed-loop embedded workflow assistant

## Project Goal

The long-term goal is to create a workflow-assistance system that reduces cognitive friction rather than replacing workers.

By externalizing routine prioritization, batching, and task-sequencing decisions, the scheduler is intended to help workers focus more attention on production, communication, and customer service.


