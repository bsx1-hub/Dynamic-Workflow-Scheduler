// Dynamic Workflow Scheduler
// Milestone 4: Process-aware batch selection

#include "Order.h"
#include "QueueManager.h"
#include "Scheduler.h"

#include <ctime>
#include <iostream>

int main() {
    const time_t now = time(nullptr);

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

    queue.prioritize(scheduler, now);

    std::cout << "\nAFTER DYNAMIC PRIORITIZATION\n\n";
    queue.displayQueue(now);

    const BatchResult batch = scheduler.buildNextBatch(
        queue.getWaitingOrders(),
        now
    );

    std::cout << "\nNEXT RECOMMENDED BATCH\n\n";

    if (batch.orders.empty()) {
        std::cout << "No waiting orders are available.\n";
    } else {
        for (std::size_t i = 0; i < batch.orders.size(); ++i) {
    const Order& order = batch.orders[i];

    if (i == 0) {
        std::cout << "Anchor: ";
    } else {
        std::cout
            << "Compatible (score "
            << scheduler.calculateCompatibility(
                   batch.orders.front(),
                   order
               )
            << "): ";
    }

    std::cout
        << "Order " << order.id << " - "
        << "[" << sourceName(order.source) << "] "
        << order.size << " "
        << (order.hot ? "Hot " : "Iced ")
        << order.drink
        << " - "
        << order.buildKey
        << '\n';
}
    }

    std::cout << "\nSTARTING ORDER 5\n\n";

    if (queue.startOrder(5)) {
        std::cout << "Order 5 is now in progress.\n";
    } else {
        std::cout << "Could not start order 5.\n";
    }

    queue.displayQueue(now);

    std::cout << "\nCOMPLETING ORDER 5\n\n";

    if (queue.completeOrder(5)) {
        std::cout << "Order 5 was completed.\n";
    } else {
        std::cout << "Could not complete order 5.\n";
    }

    queue.displayQueue(now);

    std::cout << "\nCANCELLING ORDER 10\n\n";

    if (queue.cancelOrder(10)) {
        std::cout << "Order 10 was cancelled.\n";
    } else {
        std::cout << "Could not cancel order 10.\n";
    }

    queue.displayQueue(now);

    std::cout << "\nWAITING ORDERS: "
              << queue.getWaitingOrders().size()
              << '\n';

    std::cout << "ACTIVE ORDERS: "
              << queue.getActiveOrders().size()
              << '\n';

              std::cout << scheduler.calculateCompatibility(
    queue.getWaitingOrders()[0],
    queue.getWaitingOrders()[1]
) << '\n';
    return 0;
}