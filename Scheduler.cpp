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

    double secondsWaiting = difftime(
        now,
        order.placedAt
    );

    return rate * secondsWaiting; //im modeling U = r * t
}

void Scheduler::prioritize(
    std::vector<Order>& orders,
    time_t now
) const {
    std::sort( //ranks order by U value
        orders.begin(),
        orders.end(),

        [this, now](const Order& a, const Order& b) {
            return urgency(a, now) > urgency(b, now);
        }
    );
}