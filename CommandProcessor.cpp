#include "CommandProcessor.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>

CommandProcessor::CommandProcessor(
    QueueManager& queueValue,
    const Scheduler& schedulerValue,
    TerminalDisplay& displayValue,
    std::time_t simulationStartValue
)
    : queue(queueValue),
      scheduler(schedulerValue),
      display(displayValue),
      simulationStart(simulationStartValue) {
}

void CommandProcessor::run() {
    while (running) {
        refresh();

        std::cout
            << "\nCOMMANDS: add | start <id> | complete <id> | "
            << "cancel <id> | find <id>\n"
            << "          pause | resume | help | quit\n\n"
            << (paused ? "[PAUSED] " : "")
            << "> ";

        std::string input;

        if (!std::getline(std::cin, input)) {
            running = false;
            break;
        }

        processCommand(trim(input));
    }

    TerminalDisplay::clear();
    std::cout << "Manual mode closed.\n";
}

void CommandProcessor::refresh() {
    const std::time_t now = std::time(nullptr);

    queue.prioritize(scheduler, now);

    display.render(
        queue,
        scheduler,
        simulationStart,
        now,
        eventMessage
    );
}

void CommandProcessor::printCommandHelp() const {
    std::cout
        << "\nCOMMAND REFERENCE\n"
        << "add           Prompt for a new order\n"
        << "start <id>    Move Waiting -> In Progress\n"
        << "complete <id> Move In Progress -> Complete\n"
        << "cancel <id>   Cancel a Waiting or In Progress order\n"
        << "find <id>     Show one order's complete state\n"
        << "pause         Block state-changing commands\n"
        << "resume        Re-enable state-changing commands\n"
        << "help          Show this command reference\n"
        << "quit          Exit manual mode\n"
        << "\nPress Enter to continue...";

    std::string ignored;
    std::getline(std::cin, ignored);
}

void CommandProcessor::processCommand(
    const std::string& input
) {
    if (input.empty()) {
        eventMessage = "No command entered";
        return;
    }

    std::istringstream parser(input);
    std::string command;
    parser >> command;
    command = lower(command);

    std::string arguments;
    std::getline(parser, arguments);
    arguments = trim(arguments);

    if (command == "add") {
        if (!arguments.empty()) {
            eventMessage = "Usage: add";
            return;
        }

        handleAdd();
        return;
    }

    if (command == "pause") {
        handlePause();
        return;
    }

    if (command == "resume") {
        handleResume();
        return;
    }

    if (command == "help") {
        printCommandHelp();
        eventMessage = "Command help closed";
        return;
    }

    if (command == "quit") {
        running = false;
        return;
    }

    int id = -1;

    if (command == "start" ||
        command == "complete" ||
        command == "cancel" ||
        command == "find") {
        if (!parseIdCommand(arguments, id)) {
            eventMessage =
                "Command requires one positive numeric order ID";
            return;
        }

        if (command == "start") {
            handleStart(id);
        } else if (command == "complete") {
            handleComplete(id);
        } else if (command == "cancel") {
            handleCancel(id);
        } else {
            handleFind(id);
        }

        return;
    }

    eventMessage = "Unknown command: " + command;
}

void CommandProcessor::handleAdd() {
    if (!stateChangesAllowed()) {
        return;
    }

    std::string drink;
    std::string size;
    std::string buildKey;
    Source source = Source::EatIn;
    bool isHot = false;
    int estimatedPrepSeconds = 0;

    std::cout << "\nDrink name: ";
    std::getline(std::cin, drink);
    drink = trim(drink);

    if (drink.empty()) {
        eventMessage = "Add cancelled: drink name cannot be empty";
        return;
    }

    std::cout << "Size (S/M/L): ";
    std::getline(std::cin, size);
    size = lower(trim(size));

    if (size == "s") {
        size = "S";
    } else if (size == "m") {
        size = "M";
    } else if (size == "l") {
        size = "L";
    } else {
        eventMessage = "Add cancelled: size must be S, M, or L";
        return;
    }

    if (!readTemperature(isHot)) {
        eventMessage = "Add cancelled: temperature must be hot or iced";
        return;
    }

    if (!readSource(source)) {
        eventMessage =
            "Add cancelled: source must be drive, mobile, or eatin";
        return;
    }

    std::cout << "Equipment (espresso/brew/frozen/other): ";
    std::getline(std::cin, buildKey);
    buildKey = lower(trim(buildKey));

    if (buildKey != "espresso" &&
        buildKey != "brew" &&
        buildKey != "frozen" &&
        buildKey != "other") {
        eventMessage =
            "Add cancelled: invalid equipment build key";
        return;
    }

    if (!readPositiveInteger(
            "Estimated preparation seconds: ",
            estimatedPrepSeconds
        )) {
        eventMessage =
            "Add cancelled: preparation time must be positive";
        return;
    }

    const int id = reserveNextOrderId();

    const Order order {
        id,
        drink,
        size,
        isHot,
        source,
        std::time(nullptr),
        buildKey,
        Status::Waiting,
        estimatedPrepSeconds
    };

    if (!queue.addOrder(order)) {
        eventMessage = "Could not add order #" + std::to_string(id);
        return;
    }

    eventMessage =
        "Added #" + std::to_string(id) + " " +
        sourceName(source) + " " + drink;
}

