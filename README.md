# Dynamic Workflow Scheduler

A modular C++ workflow-scheduling simulation designed to reduce cognitive load and decision latency in high-volume service environments.

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

* Structured order modeling
* Drive-thru, mobile, and eat-in order sources
* Randomized drink, size, temperature, and preparation attributes
* Normal-demand and rush-demand order generation
* Simulated order arrival times
* Estimated preparation times
* Dynamic urgency scoring
* Aging to reduce starvation
* Deterministic priority tie-breaking
* Process-aware compatibility scoring
* Equipment-based batch selection
* Maximum batch-size enforcement
* Waiting, in-progress, complete, and cancelled order states
* Safe rejection of invalid lifecycle transitions
* Duplicate order ID rejection
* Equipment availability simulation
* Fixed-tick equipment busy states
* Concurrent equipment operation
* FIFO scheduling simulation
* Dynamic scheduling with batching
* Comparative performance metrics
* Automated scheduler and lifecycle tests
* Modular C++ architecture

## Architecture

The project separates order data, queue management, scheduling policy, demand generation, equipment state, and simulation control.

```text
main.cpp
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

Example:

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

The initial source probabilities are:

| Source     | Probability |
| ---------- | ----------: |
| Drive-Thru |         45% |
| Mobile     |         35% |
| Eat-In     |         20% |

Two demand scenarios are currently supported:

| Scenario      | Arrival interval |
| ------------- | ---------------: |
| Normal demand |    15–45 seconds |
| Rush demand   |     2–10 seconds |

A fixed random seed can be used to ensure that simulation results are reproducible.

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
* Displaying the active queue
* Passing order data to the scheduler

### Scheduler

`Scheduler` contains the scheduling policy.

It is responsible for:

* Calculating urgency
* Sorting orders by priority
* Handling deterministic tie-breaking
* Calculating compatibility scores
* Determining whether orders may batch
* Selecting a highest-urgency anchor order
* Ranking compatible batch candidates
* Enforcing a maximum batch size
* Returning a structured scheduling decision

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

When an order starts, its required station becomes busy for a fixed number of simulation ticks.

Initial fixed durations:

| Equipment        | Busy duration |
| ---------------- | ------------: |
| Espresso station |       3 ticks |
| Brew station     |       2 ticks |
| Frozen station   |       4 ticks |
| Other station    |        1 tick |

Each simulation update decreases the station’s remaining busy time. The station automatically returns to `Available` when the timer reaches zero.

Different stations may operate concurrently.

### SimulationRunner

`SimulationRunner` executes the same generated order stream using different scheduling strategies.

It currently supports:

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

The scheduler prioritizes the older eat-in order because its urgency score is higher.

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

Current values include:

* `espresso`
* `brew`
* `frozen`
* `other`

Orders with different build keys cannot batch.

Example:

```text
Latte          → espresso
Cappuccino     → espresso
Frozen Matcha  → frozen
```

A latte and cappuccino may be included in one espresso batch, while frozen matcha is excluded.

### Compatibility Score

The current compatibility model is:

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
* Its urgency gap exceeds the allowed threshold

Batching fills around the priority system rather than replacing it.

### Batch Size

The default maximum batch size is three total orders:

```text
1 anchor order
2 compatible batch candidates
```

## Scheduling Strategy Comparison

Week 3 introduced a controlled experiment comparing two strategies using the same generated rush-demand order stream.

### Strategy A: FIFO

FIFO processes the oldest available order requiring a free equipment station.

It does not combine compatible orders into shared batches.

### Strategy B: Dynamic Scheduling with Batching

Dynamic scheduling:

1. Selects the highest-urgency eligible order
2. Checks its required equipment
3. Searches for compatible waiting orders
4. Pulls qualified candidates into the batch
5. Models shared preparation time
6. Protects more urgent orders from being displaced

### Simulation Assumption

The first batching model estimates batch duration as:

```text
batch duration =
longest individual preparation time
+ one tick per additional order
```

This represents shared setup and partially overlapping preparation.

It is an initial simulation assumption and has not yet been validated in a production environment.

## Week 3 Experimental Results

A fixed-seed rush scenario generated 20 orders and passed the identical stream through both scheduling strategies.

### FIFO Results

```text
Orders completed: 20
Total simulation ticks: 78
Average wait: 17.80 ticks
Maximum wait: 57 ticks
Equipment operations: 20
Orders pulled into batches: 0
```

### Dynamic Scheduling with Batching Results

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

These results demonstrate improvement within the current simulation model. They do not yet represent measured performance in a real store.

## Order Lifecycle

The current order lifecycle is:

```text
Waiting → InProgress → Complete
    |
    └──────────────→ Cancelled
