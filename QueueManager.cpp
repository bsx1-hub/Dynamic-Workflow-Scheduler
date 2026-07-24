#include "QueueManager.h"
#include "Scheduler.h"

#include <algorithm>
#include <iostream>

Order* QueueManager::findMutableOrderById(int id) {
    auto match = std::find_if(
        orders.begin(),
        orders.end(),
        [id](const Order& order) {
            return order.id == id;
        }
    );

    if (match == orders.end()) {
        return nullptr;
    }

    return &(*match);
}

bool QueueManager::addOrder(const Order& order) {
    if (findOrderById(order.id) != nullptr) {
        return false;
    }

    Order newOrder = order;
    newOrder.status = Status::Waiting;

    orders.push_back(newOrder);
    return true;
}

const Order* QueueManager::findOrderById(int id) const {
    auto match = std::find_if(
        orders.cbegin(),
        orders.cend(),
        [id](const Order& order) {
            return order.id == id;
        }
    );

    if (match == orders.cend()) {
        return nullptr;
    }

    return &(*match);
}

bool QueueManager::removeOrder(int id) {
    auto match = std::find_if(
        orders.begin(),
        orders.end(),
        [id](const Order& order) {
            return order.id == id;
        }
    );

    if (match == orders.end()) {
        return false;
    }

    orders.erase(match);
    return true;
}

bool QueueManager::startOrder(int id) {
    Order* order = findMutableOrderById(id);

    if (order == nullptr ||
        order->status != Status::Waiting) {
        return false;
    }

    order->status = Status::InProgress;
    return true;
}

bool QueueManager::completeOrder(int id) {
    Order* order = findMutableOrderById(id);

    if (order == nullptr ||
        order->status != Status::InProgress) {
        return false;
    }

    order->status = Status::Complete;
    return true;
}

bool QueueManager::cancelOrder(int id) {
    Order* order = findMutableOrderById(id);

    if (order == nullptr ||
        order->status == Status::Complete ||
        order->status == Status::Cancelled) {
        return false;
    }

    order->status = Status::Cancelled;
    return true;
}

std::vector<Order> QueueManager::getWaitingOrders() const {
    std::vector<Order> waitingOrders;

    for (const Order& order : orders) {
        if (order.status == Status::Waiting) {
            waitingOrders.push_back(order);
        }
    }

    return waitingOrders;
}

std::vector<Order> QueueManager::getActiveOrders() const {
    std::vector<Order> activeOrders;

    for (const Order& order : orders) {
        if (order.status == Status::Waiting ||
            order.status == Status::InProgress) {
            activeOrders.push_back(order);
        }
    }

    return activeOrders;
}

void QueueManager::prioritize(
    const Scheduler& scheduler,
    time_t now
) {
    scheduler.prioritize(orders, now);
}

void QueueManager::displayQueue(time_t now) const {
    std::cout
        << "==================== CURRENT QUEUE ====================\n";

    int position = 1;

    for (const Order& order : orders) {
        if (order.status == Status::Complete ||
            order.status == Status::Cancelled) {
            continue;
        }

        long waited = static_cast<long>(
            difftime(now, order.placedAt)
        );

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