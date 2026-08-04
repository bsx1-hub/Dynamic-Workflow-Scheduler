#include "EquipmentManager.h"
#include "Scheduler.h"

#include <ctime>
#include <iostream>
#include <vector>

namespace {

Order makeOrder(
    int id,
    const std::string& drink,
    Source source,
    std::time_t placedAt,
    const std::string& buildKey
) {
    return {id, drink, "M", true, source, placedAt, buildKey};
}

int failures = 0;

void check(bool condition, const std::string& name) {
    if (condition) {
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++failures;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

} // namespace

int main() {
    const std::time_t now = 1'000;
    const Scheduler scheduler;
    EquipmentManager equipmentManager;

    equipmentManager.setReportedState(
        EquipmentType::Espresso,
        EquipmentState::Busy
    );

    const std::vector<Order> orders {
        makeOrder(1, "Latte", Source::DriveThru, now - 100, "espresso"),
        makeOrder(2, "Coffee", Source::Mobile, now - 50, "brew"),
        makeOrder(3, "Frozen Matcha", Source::EatIn, now - 40, "frozen")
    };

    const ScheduleDecision busyDecision =
        scheduler.makeDecision(orders, equipmentManager, now);

    check(busyDecision.anchorOrderId == 2,
          "Scheduler skips an order requiring reported-busy Espresso");
    check(busyDecision.equipment == "Brewing Station",
          "Scheduler recommends the highest-urgency feasible station");

    equipmentManager.setReportedState(
        EquipmentType::Espresso,
        EquipmentState::Available
    );

    const ScheduleDecision availableDecision =
        scheduler.makeDecision(orders, equipmentManager, now);

    check(availableDecision.anchorOrderId == 1,
          "AVAILABLE re-enables Espresso orders for recommendation");

    std::cout << "\n" << 3 - failures << "/3 tests passed.\n";
    return failures == 0 ? 0 : 1;
}