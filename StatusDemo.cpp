#include "StatusDemo.h"

#include "EquipmentManager.h"
#include "EquipmentStatusParser.h"
#include "Order.h"
#include "Scheduler.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace Color {
constexpr const char* Reset = "\033[0m";
constexpr const char* Bold = "\033[1m";
constexpr const char* Cyan = "\033[36m";
constexpr const char* Green = "\033[32m";
constexpr const char* Yellow = "\033[33m";
constexpr const char* Red = "\033[31m";
constexpr const char* Magenta = "\033[35m";
} // namespace Color

void clearScreen() {
    std::cout << "\033[2J\033[H";
}

void waitForFrame(bool fastMode) {
    if (!fastMode) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

const char* stationToken(EquipmentType type) {
    switch (type) {
        case EquipmentType::Espresso: return "ESPRESSO";
        case EquipmentType::Brew: return "BREW";
        case EquipmentType::Frozen: return "FROZEN";
        case EquipmentType::Other: return "OTHER";
    }
    return "UNKNOWN";
}

void renderFrame(
    const std::vector<Order>& orders,
    const Scheduler& scheduler,
    const EquipmentManager& equipmentManager,
    std::time_t now,
    const std::string& eventMessage
) {
    clearScreen();

    const ScheduleDecision decision =
        scheduler.makeDecision(orders, equipmentManager, now);

    std::cout << Color::Bold << Color::Cyan
              << "DYNAMIC WORKFLOW SCHEDULER  |  STM32 STATUS DEMO\n"
              << Color::Reset
              << "Non-interactive recording mode\n\n"
              << Color::Magenta << "EVENT: " << eventMessage
              << Color::Reset << "\n\n"
              << Color::Bold << Color::Yellow << "NEXT ACTION\n"
              << Color::Reset;

    if (decision.anchorOrderId == -1) {
        std::cout << "No feasible waiting order.\n";
    } else {
        const Order* anchor = nullptr;
        for (const Order& order : orders) {
            if (order.id == decision.anchorOrderId) {
                anchor = &order;
                break;
            }
        }

        std::cout << Color::Bold << decision.equipment << Color::Reset << '\n';
        if (anchor != nullptr) {
            std::cout << "Anchor: #" << anchor->id << ' '
                      << sourceName(anchor->source) << ' '
                      << anchor->drink << '\n';
        }

        std::cout << "Batch: ";
        if (decision.batchedOrderIds.empty()) {
            std::cout << "None\n";
        } else {
            for (std::size_t i = 0; i < decision.batchedOrderIds.size(); ++i) {
                std::cout << (i == 0 ? "#" : ", #")
                          << decision.batchedOrderIds[i];
            }
            std::cout << '\n';
        }
    }

    std::cout << "\n" << Color::Bold << Color::Yellow
              << "LIVE EQUIPMENT STATUS\n" << Color::Reset;

    const EquipmentType physicalStations[] {
        EquipmentType::Espresso,
        EquipmentType::Brew,
        EquipmentType::Frozen
    };

    for (EquipmentType type : physicalStations) {
        const bool available = equipmentManager.isAvailable(type);
        std::cout << std::left << std::setw(12) << stationToken(type)
                  << (available ? Color::Green : Color::Red)
                  << (available ? "AVAILABLE" : "BUSY")
                  << Color::Reset << '\n';
    }

    std::cout << "\n" << Color::Bold << Color::Yellow
              << "WAITING ORDERS\n" << Color::Reset;
    for (const Order& order : orders) {
        std::cout << '#' << order.id << "  " << std::setw(12) << order.drink
                  << std::setw(12) << order.buildKey
                  << sourceName(order.source) << '\n';
    }

    std::cout << std::flush;
}

bool applyStatus(
    const std::string& line,
    EquipmentStatusParser& parser,
    EquipmentManager& equipmentManager
) {
    return parser.applyLine(line, equipmentManager) ==
           EquipmentStatusParseResult::Applied;
}

} // namespace

int runAutomatedStatusDemo(bool fastMode) {
    const std::time_t now = std::time(nullptr);
    const std::vector<Order> sampleOrders {
        {101, "Latte", "M", true, Source::DriveThru, now - 90, "espresso"},
        {102, "Cappuccino", "M", true, Source::Mobile, now - 70, "espresso"},
        {103, "Iced Coffee", "L", false, Source::Mobile, now - 55, "brew"},
        {104, "Frozen Matcha", "L", false, Source::EatIn, now - 30, "frozen"}
    };

    Scheduler scheduler;
    EquipmentManager equipmentManager;
    EquipmentStatusParser parser;

    renderFrame(sampleOrders, scheduler, equipmentManager, now,
                "Startup: all physical stations reported AVAILABLE");
    waitForFrame(fastMode);

    if (!applyStatus("STATUS,ESPRESSO,BUSY", parser, equipmentManager)) {
        return 1;
    }
    renderFrame(sampleOrders, scheduler, equipmentManager, now + 2,
                "UART received STATUS,ESPRESSO,BUSY - rerouted to Brew");
    waitForFrame(fastMode);

    if (!applyStatus("STATUS,BREW,BUSY", parser, equipmentManager)) {
        return 1;
    }
    renderFrame(sampleOrders, scheduler, equipmentManager, now + 4,
                "UART received STATUS,BREW,BUSY - rerouted to Frozen");
    waitForFrame(fastMode);

    if (!applyStatus("STATUS,ESPRESSO,AVAILABLE", parser, equipmentManager)) {
        return 1;
    }
    renderFrame(sampleOrders, scheduler, equipmentManager, now + 6,
                "UART received STATUS,ESPRESSO,AVAILABLE - espresso priority restored");

    std::cout << "\n" << Color::Green << Color::Bold
              << "Demo complete.\n" << Color::Reset;
    return 0;
}