void CommandProcessor::handleStart(int id) {
    if (!stateChangesAllowed()) {
        return;
    }

    if (queue.startOrder(id)) {
        eventMessage =
            "Order #" + std::to_string(id) + " started";
    } else {
        eventMessage =
            "Could not start #" + std::to_string(id) +
            ": order must exist and be Waiting";
    }
}

void CommandProcessor::handleComplete(int id) {
    if (!stateChangesAllowed()) {
        return;
    }

    if (queue.completeOrder(id)) {
        eventMessage =
            "Order #" + std::to_string(id) + " completed";
    } else {
        eventMessage =
            "Could not complete #" + std::to_string(id) +
            ": order must exist and be In Progress";
    }
}

void CommandProcessor::handleCancel(int id) {
    if (!stateChangesAllowed()) {
        return;
    }

    if (queue.cancelOrder(id)) {
        eventMessage =
            "Order #" + std::to_string(id) + " cancelled";
    } else {
        eventMessage =
            "Could not cancel #" + std::to_string(id) +
            ": order may not exist or is already terminal";
    }
}

void CommandProcessor::handleFind(int id) {
    const Order* order = queue.findOrderById(id);

    if (order == nullptr) {
        eventMessage =
            "Order #" + std::to_string(id) + " was not found";
        return;
    }

    const long ageSeconds = std::max(
        0L,
        static_cast<long>(
            std::difftime(
                std::time(nullptr),
                order->placedAt
            )
        )
    );

    eventMessage =
        "Found #" + std::to_string(order->id) +
        " | " + sourceName(order->source) +
        " | " + order->size + " " +
        (order->hot ? "Hot " : "Iced ") +
        order->drink +
        " | " + statusName(order->status) +
        " | " + order->buildKey +
        " | age " + std::to_string(ageSeconds) + "s";
}

void CommandProcessor::handlePause() {
    if (paused) {
        eventMessage = "Manual processing is already paused";
        return;
    }

    paused = true;
    eventMessage =
        "Manual processing paused; use resume to continue";
}

void CommandProcessor::handleResume() {
    if (!paused) {
        eventMessage = "Manual processing is already active";
        return;
    }

    paused = false;
    eventMessage = "Manual processing resumed";
}

bool CommandProcessor::stateChangesAllowed() {
    if (!paused) {
        return true;
    }

    eventMessage =
        "Command rejected while paused; use resume first";
    return false;
}

int CommandProcessor::reserveNextOrderId() {
    while (queue.findOrderById(nextOrderId) != nullptr) {
        ++nextOrderId;
    }

    return nextOrderId++;
}

std::string CommandProcessor::trim(
    const std::string& text
) {
    const auto first = std::find_if_not(
        text.begin(),
        text.end(),
        [](unsigned char character) {
            return std::isspace(character) != 0;
        }
    );

    if (first == text.end()) {
        return "";
    }

    const auto last = std::find_if_not(
        text.rbegin(),
        text.rend(),
        [](unsigned char character) {
            return std::isspace(character) != 0;
        }
    ).base();

    return std::string(first, last);
}

std::string CommandProcessor::lower(
    std::string text
) {
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char character) {
            return static_cast<char>(
                std::tolower(character)
            );
        }
    );

    return text;
}

bool CommandProcessor::parseIdCommand(
    const std::string& arguments,
    int& id
) {
    std::istringstream parser(arguments);
    std::string extra;

    if (!(parser >> id) || id <= 0) {
        return false;
    }

    return !(parser >> extra);
}

bool CommandProcessor::readSource(Source& source) {
    std::cout << "Source (drive/mobile/eatin): ";

    std::string input;
    std::getline(std::cin, input);
    input = lower(trim(input));

    if (input == "drive" ||
        input == "drive-thru" ||
        input == "drivethru") {
        source = Source::DriveThru;
        return true;
    }

    if (input == "mobile") {
        source = Source::Mobile;
        return true;
    }

    if (input == "eatin" ||
        input == "eat-in" ||
        input == "eat in") {
        source = Source::EatIn;
        return true;
    }

    return false;
}

bool CommandProcessor::readTemperature(bool& isHot) {
    std::cout << "Temperature (hot/iced): ";

    std::string input;
    std::getline(std::cin, input);
    input = lower(trim(input));

    if (input == "hot") {
        isHot = true;
        return true;
    }

    if (input == "iced" || input == "cold") {
        isHot = false;
        return true;
    }

    return false;
}

bool CommandProcessor::readPositiveInteger(
    const std::string& prompt,
    int& value
) {
    std::cout << prompt;

    std::string input;
    std::getline(std::cin, input);

    std::istringstream parser(trim(input));
    std::string extra;

    if (!(parser >> value) || value <= 0) {
        return false;
    }

    return !(parser >> extra);
}