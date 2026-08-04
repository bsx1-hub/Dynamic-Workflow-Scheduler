#include "EquipmentManager.h"
#include "EquipmentStatusParser.h"
#include "Order.h"
#include "Scheduler.h"

#include <ctime>
#include <iostream>
#include <vector>

int main() {
    const std::time_t now = 1000;
    const std::vector<Order> orders {
        {1, "Latte", "M", true, Source::DriveThru, now - 90, "espresso"},
        {2, "Coffee", "L", false, Source::Mobile, now - 55, "brew"},
        {3, "Frozen Matcha", "L", false, Source::EatIn, now - 30, "frozen"}
    };
    Scheduler scheduler;
    EquipmentManager equipment;
    EquipmentStatusParser parser;

    if (scheduler.makeDecision(orders, equipment, now).anchorOrderId != 1) return 1;
    parser.applyLine("STATUS,ESPRESSO,BUSY", equipment);
    if (scheduler.makeDecision(orders, equipment, now).anchorOrderId != 2) return 1;
    parser.applyLine("STATUS,BREW,BUSY", equipment);
    if (scheduler.makeDecision(orders, equipment, now).anchorOrderId != 3) return 1;
    parser.applyLine("STATUS,ESPRESSO,AVAILABLE", equipment);
    if (scheduler.makeDecision(orders, equipment, now).anchorOrderId != 1) return 1;

    std::cout << "[PASS] Status demo reroutes and restores recommendations\n";
    return 0;
}