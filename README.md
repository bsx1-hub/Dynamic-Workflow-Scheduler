# Dynamic Workflow Scheduler

A C++ scheduling system designed to reduce cognitive load and decision latency in high-volume service workflows.

## Overview

The Dynamic Workflow Scheduler is a real-time order prioritization system inspired by my experience working in a high-volume coffee shop environment.

During rush periods, I observed that a major workflow bottleneck was not always the physical speed of drink production. Instead, workers were required to continuously process multiple incoming order streams, reprioritize tickets, identify similar tasks, and determine what action should happen next.

In other words, the workflow required workers to repeatedly perform a scheduling algorithm mentally while simultaneously completing physical tasks.

This project explores how software can offload part of that decision-making process.

The scheduler models incoming orders as structured data and dynamically determines their production priority. The long-term goal is to reduce context switching and allow workers to focus on execution rather than continuously reorganizing the workflow in their heads.

## The Problem

During a service rush, workers may receive orders from several concurrent sources:

- Drive-thru
- Mobile ordering
- Front-counter or eat-in customers

These streams have different operational priorities.

At the same time, each drink may require a different production process or piece of equipment:

- Espresso machine
- Brewing station
- Blender
- Other preparation processes

The worker must mentally evaluate both **service urgency** and **process similarity**.

A strict first-in, first-out queue does not capture these workflow constraints. However, a rigid priority system such as:

`Drive-Thru > Mobile > Eat-In`

can cause lower-priority order streams to wait indefinitely during sustained demand.

The Dynamic Workflow Scheduler treats this as a scheduling problem.

## Current Scheduling Algorithm

The current prototype assigns each order a dynamically increasing urgency score:

`urgency = source rate × seconds waiting`

Initial source rates are:

| Order Source | Urgency Rate |
|---|---:|
| Drive-Thru | 1.0 |
| Mobile | 0.6 |
| Eat-In | 0.4 |

Drive-thru orders accumulate urgency more quickly because they are more time-sensitive within the modeled workflow.

However, urgency also increases with wait time. An older mobile or eat-in order can eventually overtake a newer drive-thru order.

This aging behavior helps prevent lower-priority order streams from being indefinitely starved.

### Example

An eat-in order waiting for 200 seconds receives:

`0.4 × 200 = 80`

A drive-thru order waiting for 30 seconds receives:

`1.0 × 30 = 30`

The scheduler prioritizes the older eat-in order despite the higher base urgency rate of the drive-thru stream.

This creates a dynamic queue rather than a fixed priority hierarchy.

## Architecture

The prototype uses a modular C++ architecture:

```text
main.cpp
    |
    +-- QueueManager
    |       |
    |       +-- owns and manages Order objects
    |
    +-- Scheduler
            |
            +-- calculates urgency
            +-- dynamically prioritizes orders

Order
    |
    +-- shared order data model