```

Valid operations include:

```text
Waiting → InProgress
InProgress → Complete
Waiting → Cancelled
InProgress → Cancelled
```

Completed and cancelled orders are treated as terminal and are excluded from new scheduling decisions.

## Project Structure

```text
Dynamic-Workflow-Scheduler/
├── main.cpp
├── Order.h
├── Order.cpp
├── OrderGenerator.h
├── OrderGenerator.cpp
├── EquipmentManager.h
├── EquipmentManager.cpp
├── QueueManager.h
├── QueueManager.cpp
├── Scheduler.h
├── Scheduler.cpp
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

### Compile the Week 3 Demo

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g main.cpp Order.cpp OrderGenerator.cpp EquipmentManager.cpp Scheduler.cpp SimulationRunner.cpp -o scheduler.exe
```

### Run on Windows PowerShell

```powershell
.\scheduler.exe
```

### Compile the Scheduler Tests

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g test_scheduler.cpp Order.cpp QueueManager.cpp Scheduler.cpp -o scheduler_tests.exe
```

### Run the Tests

```powershell
.\scheduler_tests.exe
```

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
* Completed order exclusion
* Cancelled order exclusion
* In-progress order exclusion
* Duplicate order ID rejection
* Valid lifecycle transitions
* Invalid lifecycle transition rejection

## Current Limitations

The current version is a simulation prototype rather than a production-ready system.

Current limitations include:

* The terminal simulation is not yet real-time
* Waiting orders in the standalone equipment demonstration are not retried
* Equipment durations use simplified fixed or estimated tick values
* Batching-time savings are based on an initial assumption
* No interactive order input
* No graphical interface
* No persistent order history
* No automatic archival of completed orders
* No physical equipment-status input
* No sensor or STM32 integration
* Source urgency rates are initial assumptions
* Compatibility weights are initial assumptions
* Simulation parameters have not been calibrated using store data
* Results currently come from one fixed-seed order stream
* The system does not yet model worker capacity, machine failures, cleaning, modifiers, or inventory constraints

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
* [x] Document Week 3 simulation results

### In Progress

* [ ] Add simulation-specific automated tests
* [ ] Run comparisons across multiple random seeds
* [ ] Improve terminal visualization
* [ ] Refine scheduling parameters using customer discovery
* [ ] Integrate simulation targets into CMake

### Planned

* [ ] Add real-time terminal refresh
* [ ] Retry blocked orders in the equipment demonstration
* [ ] Record per-source wait-time metrics
* [ ] Compare FIFO, urgency-only, and dynamic batching
* [ ] Measure equipment utilization
* [ ] Add automatic archival and order history
* [ ] Model workers and parallel preparation capacity
* [ ] Model equipment failures and cleaning delays
* [ ] Add interactive order input
* [ ] Create a live visualization
* [ ] Add STM32-based equipment or sensor inputs
* [ ] Explore a closed-loop embedded workflow assistant

## Project Goal

The long-term goal is to create a workflow-assistance system that reduces cognitive friction rather than replacing workers.

By externalizing routine prioritization, equipment tracking, and task-sequencing decisions, the scheduler is intended to help workers focus more attention on production, communication, and customer service.

The project also demonstrates how scheduling algorithms, embedded-system concepts, simulation, and human-centered design can be applied to a real operational problem.


The long-term goal is to create a workflow-assistance system that reduces cognitive friction rather than replacing workers.

By externalizing routine prioritization, batching, and task-sequencing decisions, the scheduler is intended to help workers focus more attention on production, communication, and customer service.


