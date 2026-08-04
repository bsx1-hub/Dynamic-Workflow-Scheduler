#include "Scheduler.h"

#include <algorithm>

namespace {

std::string equipmentName(const std::string& buildKey) {
    if (buildKey == "espresso") {
        return "Espresso Station";
    }

    if (buildKey == "brew") {
        return "Brewing Station";
    }

    if (buildKey == "frozen") {
        return "Blender Station";
    }

    return "Other Station";
}

} // namespace

double Scheduler::urgency(const Order& order, std::time_t now) const {
    double rate = 0.0;

    switch (order.source) {
        case Source::DriveThru:
            rate = 1.0;
            break;
        case Source::Mobile:
            rate = 0.6;
            break;
        case Source::EatIn:
            rate = 0.4;
            break;
    }

    return rate * std::max(0.0, std::difftime(now, order.placedAt));
}

double Scheduler::calculateCompatibility(
    const Order& anchor,
    const Order& candidate
) const {
    if (anchor.buildKey.empty() || candidate.buildKey.empty() ||
        anchor.buildKey != candidate.buildKey) {
        return 0.0;
    }

    double score = 40.0;
    if (anchor.hot == candidate.hot) {
        score += 10.0;
    }
    if (anchor.drink == candidate.drink) {
        score += 10.0;
    }
    return score;
}

void Scheduler::prioritize(
    std::vector<Order>& orders,
    std::time_t now
) const {
    std::sort(orders.begin(), orders.end(), [this, now](const Order& a, const Order& b) {
        const bool aWaiting = a.status == Status::Waiting;
        const bool bWaiting = b.status == Status::Waiting;
        if (aWaiting != bWaiting) {
            return aWaiting;
        }

        const double aUrgency = urgency(a, now);
        const double bUrgency = urgency(b, now);
        if (aUrgency != bUrgency) {
            return aUrgency > bUrgency;
        }
        if (a.placedAt != b.placedAt) {
            return a.placedAt < b.placedAt;
        }
        return a.id < b.id;
    });
}

bool Scheduler::canBatch(
    const Order& anchor,
    const Order& candidate,
    std::time_t now
) const {
    constexpr double minimumCompatibility = 40.0;
    constexpr double maximumUrgencyGap = 20.0;

    if (anchor.id == candidate.id || anchor.status != Status::Waiting ||
        candidate.status != Status::Waiting ||
        calculateCompatibility(anchor, candidate) < minimumCompatibility) {
        return false;
    }

    const double anchorUrgency = urgency(anchor, now);
    const double candidateUrgency = urgency(candidate, now);
    return candidateUrgency <= anchorUrgency &&
           anchorUrgency - candidateUrgency <= maximumUrgencyGap;
}

ScheduleDecision Scheduler::makeDecision(
    const std::vector<Order>& orders,
    std::time_t now,
    std::size_t maxBatchSize
) const {
    EquipmentManager allStationsAvailable;
    return makeDecision(orders, allStationsAvailable, now, maxBatchSize);
}

ScheduleDecision Scheduler::makeDecision(
    const std::vector<Order>& orders,
    const EquipmentManager& equipmentManager,
    std::time_t now,
    std::size_t maxBatchSize
) const {
    ScheduleDecision decision;
    std::vector<Order> waitingOrders;

    for (const Order& order : orders) {
        const EquipmentType requiredEquipment =
            equipmentTypeFromBuildKey(order.buildKey);

        if (order.status == Status::Waiting &&
            equipmentManager.isAvailable(requiredEquipment)) {
            waitingOrders.push_back(order);
        }
    }

    if (waitingOrders.empty()) {
        return decision;
    }

    prioritize(waitingOrders, now);
    const Order& anchor = waitingOrders.front();
    decision.anchorOrderId = anchor.id;
    decision.equipment = equipmentName(anchor.buildKey);
    decision.anchorUrgency = urgency(anchor, now);

    if (maxBatchSize <= 1) {
        return decision;
    }

    std::vector<Order> compatibleCandidates;
    for (std::size_t i = 1; i < waitingOrders.size(); ++i) {
        if (canBatch(anchor, waitingOrders[i], now)) {
            compatibleCandidates.push_back(waitingOrders[i]);
        }
    }

    std::sort(compatibleCandidates.begin(), compatibleCandidates.end(),
        [this, &anchor, now](const Order& left, const Order& right) {
            const double leftCompatibility = calculateCompatibility(anchor, left);
            const double rightCompatibility = calculateCompatibility(anchor, right);
            if (leftCompatibility != rightCompatibility) {
                return leftCompatibility > rightCompatibility;
            }
            const double leftUrgency = urgency(left, now);
            const double rightUrgency = urgency(right, now);
            if (leftUrgency != rightUrgency) {
                return leftUrgency > rightUrgency;
            }
            return left.id < right.id;
        });

    for (const Order& candidate : compatibleCandidates) {
        if (decision.batchedOrderIds.size() >= maxBatchSize - 1) {
            break;
        }
        decision.batchedOrderIds.push_back(candidate.id);
    }

    return decision;
}