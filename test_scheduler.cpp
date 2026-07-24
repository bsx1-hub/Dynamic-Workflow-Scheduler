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

void check(
    bool condition,
    const std::string& testName
) {
    ++testsRun;

    if (condition) {
        std::cout
            << "[PASS] "
            << testName
            << '\n';
    } else {
        ++testsFailed;

        std::cerr
            << "[FAIL] "
            << testName
            << '\n';
    }
}

bool approximatelyEqual(
    double left,
    double right,
    double tolerance = 1e-9
) {
    return std::fabs(left - right) <= tolerance;
}

// Creates a basic order for urgency and priority tests.
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

// Creates an order with customizable batching attributes.
Order makeDetailedOrder(
    int id,
    const std::string& drink,
    bool hot,
    Source source,
    std::time_t placedAt,
    const std::string& buildKey,
    Status status = Status::Waiting
) {
    return {
        id,
        drink,
        "M",
        hot,
        source,
        placedAt,
        buildKey,
        status
    };
}

void testSourceUrgencyRates() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    const Order driveThru =
        makeOrder(
            1,
            Source::DriveThru,
            now - 100
        );

    const Order mobile =
        makeOrder(
            2,
            Source::Mobile,
            now - 100
        );

    const Order eatIn =
        makeOrder(
            3,
            Source::EatIn,
            now - 100
        );

    const double driveUrgency =
        scheduler.urgency(driveThru, now);

    const double mobileUrgency =
        scheduler.urgency(mobile, now);

    const double eatInUrgency =
        scheduler.urgency(eatIn, now);

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
        makeOrder(
            1,
            Source::DriveThru,
            now - 20
        ),
        makeOrder(
            2,
            Source::EatIn,
            now - 100
        )
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
        makeOrder(
            1,
            Source::DriveThru,
            now - 10,
            Status::Waiting
        ),
        makeOrder(
            2,
            Source::DriveThru,
            now - 500,
            Status::Complete
        )
    };

    scheduler.prioritize(orders, now);

    check(
        orders.front().id == 1 &&
        orders.front().status == Status::Waiting,
        "Completed orders do not outrank waiting orders"
    );

    QueueManager queue;

    queue.addOrder(
        makeOrder(
            10,
            Source::Mobile,
            now - 20
        )
    );

    queue.addOrder(
        makeOrder(
            11,
            Source::EatIn,
            now - 200
        )
    );

    queue.startOrder(11);
    queue.completeOrder(11);

    const std::vector<Order> waiting =
        queue.getWaitingOrders();

    check(
        waiting.size() == 1 &&
        waiting.front().id == 10,
        "Completed orders are excluded from the waiting-order view"
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

    // Both orders have an urgency score of 30:
    // Drive-thru: 1.0 * 30 seconds
    // Mobile:     0.6 * 50 seconds
    std::vector<Order> orders {
        makeOrder(
            20,
            Source::DriveThru,
            now - 30
        ),
        makeOrder(
            10,
            Source::Mobile,
            now - 50
        )
    };

    scheduler.prioritize(orders, now);

    check(
        orders[0].id == 10 &&
        orders[1].id == 20,
        "Equal urgency scores are ordered by earlier placement time"
    );

    std::vector<Order> sameTimestamp {
        makeOrder(
            9,
            Source::DriveThru,
            now - 30
        ),
        makeOrder(
            3,
            Source::DriveThru,
            now - 30
        )
    };

    scheduler.prioritize(sameTimestamp, now);

    check(
        sameTimestamp[0].id == 3 &&
        sameTimestamp[1].id == 9,
        "Equal scores and timestamps are ordered by lower ID"
    );
}

void testInvalidWaitTimesAreHandledSafely() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    const Order futureOrder =
        makeOrder(
            1,
            Source::DriveThru,
            now + 100
        );

    check(
        approximatelyEqual(
            scheduler.urgency(futureOrder, now),
            0.0
        ),
        "Future timestamps are clamped to zero urgency"
    );

    const Order invalidSourceOrder =
        makeOrder(
            2,
            static_cast<Source>(999),
            now - 100
        );

    check(
        approximatelyEqual(
            scheduler.urgency(
                invalidSourceOrder,
                now
            ),
            0.0
        ),
        "Invalid source values produce zero urgency"
    );
}

