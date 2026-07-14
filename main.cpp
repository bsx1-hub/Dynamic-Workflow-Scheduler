// Dynamic Workflow Scheduler
// Milestone 2: Dynamic order prioritization

#include "Order.h"
#include "QueueManager.h"
#include "Scheduler.h"

#include <iostream>
#include <ctime>

int main() {
    time_t now = time(nullptr);

    QueueManager queue;
    Scheduler scheduler;

    queue.addOrder({
        1,
        "Latte",
        "M",
        true,
        Source::DriveThru,
        now - 30,
        "espresso"
    });

    queue.addOrder({
        2,
        "Coffee",
        "L",
        false,
        Source::Mobile,
        now - 90,
        "brew"
    });

    queue.addOrder({
        3,
        "Espresso",
        "S",
        true,
        Source::DriveThru,
        now - 10,
        "espresso"
    });

    queue.addOrder({
        4,
        "Frozen Coffee",
        "M",
        false,
        Source::Mobile,
        now - 60,
        "frozen"
    });

    queue.addOrder({
        5,
        "Tea",
        "M",
        true,
        Source::EatIn,
        now - 200,
        "brew"
    });

    queue.addOrder({
        6,
        "Cold Brew",
        "L",
        false,
        Source::EatIn,
        now - 45,
        "brew"
    });

    queue.addOrder({
        7,
        "Cappuccino",
        "S",
        true,
        Source::Mobile,
        now - 20,
        "espresso"
    });

    queue.addOrder({
        8,
        "Frozen Matcha",
        "L",
        false,
        Source::DriveThru,
        now - 5,
        "frozen"
    });

    queue.addOrder({
        9,
        "Latte",
        "M",
        false,
        Source::Mobile,
        now - 120,
        "espresso"
    });

    queue.addOrder({
        10,
        "Chocolate",
        "S",
        true,
        Source::EatIn,
        now - 15,
        "other"
    });

    std::cout << "BEFORE PRIORITIZATION\n\n";

    queue.displayQueue(now);

    scheduler.prioritize(
        queue.getOrders(),
        now
    );

    std::cout << "\nAFTER DYNAMIC PRIORITIZATION\n\n";

    queue.displayQueue(now);

    return 0;
}