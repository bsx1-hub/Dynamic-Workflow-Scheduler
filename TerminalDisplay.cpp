#include "TerminalDisplay.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Color {

constexpr const char* Reset = "\033[0m";
constexpr const char* Bold = "\033[1m";
constexpr const char* Cyan = "\033[36m";
constexpr const char* Green = "\033[32m";
constexpr const char* Yellow = "\033[33m";
constexpr const char* Red = "\033[31m";
constexpr const char* Magenta = "\033[35m";
constexpr const char* Gray = "\033[90m";

} // namespace Color

void TerminalDisplay::enableAnsiColors() {
#ifdef _WIN32
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

    if (output == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD mode = 0;

    if (!GetConsoleMode(output, &mode)) {
        return;
    }

    SetConsoleMode(
        output,
        mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING
    );
#endif
}

void TerminalDisplay::clear() {
#ifdef _WIN32
    std::system("cls");
#else
    std::cout << "\033[2J\033[H";
#endif
}

std::string TerminalDisplay::formatElapsed(long seconds) {
    seconds = std::max(0L, seconds);

    const long minutes = seconds / 60;
    const long remainingSeconds = seconds % 60;

    std::ostringstream output;

    output
        << std::setfill('0')
        << std::setw(2)
        << minutes
        << ':'
        << std::setw(2)
        << remainingSeconds;

    return output.str();
}

std::string TerminalDisplay::rushLevel(
    std::size_t activeOrders
) {
    if (activeOrders >= 8) {
        return "HIGH";
    }

    if (activeOrders >= 4) {
        return "MEDIUM";
    }

    return "LOW";
}

std::string TerminalDisplay::equipmentLabel(
    const std::string& buildKey
) {
    if (buildKey == "espresso") {
        return "Espresso";
    }

    if (buildKey == "brew") {
        return "Brew";
    }

    if (buildKey == "frozen") {
        return "Frozen";
    }

    return "Other";
}

void TerminalDisplay::render(
    const QueueManager& queue,
    const Scheduler& scheduler,
    std::time_t simulationStart,
    std::time_t now,
    const std::string& eventMessage
) const {
    clear();

    const std::vector<Order> activeOrders =
        queue.getActiveOrders();

    const std::vector<Order> waitingOrders =
        queue.getWaitingOrders();

    const ScheduleDecision decision =
        scheduler.makeDecision(
            waitingOrders,
            now
        );

    std::cout
        << Color::Bold
        << Color::Cyan
        << "DYNAMIC WORKFLOW SCHEDULER\n"
        << Color::Reset;

    std::cout
        << "Simulation Time: "
        << formatElapsed(
               static_cast<long>(
                   std::difftime(
                       now,
                       simulationStart
                   )
               )
           )
        << '\n';

    const std::string level =
        rushLevel(activeOrders.size());

    const char* rushColor =
        level == "HIGH"
            ? Color::Red
            : level == "MEDIUM"
                  ? Color::Yellow
                  : Color::Green;

    std::cout
        << "Rush Level: "
        << rushColor
        << Color::Bold
        << level
        << Color::Reset
        << "\n\n";

    if (!eventMessage.empty()) {
        std::cout
            << Color::Magenta
            << "EVENT: "
            << eventMessage
            << Color::Reset
            << "\n\n";
    }

    std::cout
        << Color::Bold
        << Color::Yellow
        << "NEXT ACTION\n"
        << Color::Reset;

    if (decision.anchorOrderId == -1) {
        std::cout
            << Color::Gray
            << "No waiting orders\n"
            << Color::Reset;
    } else {
        const Order* anchor =
            queue.findOrderById(
                decision.anchorOrderId
            );

        std::cout
            << decision.equipment
            << '\n';

        if (anchor != nullptr) {
            std::cout
                << "Anchor: "
                << Color::Bold
                << '#'
                << anchor->id
                << ' '
                << sourceName(anchor->source)
                << ' '
                << anchor->drink
                << Color::Reset
                << '\n';
        }

        std::cout << "Batch: ";

        if (decision.batchedOrderIds.empty()) {
            std::cout
                << Color::Gray
                << "None"
                << Color::Reset;
        } else {
            bool firstPrintedOrder = true;

            for (int orderId :
                 decision.batchedOrderIds) {
                const Order* batchedOrder =
                    queue.findOrderById(orderId);

                if (batchedOrder == nullptr) {
                    continue;
                }

                if (!firstPrintedOrder) {
                    std::cout << ", ";
                }

                std::cout
                    << '#'
                    << batchedOrder->id
                    << ' '
                    << sourceName(
                           batchedOrder->source
                       )
                    << ' '
                    << batchedOrder->drink;

                firstPrintedOrder = false;
            }
        }

        std::cout << '\n';
    }

    std::cout
        << '\n'
        << Color::Bold
        << Color::Yellow
        << "ACTIVE EQUIPMENT\n"
        << Color::Reset;

    std::set<std::string> busyEquipment;

    for (const Order& order : activeOrders) {
        if (order.status == Status::InProgress) {
            busyEquipment.insert(
                order.buildKey
            );
        }
    }

    const std::vector<std::string> equipmentKeys {
        "espresso",
        "brew",
        "frozen",
        "other"
    };

    for (const std::string& key :
         equipmentKeys) {
        const bool busy =
            busyEquipment.count(key) > 0;

        std::cout
            << std::left
            << std::setw(12)
            << equipmentLabel(key)
            << (busy
                    ? Color::Red
                    : Color::Green)
            << (busy
                    ? "BUSY"
                    : "AVAILABLE")
            << Color::Reset
            << '\n';
    }

    std::cout
        << '\n'
        << Color::Bold
        << Color::Yellow
        << "WAITING QUEUE\n"
        << Color::Reset;

    std::cout
        << std::left
        << std::setw(5)
        << "ID"
        << std::setw(14)
        << "SOURCE"
        << std::setw(8)
        << "WAIT"
        << std::setw(13)
        << "STATUS"
        << "EQUIPMENT\n";

    std::cout
        << Color::Gray
        << "--------------------------------------------------------\n"
        << Color::Reset;

    if (activeOrders.empty()) {
        std::cout
            << Color::Gray
            << "No active orders\n"
            << Color::Reset;
    }

    for (const Order& order :
         activeOrders) {
        const long waitSeconds =
            std::max(
                0L,
                static_cast<long>(
                    std::difftime(
                        now,
                        order.placedAt
                    )
                )
            );

        const bool isAnchor =
            order.id ==
            decision.anchorOrderId;

        const bool isBatchCandidate =
            std::find(
                decision.batchedOrderIds.begin(),
                decision.batchedOrderIds.end(),
                order.id
            ) !=
            decision.batchedOrderIds.end();

        const bool scheduled =
            isAnchor ||
            isBatchCandidate;

        std::string displayStatus =
            statusName(order.status);

        if (order.status == Status::Waiting &&
            scheduled) {
            displayStatus = "Scheduled";
        }

        const char* statusColor =
            order.status ==
                    Status::InProgress
                ? Color::Red
                : scheduled
                      ? Color::Cyan
                      : Color::Reset;

        std::cout
            << statusColor
            << std::left
            << std::setw(5)
            << order.id
            << std::setw(14)
            << sourceName(order.source)
            << std::setw(8)
            << (
                   std::to_string(
                       waitSeconds
                   ) +
                   "s"
               )
            << std::setw(13)
            << displayStatus
            << equipmentLabel(
                   order.buildKey
               )
            << Color::Reset
            << '\n';
    }

    const std::size_t completedCount =
        queue.completedCount();

    long totalWait = 0;

    for (const Order& order :
         waitingOrders) {
        totalWait +=
            std::max(
                0L,
                static_cast<long>(
                    std::difftime(
                        now,
                        order.placedAt
                    )
                )
            );
    }

    const long averageWait =
        waitingOrders.empty()
            ? 0
            : totalWait /
                  static_cast<long>(
                      waitingOrders.size()
                  );

    const std::size_t currentBatchSize =
        decision.anchorOrderId == -1
            ? 0
            : 1 +
                  decision
                      .batchedOrderIds
                      .size();

    std::cout
        << '\n'
        << Color::Bold
        << Color::Yellow
        << "METRICS\n"
        << Color::Reset
        << "Completed: "
        << completedCount
        << '\n'
        << "Average Wait: "
        << averageWait
        << "s\n"
        << "Current Batch Size: "
        << currentBatchSize
        << '\n'
        << std::flush;
}