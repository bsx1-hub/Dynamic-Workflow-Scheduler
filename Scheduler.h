#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Order.h"

#include <ctime>
#include <vector>

struct BatchResult {
    std::vector<Order> orders;
};

class Scheduler {
public:
    double urgency(const Order& order, time_t now) const;

    double calculateCompatibility(
        const Order& anchor,
        const Order& candidate
    ) const;

    void prioritize(
        std::vector<Order>& orders,
        time_t now
    ) const;

    bool canBatch(
        const Order& anchor,
        const Order& candidate,
        time_t now
    ) const;

    BatchResult buildNextBatch(
        std::vector<Order> waitingOrders,
        time_t now
    ) const;
};

#endif