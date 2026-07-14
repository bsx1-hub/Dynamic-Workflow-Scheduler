// no actual algorithm here. interface/contract. Scheduler can calcualte urgency and prioritize orders.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Order.h"

#include <vector>
#include <ctime>

class Scheduler {
public:
    double urgency(const Order& order, time_t now) const;

    void prioritize(
        std::vector<Order>& orders,
        time_t now
    ) const;
};

#endif