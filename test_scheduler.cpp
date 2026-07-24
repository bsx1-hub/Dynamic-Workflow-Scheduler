#include "Order.h"
#include "QueueManager.h"
#include "Scheduler.h"

#include <cmath>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

namespace {
int testsRun = 0;
int testsFailed = 0;

void check(bool condition, const std::string& testName) {
    ++testsRun;

    if (condition) {
        std::cout << "[PASS] " << testName << '\n';
    } else {
        ++testsFailed;
        std::cerr << "[FAIL] " << testName << '\n';
    }
}

bool approximatelyEqual(double left, double right, double tolerance = 1e-9) {
    return std::fabs(left - right) <= tolerance;
}

Order makeOrder(
    int id,
    Source source,
    std::time_t placedAt,
    Status status = Status::Waiting
) {
    return {
        id,
        "Test Drink",
        "M",
        true,
        source,
        placedAt,
        "test",
        status
    };
}

void testSourceUrgencyRates() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    const Order driveThru = makeOrder(1, Source::DriveThru, now - 100);
    const Order mobile = makeOrder(2, Source::Mobile, now - 100);
    const Order eatIn = makeOrder(3, Source::EatIn, now - 100);

    const double driveUrgency = scheduler.urgency(driveThru, now);
    const double mobileUrgency = scheduler.urgency(mobile, now);
    const double eatInUrgency = scheduler.urgency(eatIn, now);

    check(
        driveUrgency > mobileUrgency,
        "Drive-thru urgency grows faster than mobile urgency"
    );

    check(
        mobileUrgency > eatInUrgency,
        "Mobile urgency grows faster than eat-in urgency"
    );

    check(
        approximatelyEqual(driveUrgency, 100.0) &&
        approximatelyEqual(mobileUrgency, 60.0) &&
        approximatelyEqual(eatInUrgency, 40.0),
        "Urgency scores use the configured source rates"
    );
}

void testOldEatInCanOutrankNewerOrder() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    std::vector<Order> orders {
        makeOrder(1, Source::DriveThru, now - 20),  // 20 urgency
        makeOrder(2, Source::EatIn, now - 100)      // 40 urgency
    };

    scheduler.prioritize(orders, now);

    check(
        orders.front().id == 2,
        "An old eat-in order eventually outranks a newer drive-thru order"
    );
}

void testCompletedOrdersAreExcluded() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    std::vector<Order> orders {
        makeOrder(1, Source::DriveThru, now - 10, Status::Waiting),
        makeOrder(2, Source::DriveThru, now - 500, Status::Complete)
    };

    scheduler.prioritize(orders, now);

    check(
        orders.front().id == 1 &&
        orders.front().status == Status::Waiting,
        "Completed orders do not outrank waiting orders"
    );

    QueueManager queue;
    queue.addOrder(makeOrder(10, Source::Mobile, now - 20));
    queue.addOrder(makeOrder(11, Source::EatIn, now - 200));
    queue.startOrder(11);
    queue.completeOrder(11);

    const std::vector<Order> waiting = queue.getWaitingOrders();

    check(
        waiting.size() == 1 && waiting.front().id == 10,
        "Completed orders are excluded from the schedulable waiting-order view"
    );
}

void testEmptyQueueDoesNotCrash() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;
    std::vector<Order> orders;

    scheduler.prioritize(orders, now);

    check(
        orders.empty(),
        "Prioritizing an empty queue does not crash"
    );

    QueueManager queue;
    queue.prioritize(scheduler, now);

    check(
        queue.getWaitingOrders().empty(),
        "QueueManager safely prioritizes an empty queue"
    );
}

void testEqualScoresHaveDeterministicOrdering() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    // Both receive an urgency score of 30:
    // Drive-thru: 1.0 * 30 seconds
    // Mobile:     0.6 * 50 seconds
    std::vector<Order> orders {
        makeOrder(20, Source::DriveThru, now - 30),
        makeOrder(10, Source::Mobile, now - 50)
    };

    scheduler.prioritize(orders, now);

    check(
        orders[0].id == 10 && orders[1].id == 20,
        "Equal scores are ordered by the earlier placement time"
    );

    std::vector<Order> sameTimestamp {
        makeOrder(9, Source::DriveThru, now - 30),
        makeOrder(3, Source::DriveThru, now - 30)
    };

    scheduler.prioritize(sameTimestamp, now);

    check(
        sameTimestamp[0].id == 3 && sameTimestamp[1].id == 9,
        "Equal scores and timestamps are ordered by lower ID"
    );
}

void testInvalidWaitTimesAreHandledSafely() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    const Order futureOrder = makeOrder(
        1,
        Source::DriveThru,
        now + 100
    );

    check(
        approximatelyEqual(scheduler.urgency(futureOrder, now), 0.0),
        "Future timestamps are clamped to zero urgency"
    );

    Order invalidSourceOrder = makeOrder(
        2,
        static_cast<Source>(999),
        now - 100
    );

    check(
        approximatelyEqual(scheduler.urgency(invalidSourceOrder, now), 0.0),
        "Invalid source values produce a safe zero urgency score"
    );
}
} // namespace

int main() {
    testSourceUrgencyRates();
    testOldEatInCanOutrankNewerOrder();
    testCompletedOrdersAreExcluded();
    testEmptyQueueDoesNotCrash();
    testEqualScoresHaveDeterministicOrdering();
    testInvalidWaitTimesAreHandledSafely();

    std::cout << "\n" << testsRun - testsFailed
              << "/" << testsRun << " tests passed.\n";

    return testsFailed == 0 ? 0 : 1;
}