void testCompatibilityScoring() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    const Order anchor =
        makeDetailedOrder(
            1,
            "Latte",
            true,
            Source::DriveThru,
            now - 70,
            "espresso"
        );

    const Order exactMatch =
        makeDetailedOrder(
            2,
            "Latte",
            true,
            Source::Mobile,
            now - 100,
            "espresso"
        );

    const Order sameTemperature =
        makeDetailedOrder(
            3,
            "Cappuccino",
            true,
            Source::Mobile,
            now - 100,
            "espresso"
        );

    const Order sameEquipmentOnly =
        makeDetailedOrder(
            4,
            "Cappuccino",
            false,
            Source::Mobile,
            now - 100,
            "espresso"
        );

    const Order differentEquipment =
        makeDetailedOrder(
            5,
            "Frozen Coffee",
            false,
            Source::Mobile,
            now - 100,
            "frozen"
        );

    const Order emptyBuildKey =
        makeDetailedOrder(
            6,
            "Latte",
            true,
            Source::Mobile,
            now - 100,
            ""
        );

    check(
        approximatelyEqual(
            scheduler.calculateCompatibility(
                anchor,
                exactMatch
            ),
            60.0
        ),
        "Matching process, temperature, and drink scores 60"
    );

    check(
        approximatelyEqual(
            scheduler.calculateCompatibility(
                anchor,
                sameTemperature
            ),
            50.0
        ),
        "Matching process and temperature scores 50"
    );

    check(
        approximatelyEqual(
            scheduler.calculateCompatibility(
                anchor,
                sameEquipmentOnly
            ),
            40.0
        ),
        "Matching process only scores 40"
    );

    check(
        approximatelyEqual(
            scheduler.calculateCompatibility(
                anchor,
                differentEquipment
            ),
            0.0
        ),
        "Different build keys score zero"
    );

    check(
        approximatelyEqual(
            scheduler.calculateCompatibility(
                anchor,
                emptyBuildKey
            ),
            0.0
        ),
        "Empty build keys score zero"
    );
}

void testBatchEligibility() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    // Anchor urgency:
    // Drive-thru rate 1.0 * 70 seconds = 70
    const Order anchor =
        makeDetailedOrder(
            1,
            "Latte",
            true,
            Source::DriveThru,
            now - 70,
            "espresso"
        );

    // Candidate urgency:
    // Mobile rate 0.6 * 100 seconds = 60
    // Urgency gap = 10
    const Order compatible =
        makeDetailedOrder(
            2,
            "Cappuccino",
            true,
            Source::Mobile,
            now - 100,
            "espresso"
        );

    // Candidate urgency:
    // Mobile rate 0.6 * 50 seconds = 30
    // Urgency gap = 40
    const Order tooFarBehind =
        makeDetailedOrder(
            3,
            "Latte",
            true,
            Source::Mobile,
            now - 50,
            "espresso"
        );

    const Order differentEquipment =
        makeDetailedOrder(
            4,
            "Frozen Coffee",
            false,
            Source::Mobile,
            now - 100,
            "frozen"
        );

    const Order completed =
        makeDetailedOrder(
            5,
            "Latte",
            true,
            Source::Mobile,
            now - 100,
            "espresso",
            Status::Complete
        );

    const Order moreUrgentCandidate =
        makeDetailedOrder(
            6,
            "Latte",
            true,
            Source::DriveThru,
            now - 80,
            "espresso"
        );

    check(
        scheduler.canBatch(
            anchor,
            compatible,
            now
        ),
        "Compatible order within the urgency threshold can batch"
    );

    check(
        !scheduler.canBatch(
            anchor,
            tooFarBehind,
            now
        ),
        "Order outside the urgency threshold cannot batch"
    );

    check(
        !scheduler.canBatch(
            anchor,
            differentEquipment,
            now
        ),
        "Different equipment cannot batch"
    );

    check(
        !scheduler.canBatch(
            anchor,
            completed,
            now
        ),
        "Completed order cannot join a batch"
    );

    check(
        !scheduler.canBatch(
            anchor,
            anchor,
            now
        ),
        "Anchor cannot batch with itself"
    );

    check(
        !scheduler.canBatch(
            anchor,
            moreUrgentCandidate,
            now
        ),
        "A candidate more urgent than the anchor cannot be pulled behind it"
    );
}

void testSchedulingDecision() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    std::vector<Order> orders {
        makeDetailedOrder(
            12,
            "Latte",
            true,
            Source::DriveThru,
            now - 70,
            "espresso"
        ),
        makeDetailedOrder(
            15,
            "Cappuccino",
            true,
            Source::Mobile,
            now - 100,
            "espresso"
        ),
        makeDetailedOrder(
            18,
            "Latte",
            true,
            Source::Mobile,
            now - 90,
            "espresso"
        ),
        makeDetailedOrder(
            20,
            "Frozen Matcha",
            false,
            Source::DriveThru,
            now - 65,
            "frozen"
        )
    };

    const ScheduleDecision decision =
        scheduler.makeDecision(
            orders,
            now
        );

    check(
        decision.anchorOrderId == 12,
        "Scheduling decision selects the highest-urgency anchor"
    );

    check(
        decision.equipment == "Espresso Station",
        "Scheduling decision reports the anchor equipment"
    );

    check(
        approximatelyEqual(
            decision.anchorUrgency,
            70.0
        ),
        "Scheduling decision records anchor urgency"
    );

    check(
        decision.batchedOrderIds.size() == 2,
        "Scheduling decision fills the available batch slots"
    );

    check(
        decision.batchedOrderIds.size() == 2 &&
        decision.batchedOrderIds[0] == 18 &&
        decision.batchedOrderIds[1] == 15,
        "Batch candidates are ranked by compatibility before urgency"
    );
}

