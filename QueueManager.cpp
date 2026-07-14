#include "QueueManager.h"

#include <iostream>

void QueueManager::addOrder(const Order& order) {
    orders.push_back(order);
}

void QueueManager::completeOrder(int id) {
    for (Order& order : orders) {
        if (order.id == id) {
            order.status = Status::Complete;
            return;
        }
    }
}

void QueueManager::displayQueue(time_t now) const {
    std::cout
        << "==================== CURRENT QUEUE ====================\n";

    int position = 1;

    for (const Order& order : orders) {
        if (order.status == Status::Complete) {
            continue;
        }

        long waited = now - order.placedAt;

        std::cout
            << position++ << ". "
            << "[" << sourceName(order.source) << "] "
            << order.size << " "
            << (order.hot ? "Hot" : "Iced") << " "
            << order.drink
            << "  (waiting "
            << waited << "s, "
            << statusName(order.status)
            << ")\n";
    }

    std::cout
        << "=======================================================\n";
}

std::vector<Order>& QueueManager::getOrders() {
    return orders;
}