#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "EquipmentManager.h"
#include "Order.h"

#include <cstddef>
#include <ctime>
#include <string>
#include <vector>

struct ScheduleDecision {
    int anchorOrderId = -1;
    std::vector<int> batchedOrderIds;
    std::string equipment;
    double anchorUrgency = 0.0;
};

class Scheduler {
public:
    double urgency(const Order& order, std::time_t now) const;

    double calculateCompatibility(
        const Order& anchor,
        const Order& candidate
    ) const;

    void prioritize(
        std::vector<Order>& orders,
        std::time_t now
    ) const;

    bool canBatch(
        const Order& anchor,
        const Order& candidate,
        std::time_t now
    ) const;

    // Used by simulations where all stations are considered available.
    ScheduleDecision makeDecision(
        const std::vector<Order>& orders,
        std::time_t now,
        std::size_t maxBatchSize = 3
    ) const;

    // Used by the live scheduler. Orders requiring unavailable equipment are
    // excluded before the anchor and batch are selected.
    ScheduleDecision makeDecision(
        const std::vector<Order>& orders,
        const EquipmentManager& equipmentManager,
        std::time_t now,
        std::size_t maxBatchSize = 3
    ) const;
};

#endif