// Dynamic Workflow Scheduler
// Week 4: Live terminal visualization

#include "Order.h"
#include "QueueManager.h"
#include "Scheduler.h"
#include "TerminalDisplay.h"

#include <chrono>
#include <ctime>
#include <string>
#include <thread>
#include <iostream>

namespace {

void pauseForDemo() {
    std::this_thread::sleep_for(
        std::chrono::seconds(4)
    );
}

void refresh(
    TerminalDisplay& display,
    QueueManager& queue,
    const Scheduler& scheduler,
    std::time_t simulationStart,
    const std::string& eventMessage
) {
    const std::time_t now =
        std::time(nullptr);

    queue.prioritize(
        scheduler,
        now
    );

    display.render(
        queue,
        scheduler,
        simulationStart,
        now,
        eventMessage
    );
}

bool startDecision(
    QueueManager& queue,
    const ScheduleDecision& decision
) {
    if (decision.anchorOrderId == -1) {
        return false;
    }

    if (!queue.startOrder(
            decision.anchorOrderId
        )) {
        return false;
    }

    for (int orderId :
         decision.batchedOrderIds) {
        queue.startOrder(orderId);
    }

    return true;
}

void completeDecision(
    QueueManager& queue,
    const ScheduleDecision& decision
) {
    if (decision.anchorOrderId != -1) {
        queue.completeOrder(
            decision.anchorOrderId
        );
    }

    for (int orderId :
         decision.batchedOrderIds) {
        queue.completeOrder(orderId);
    }
}

std::string describeStartedBatch(
    const ScheduleDecision& decision
) {
    const std::size_t totalBatchSize =
        decision.anchorOrderId == -1
            ? 0
            : 1 +
                  decision
                      .batchedOrderIds
                      .size();

    return
        "Batch started at " +
        decision.equipment +
        ": anchor #" +
        std::to_string(
            decision.anchorOrderId
        ) +
        ", total orders " +
        std::to_string(
            totalBatchSize
        );
}

std::string describeCompletedBatch(
    const ScheduleDecision& decision
) {
    const std::size_t totalBatchSize =
        decision.anchorOrderId == -1
            ? 0
            : 1 +
                  decision
                      .batchedOrderIds
                      .size();

    return
        "Batch completed: " +
        std::to_string(
            totalBatchSize
        ) +
        " order(s); schedule recalculated";
}

} // namespace

int main() {
    TerminalDisplay::enableAnsiColors();

    const std::time_t simulationStart =
        std::time(nullptr);

    QueueManager queue;
    Scheduler scheduler;
    TerminalDisplay display;

    queue.addOrder({
        18,
        "Latte",
        "M",
        true,
        Source::DriveThru,
        simulationStart - 42,
        "espresso"
    });

    queue.addOrder({
        21,
        "Cappuccino",
        "M",
        true,
        Source::Mobile,
        simulationStart - 65,
        "espresso"
    });

    queue.addOrder({
        22,
        "Coffee",
        "L",
        false,
        Source::EatIn,
        simulationStart - 89,
        "brew"
    });

    queue.addOrder({
        23,
        "Frozen Matcha",
        "L",
        false,
        Source::DriveThru,
        simulationStart - 12,
        "frozen"
    });

    refresh(
        display,
        queue,
        scheduler,
        simulationStart,
        "Initial schedule calculated"
    );

    pauseForDemo();

    queue.addOrder({
        24,
        "Espresso",
        "S",
        true,
        Source::DriveThru,
        std::time(nullptr),
        "espresso"
    });

    refresh(
        display,
        queue,
        scheduler,
        simulationStart,
        "New order arrived: #24 Drive-Thru Espresso"
    );

    pauseForDemo();

    const std::time_t decisionTime =
        std::time(nullptr);

    const ScheduleDecision activeDecision =
        scheduler.makeDecision(
            queue.getWaitingOrders(),
            decisionTime
        );

    if (startDecision(
            queue,
            activeDecision
        )) {
        refresh(
            display,
            queue,
            scheduler,
            simulationStart,
            describeStartedBatch(
                activeDecision
            )
        );

        pauseForDemo();

        completeDecision(
            queue,
            activeDecision
        );

        refresh(
            display,
            queue,
            scheduler,
            simulationStart,
            describeCompletedBatch(
                activeDecision
            )
        );
    } else {
        refresh(
            display,
            queue,
            scheduler,
            simulationStart,
            "No valid batch was available to start"
        );
    }

    std::cout << "\nPress Enter to exit...";
    std::cin.get();
    
    return 0;
}