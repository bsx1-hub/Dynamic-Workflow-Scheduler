#ifndef TERMINAL_DISPLAY_H
#define TERMINAL_DISPLAY_H

#include "QueueManager.h"
#include "Scheduler.h"

#include <ctime>
#include <string>

class TerminalDisplay {
public:
    static void enableAnsiColors();
    static void clear();

    void render(
        const QueueManager& queue,
        const Scheduler& scheduler,
        std::time_t simulationStart,
        std::time_t now,
        const std::string& eventMessage = ""
    ) const;

private:
    static std::string formatElapsed(long seconds);
    static std::string rushLevel(std::size_t activeOrders);
    static std::string equipmentLabel(const std::string& buildKey);
};

#endif