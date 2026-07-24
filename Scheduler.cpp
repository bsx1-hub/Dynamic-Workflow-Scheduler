// Brain of the project: contains the scheduling algorithms.

#include "Scheduler.h"

#include <algorithm>
#include <ctime>

namespace {

// Converts an internal build key into a worker-facing station name.
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

}

double Scheduler::urgency(
    const Order& order,
    time_t now
) const {
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

    const double secondsWaiting = std::max(
        0.0,
        difftime(now, order.placedAt)
    );

    return rate * secondsWaiting;
}

double Scheduler::calculateCompatibility(
    const Order& anchor,
    const Order& candidate
) const {
    if (anchor.buildKey.empty() ||
        candidate.buildKey.empty()) {
        return 0.0;
    }

    if (anchor.buildKey != candidate.buildKey) {
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
    time_t now
) const {
    std::sort(
        orders.begin(),
        orders.end(),

        [this, now](const Order& a, const Order& b) {
            const bool aWaiting =
                a.status == Status::Waiting;

            const bool bWaiting =
                b.status == Status::Waiting;

            if (aWaiting != bWaiting) {
                return aWaiting;
            }

            const double aUrgency =
                urgency(a, now);

            const double bUrgency =
                urgency(b, now);

            if (aUrgency != bUrgency) {
                return aUrgency > bUrgency;
            }

            if (a.placedAt != b.placedAt) {
                return a.placedAt < b.placedAt;
            }

            return a.id < b.id;
        }
    );
}

bool Scheduler::canBatch(
    const Order& anchor,
    const Order& candidate,
    time_t now
) const {
    constexpr double MIN_COMPATIBILITY_SCORE = 40.0;
    constexpr double MAX_BATCH_URGENCY_GAP = 20.0;

    if (anchor.id == candidate.id) {
        return false;
    }

    if (anchor.status != Status::Waiting ||
        candidate.status != Status::Waiting) {
        return false;
    }

    const double compatibility =
        calculateCompatibility(anchor, candidate);

    if (compatibility < MIN_COMPATIBILITY_SCORE) {
        return false;
    }

    const double anchorUrgency =
        urgency(anchor, now);

    const double candidateUrgency =
        urgency(candidate, now);

    if (candidateUrgency > anchorUrgency) {
        return false;
    }

    const double urgencyGap =
        anchorUrgency - candidateUrgency;

    return urgencyGap <= MAX_BATCH_URGENCY_GAP;
}

ScheduleDecision Scheduler::makeDecision(
    const std::vector<Order>& orders,
    time_t now,
    std::size_t maxBatchSize
) const {
    ScheduleDecision decision;

    std::vector<Order> waitingOrders;

    for (const Order& order : orders) {
        if (order.status == Status::Waiting) {
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

    for (std::size_t i = 1;
         i < waitingOrders.size();
         ++i) {
        const Order& candidate = waitingOrders[i];

        if (canBatch(anchor, candidate, now)) {
            compatibleCandidates.push_back(candidate);
        }
    }

    std::sort(
        compatibleCandidates.begin(),
        compatibleCandidates.end(),

        [this, &anchor, now](
            const Order& left,
            const Order& right
        ) {
            const double leftCompatibility =
                calculateCompatibility(anchor, left);

            const double rightCompatibility =
                calculateCompatibility(anchor, right);

            if (leftCompatibility != rightCompatibility) {
                return leftCompatibility >
                       rightCompatibility;
            }

            const double leftUrgency =
                urgency(left, now);

            const double rightUrgency =
                urgency(right, now);

            if (leftUrgency != rightUrgency) {
                return leftUrgency > rightUrgency;
            }

            return left.id < right.id;
        }
    );

    for (const Order& candidate : compatibleCandidates) {
        if (decision.batchedOrderIds.size()
            >= maxBatchSize - 1) {
            break;
        }

        decision.batchedOrderIds.push_back(
            candidate.id
        );
    }

    return decision;
}