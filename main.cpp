// Dynamic Workflow Scheduler
// Milestone 4: Process-aware scheduling decisions

#include "Order.h"
#include "QueueManager.h"
#include "Scheduler.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <vector>

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

    std::cout
        << "\nAFTER DYNAMIC PRIORITIZATION\n\n";

    queue.displayQueue(now);

    const std::vector<Order> waitingOrders =
        queue.getWaitingOrders();

    const ScheduleDecision decision =
        scheduler.makeDecision(
            waitingOrders,
            now
        );

    std::cout << "\nNEXT ACTION\n\n";

    if (decision.anchorOrderId == -1) {
        std::cout
            << "No waiting orders are available.\n";
    } else {
        const Order* anchor =
            queue.findOrderById(
                decision.anchorOrderId
            );

        std::cout
            << "Equipment: "
            << decision.equipment
            << '\n';

        std::cout
            << "Anchor Order: #"
            << decision.anchorOrderId;

        if (anchor != nullptr) {
            std::cout
                << " "
                << sourceName(anchor->source)
                << " "
                << anchor->drink;
        }

        std::cout
            << "\nAnchor Urgency: "
            << std::fixed
            << std::setprecision(1)
            << decision.anchorUrgency
            << '\n';

        std::cout << "Batch With:\n";

        if (decision.batchedOrderIds.empty()) {
            std::cout << "- None\n";
        } else {
            for (int orderId :
                 decision.batchedOrderIds) {
                const Order* batchedOrder =
                    queue.findOrderById(orderId);

                if (batchedOrder == nullptr) {
                    continue;
                }

                std::cout
                    << "- #"
                    << batchedOrder->id
                    << " "
                    << sourceName(
                           batchedOrder->source
                       )
                    << " "
                    << batchedOrder->drink;

                if (anchor != nullptr) {
                    std::cout
                        << " (compatibility "
                        << scheduler
                               .calculateCompatibility(
                                   *anchor,
                                   *batchedOrder
                               )
                        << ")";
                }

                std::cout << '\n';
            }
        }

        if (anchor != nullptr) {
            std::cout
                << "Reason: shared "
                << anchor->buildKey
                << " preparation\n";
        }
    }

    std::cout << "\nSTARTING ORDER 5\n\n";

    if (queue.startOrder(5)) {
        std::cout
            << "Order 5 is now in progress.\n";
    } else {
        std::cout
            << "Could not start order 5.\n";
    }

    queue.displayQueue(now);

    std::cout << "\nCOMPLETING ORDER 5\n\n";

    if (queue.completeOrder(5)) {
        std::cout
            << "Order 5 was completed.\n";
    } else {
        std::cout
            << "Could not complete order 5.\n";
    }

    queue.displayQueue(now);

    std::cout << "\nCANCELLING ORDER 10\n\n";

    if (queue.cancelOrder(10)) {
        std::cout
            << "Order 10 was cancelled.\n";
    } else {
        std::cout
            << "Could not cancel order 10.\n";
    }

    queue.displayQueue(now);

    std::cout
        << "\nWAITING ORDERS: "
        << queue.getWaitingOrders().size()
        << '\n';

    std::cout
        << "ACTIVE ORDERS: "
        << queue.getActiveOrders().size()
        << '\n';

    return 0;
}