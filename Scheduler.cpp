//brain of the project

#include "Scheduler.h"

#include <algorithm>
#include <ctime>

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

    double secondsWaiting = std::max( // compiles because of <algorithm>
        0.0,
        difftime(now, order.placedAt)
    );

    return rate * secondsWaiting; //im modeling U = r * t
}

void Scheduler::prioritize(
    std::vector<Order>& orders,
    time_t now
) const {
    std::sort(
        orders.begin(),
        orders.end(),

        [this, now](const Order& a, const Order& b) {
            const bool aWaiting = a.status == Status::Waiting;
            const bool bWaiting = b.status == Status::Waiting;

            if (aWaiting != bWaiting) {
                return aWaiting;
            }

            double aUrgency = urgency(a, now);
            double bUrgency = urgency(b, now);

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
    const Order& lead,
    const Order& candidate,
    time_t now
) const {
    const double BATCH_WINDOW = 20.0;

    if (lead.buildKey != candidate.buildKey) {
        return false;
    }

    double leadUrgency = urgency(lead, now);
    double candidateUrgency = urgency(candidate, now);

    if (candidateUrgency > leadUrgency) {
        return false;
    }

    double urgencyGap = leadUrgency - candidateUrgency;

    return urgencyGap <= BATCH_WINDOW;
}