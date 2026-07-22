# Dynamic Workflow Scheduler

A modular C++ scheduling system designed to reduce cognitive load and decision latency in high-volume service workflows.

## Project Overview

The Dynamic Workflow Scheduler is a C++ prototype inspired by workflow bottlenecks I observed while working in a high-volume coffee shop environment.

During rush periods, workers must process several incoming order streams, identify urgent tickets, recognize similar preparation tasks, and continuously decide what should happen next.

The main bottleneck is not always the physical speed of production. It is often the cognitive overhead created by repeatedly sorting, prioritizing, and reorganizing work while completing physical tasks.

This project explores how a software scheduling layer can support workers by dynamically organizing incoming orders and recommending a more efficient production sequence.

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
2. Process similarity

A strict first-in, first-out queue does not account for these workflow constraints.

A rigid priority rule such as:

```text
Drive-Thru > Mobile > Eat-In
```

can also cause lower-priority orders to wait indefinitely during sustained demand.

The Dynamic Workflow Scheduler treats this as a real-time scheduling problem rather than a simple queue.

## Current Features

* Structured order modeling
* Drive-thru, mobile, and eat-in order sources
* Order timestamps
* Waiting, in-progress, and complete statuses
* Dynamic urgency scoring
* Aging to reduce starvation
* Chronological queue display
* Urgency-based prioritization
* Deterministic tie-breaking
* Initial batching compatibility logic
* Modular C++ architecture
* Terminal-based demonstration

## Architecture

The project separates order data, queue management, and scheduling policy.

```text
main.cpp
    |
    +-- QueueManager
    |       |
    |       +-- stores and manages Order objects
    |
    +-- Scheduler
            |
            +-- calculates urgency
            +-- prioritizes orders
            +-- evaluates batching compatibility

Order
    |
    +-- shared order data model
```

### Order

The `Order` structure represents one customer order.

Each order currently stores:

* Order ID
* Drink name
* Size
* Hot or iced state
* Order source
* Placement timestamp
* Preparation build key
* Current status

### QueueManager

`QueueManager` owns the order collection and provides operations for:

* Adding orders
* Completing orders
* Displaying the active queue
* Providing order data to the scheduler

### Scheduler

`Scheduler` contains the scheduling policy.

It is responsible for:

* Calculating urgency
* Sorting orders by priority
* Handling deterministic tie-breaking
* Checking whether two orders may be compatible for batching

### main.cpp

`main.cpp` currently acts as a demonstration program.

It:

* Creates sample orders
* Displays the queue chronologically
* Runs the scheduling algorithm
* Displays the dynamically prioritized queue

## Scheduling Logic

The current scheduler assigns each order an urgency score:

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

### Example

An eat-in order waiting for 200 seconds receives:

```text
0.4 × 200 = 80
```

A drive-thru order waiting for 30 seconds receives:

```text
1.0 × 30 = 30
```

The scheduler prioritizes the older eat-in order because its urgency score is higher.

### Eligibility Rules

Waiting orders are prioritized ahead of orders that are already in progress or complete.

When two orders have equal urgency, the scheduler uses:

1. Earlier placement time
2. Lower order ID

This produces deterministic output.

### buildKey

`buildKey` represents the primary equipment or preparation process required by an order, such as:

* `espresso`
* `brew`
* `frozen`
* `other`

Orders with matching build keys may be considered for batching.

For example, two drinks with the `espresso` build key may share part of the same preparation workflow.

The current prototype includes a compatibility check using `buildKey`, but it does not yet construct or execute complete production batches.

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
└── README.md
```

## Build Instructions

### Requirements

* A C++17-compatible compiler
* GCC, MinGW, Clang, or another supported C++ compiler

### Compile with g++

From the project directory, run:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -g main.cpp Order.cpp QueueManager.cpp Scheduler.cpp -o scheduler.exe
```

## Run Instructions

### Windows PowerShell

```powershell
.\scheduler.exe
```

### Linux or macOS

```bash
./scheduler
```

## Example Output

```text
BEFORE PRIORITIZATION

==================== CURRENT QUEUE ====================
1. [Eat-In] M Hot Tea  (waiting 200s, Waiting)
2. [Mobile] M Iced Latte  (waiting 120s, Waiting)
3. [Mobile] L Iced Coffee  (waiting 90s, Waiting)
4. [Mobile] M Iced Frozen Coffee  (waiting 60s, Waiting)
5. [Eat-In] L Iced Cold Brew  (waiting 45s, Waiting)
6. [Drive-Thru] M Hot Latte  (waiting 30s, Waiting)
=======================================================

AFTER DYNAMIC PRIORITIZATION

==================== CURRENT QUEUE ====================
1. [Eat-In] M Hot Tea  (waiting 200s, Waiting)
2. [Mobile] M Iced Latte  (waiting 120s, Waiting)
3. [Mobile] L Iced Coffee  (waiting 90s, Waiting)
4. [Mobile] M Iced Frozen Coffee  (waiting 60s, Waiting)
5. [Drive-Thru] M Hot Latte  (waiting 30s, Waiting)
6. [Eat-In] L Iced Cold Brew  (waiting 45s, Waiting)
=======================================================
```

The exact ordering depends on each order's source, timestamp, status, and calculated urgency.

## Current Limitations

The current version is an early scheduling prototype.

Current limitations include:

* Orders are hard-coded in `main.cpp`
* No real-time order generator
* No interactive user input
* No complete batch-construction algorithm
* No equipment availability tracking
* No estimated preparation times
* No order cancellation or modification
* No automatic archival of completed orders
* No performance metrics
* No graphical interface
* No persistent storage
* No sensor or STM32 integration
* Source urgency rates are initial assumptions
* Batching rules are not yet validated through customer discovery

The project currently demonstrates scheduling logic rather than a production-ready service system.

## Development Roadmap

### Completed

* [x] Identify the workflow bottleneck
* [x] Define the `Order` data model
* [x] Implement `QueueManager`
* [x] Separate scheduling logic into `Scheduler`
* [x] Implement dynamic urgency scoring
* [x] Add aging to reduce starvation
* [x] Implement dynamic prioritization
* [x] Add chronological queue display
* [x] Add deterministic tie-breaking
* [x] Add initial `buildKey` compatibility logic
* [x] Create a modular C++ structure
* [x] Build and debug the terminal prototype

### In Progress

* [ ] Reject duplicate order IDs
* [ ] Improve order lifecycle operations
* [ ] Add automated tests
* [ ] Add CMake build support
* [ ] Implement process-aware batch selection

### Planned

* [ ] Add finite-state-machine order transitions
* [ ] Generate orders at random intervals
* [ ] Refresh the terminal display in real time
* [ ] Track wait times and throughput
* [ ] Compare FIFO, urgency-only, and batching strategies
* [ ] Model equipment availability
* [ ] Add estimated preparation times
* [ ] Create a live terminal visualization
* [ ] Add STM32-based sensor or equipment inputs
* [ ] Explore a closed-loop embedded workflow assistant

## Project Goal

The long-term goal is to create a workflow-assistance system that reduces cognitive friction rather than replacing workers.

By externalizing routine prioritization and task-sequencing decisions, the scheduler is intended to help workers focus more attention on production, communication, and customer service.

