// Dynamic Workflow Scheduler
// Week 4: Live terminal visualization and user commands

#include "CommandProcessor.h"
#include "Order.h"
#include "QueueManager.h"
#include "Scheduler.h"
#include "TerminalDisplay.h"
#include "StatusDemo.h"
#include <cstring>

#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

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
    const std::time_t now = std::time(nullptr);

    queue.prioritize(scheduler, now);

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

    if (!queue.startOrder(decision.anchorOrderId)) {
        return false;
    }

    for (int orderId : decision.batchedOrderIds) {
        queue.startOrder(orderId);
    }

    return true;
}

void completeDecision(
    QueueManager& queue,
    const ScheduleDecision& decision
) {
    if (decision.anchorOrderId != -1) {
        queue.completeOrder(decision.anchorOrderId);
    }

    for (int orderId : decision.batchedOrderIds) {
        queue.completeOrder(orderId);
    }
}

std::size_t totalBatchSize(
    const ScheduleDecision& decision
) {
    if (decision.anchorOrderId == -1) {
        return 0;
    }

    return 1 + decision.batchedOrderIds.size();
}

void seedDemoQueue(
    QueueManager& queue,
    std::time_t startTime
) {
    queue.addOrder({
        18,
        "Latte",
        "M",
        true,
        Source::DriveThru,
        startTime - 42,
        "espresso",
        Status::Waiting,
        90
    });

    queue.addOrder({
        21,
        "Cappuccino",
        "M",
        true,
        Source::Mobile,
        startTime - 65,
        "espresso",
        Status::Waiting,
        85
    });

    queue.addOrder({
        22,
        "Coffee",
        "L",
        false,
        Source::EatIn,
        startTime - 89,
        "brew",
        Status::Waiting,
        35
    });

    queue.addOrder({
        23,
        "Frozen Matcha",
        "L",
        false,
        Source::DriveThru,
        startTime - 12,
        "frozen",
        Status::Waiting,
        110
    });
}

void runLiveDemo() {
    const std::time_t simulationStart = std::time(nullptr);

    QueueManager queue;
    Scheduler scheduler;
    TerminalDisplay display;

    seedDemoQueue(queue, simulationStart);

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
        "espresso",
        Status::Waiting,
        40
    });

    refresh(
        display,
        queue,
        scheduler,
        simulationStart,
        "New order arrived: #24 Drive-Thru Espresso"
    );

    pauseForDemo();

    const ScheduleDecision decision =
        scheduler.makeDecision(
            queue.getWaitingOrders(),
            std::time(nullptr)
        );

    if (startDecision(queue, decision)) {
        refresh(
            display,
            queue,
            scheduler,
            simulationStart,
            "Batch started at " + decision.equipment +
                ": " +
                std::to_string(totalBatchSize(decision)) +
                " order(s)"
        );

        pauseForDemo();

        completeDecision(queue, decision);

        refresh(
            display,
            queue,
            scheduler,
            simulationStart,
            "Batch completed: " +
                std::to_string(totalBatchSize(decision)) +
                " order(s); schedule recalculated"
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

    std::cout << "\nPress Enter to return to the mode menu...";
    std::string ignored;
    std::getline(std::cin, ignored);
}

void runManualMode() {
    const std::time_t simulationStart = std::time(nullptr);

    QueueManager queue;
    Scheduler scheduler;
    TerminalDisplay display;

    CommandProcessor commands(
        queue,
        scheduler,
        display,
        simulationStart
    );

    commands.run();
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 2 &&
        (std::strcmp(argv[1], "--demo") == 0 ||
         std::strcmp(argv[1], "--simulate-status") == 0)) {
        return runAutomatedStatusDemo(false);
    }

    if (argc == 2 && std::strcmp(argv[1], "--demo-fast") == 0) {
        return runAutomatedStatusDemo(true);
    }
    TerminalDisplay::enableAnsiColors();

    while (true) {
        TerminalDisplay::clear();

        std::cout
            << "DYNAMIC WORKFLOW SCHEDULER\n\n"
            << "Choose a program mode:\n"
            << "1. Live demonstration\n"
            << "2. Manual command mode\n"
            << "3. Quit\n\n"
            << "> ";

        std::string choice;

        if (!std::getline(std::cin, choice)) {
            break;
        }

        if (choice == "1") {
            runLiveDemo();
        } else if (choice == "2") {
            runManualMode();
        } else if (choice == "3" || choice == "quit") {
            break;
        }
    }

    TerminalDisplay::clear();
    std::cout << "Dynamic Workflow Scheduler closed.\n";

    return 0;
}