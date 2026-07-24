#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "Order.h"

#include <ctime>
#include <vector>
#include <cstddef>
#include <string>

struct ScheduleDecision { //packages the scheduler's recc into one object
    int anchorOrderId = -1; //-1 means no valid anchor, such as when the queue has no waiting orders
    std::vector<int> batchedOrderIds; //stores the IDs of compatible orders that can be made alongside the anchor
    std::string equipment; //expresso, brewing, blender station
    double anchorUrgency = 0.0; //records the anxhor order's urgency score so UI can explain selection choice
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

    ScheduleDecision makeDecision( //accepts available orders, current time, max total number of orders in batch
    const std::vector<Order>& orders,
    time_t now,
    std::size_t maxBatchSize = 3 //default: one anchor, up to 2 batch candidates
) const;
};

#endif