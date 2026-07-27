#ifndef COMMAND_PROCESSOR_H
#define COMMAND_PROCESSOR_H

#include "QueueManager.h"
#include "Scheduler.h"
#include "TerminalDisplay.h"

#include <ctime>
#include <string>

class CommandProcessor {
public:
    CommandProcessor(
        QueueManager& queue,
        const Scheduler& scheduler,
        TerminalDisplay& display,
        std::time_t simulationStart
    );

    void run();

private:
    QueueManager& queue;
    const Scheduler& scheduler;
    TerminalDisplay& display;
    std::time_t simulationStart;

    bool paused = false;
    bool running = true;
    int nextOrderId = 1;
    std::string eventMessage = "Manual mode started";

    void refresh();
    void printCommandHelp() const;
    void processCommand(const std::string& input);

    void handleAdd();
    void handleStart(int id);
    void handleComplete(int id);
    void handleCancel(int id);
    void handleFind(int id);
    void handlePause();
    void handleResume();

    bool stateChangesAllowed();
    int reserveNextOrderId();

    static std::string trim(const std::string& text);
    static std::string lower(std::string text);
    static bool parseIdCommand(
        const std::string& arguments,
        int& id
    );

    static bool readSource(Source& source);
    static bool readTemperature(bool& isHot);
    static bool readPositiveInteger(
        const std::string& prompt,
        int& value
    );
};

#endif