void testSchedulingDecisionRespectsMaximumBatchSize() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    std::vector<Order> orders {
        makeDetailedOrder(
            1,
            "Latte",
            true,
            Source::DriveThru,
            now - 70,
            "espresso"
        ),
        makeDetailedOrder(
            2,
            "Latte",
            true,
            Source::Mobile,
            now - 100,
            "espresso"
        ),
        makeDetailedOrder(
            3,
            "Cappuccino",
            true,
            Source::Mobile,
            now - 95,
            "espresso"
        )
    };

    const ScheduleDecision decision =
        scheduler.makeDecision(
            orders,
            now,
            2
        );

    check(
        decision.anchorOrderId == 1,
        "Batch-size test selects the correct anchor"
    );

    check(
        decision.batchedOrderIds.size() == 1,
        "Maximum batch size includes the anchor in the total"
    );
}

void testAnchorOnlyDecision() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    std::vector<Order> orders {
        makeDetailedOrder(
            1,
            "Latte",
            true,
            Source::DriveThru,
            now - 70,
            "espresso"
        ),
        makeDetailedOrder(
            2,
            "Latte",
            true,
            Source::Mobile,
            now - 100,
            "espresso"
        )
    };

    const ScheduleDecision decision =
        scheduler.makeDecision(
            orders,
            now,
            1
        );

    check(
        decision.anchorOrderId == 1,
        "Anchor-only decision selects the correct anchor"
    );

    check(
        decision.batchedOrderIds.empty(),
        "Maximum batch size of one returns no candidates"
    );
}

void testSchedulingDecisionIgnoresNonWaitingOrders() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    std::vector<Order> orders {
        makeDetailedOrder(
            1,
            "Latte",
            true,
            Source::DriveThru,
            now - 50,
            "espresso",
            Status::Waiting
        ),
        makeDetailedOrder(
            2,
            "Latte",
            true,
            Source::DriveThru,
            now - 500,
            "espresso",
            Status::Complete
        ),
        makeDetailedOrder(
            3,
            "Latte",
            true,
            Source::DriveThru,
            now - 400,
            "espresso",
            Status::InProgress
        ),
        makeDetailedOrder(
            4,
            "Latte",
            true,
            Source::DriveThru,
            now - 300,
            "espresso",
            Status::Cancelled
        )
    };

    const ScheduleDecision decision =
        scheduler.makeDecision(
            orders,
            now
        );

    check(
        decision.anchorOrderId == 1,
        "Scheduling decision ignores non-waiting orders"
    );

    check(
        decision.batchedOrderIds.empty(),
        "Non-waiting orders are not included as batch candidates"
    );
}

void testSchedulingDecisionHandlesNoWaitingOrders() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;

    std::vector<Order> orders {
        makeDetailedOrder(
            1,
            "Latte",
            true,
            Source::DriveThru,
            now - 100,
            "espresso",
            Status::Complete
        ),
        makeDetailedOrder(
            2,
            "Coffee",
            false,
            Source::Mobile,
            now - 100,
            "brew",
            Status::Cancelled
        )
    };

    const ScheduleDecision decision =
        scheduler.makeDecision(
            orders,
            now
        );

    check(
        decision.anchorOrderId == -1,
        "No waiting orders produces an invalid anchor ID"
    );

    check(
        decision.batchedOrderIds.empty(),
        "No waiting orders produces an empty batch"
    );

    check(
        decision.equipment.empty(),
        "No waiting orders produces no equipment recommendation"
    );

    check(
        approximatelyEqual(
            decision.anchorUrgency,
            0.0
        ),
        "No waiting orders produces zero anchor urgency"
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

    testCompatibilityScoring();
    testBatchEligibility();
    testSchedulingDecision();
    testSchedulingDecisionRespectsMaximumBatchSize();
    testAnchorOnlyDecision();
    testSchedulingDecisionIgnoresNonWaitingOrders();
    testSchedulingDecisionHandlesNoWaitingOrders();

    std::cout
        << '\n'
        << testsRun - testsFailed
        << "/"
        << testsRun
        << " tests passed.\n";

    return testsFailed == 0 ? 0 : 1;
}