#include "EquipmentStatusParser.h"

#include <array>
#include <string_view>


namespace {

bool containsWhitespace(const std::string& value) {
    for (const char character : value) {
        if (character == ' ' || character == '\t' ||
            character == '\r' || character == '\n') {
            return true;
        }
    }

    return false;
}

bool stationFromToken(
    std::string_view token,
    EquipmentType& station
) {
    if (token == "ESPRESSO") {
        station = EquipmentType::Espresso;
        return true;
    }

    if (token == "BREW") {
        station = EquipmentType::Brew;
        return true;
    }

    if (token == "FROZEN") {
        station = EquipmentType::Frozen;
        return true;
    }

    return false;
}

bool stateFromToken(
    std::string_view token,
    EquipmentState& state
) {
    if (token == "AVAILABLE") {
        state = EquipmentState::Available;
        return true;
    }

    if (token == "BUSY") {
        state = EquipmentState::Busy;
        return true;
    }

    return false;
}

} // namespace

EquipmentStatusParseResult EquipmentStatusParser::applyLine(
    const std::string& line,
    EquipmentManager& equipmentManager
) const {
    std::string message = line;

    if (!message.empty() && message.back() == '\r') {
        message.pop_back();
    }

    if (message.empty() || containsWhitespace(message)) {
        return EquipmentStatusParseResult::Invalid;
    }

    std::array<std::string_view, 3> fields;
    std::size_t fieldStart = 0;

    for (std::size_t index = 0; index < fields.size(); ++index) {
        const std::size_t comma = message.find(',', fieldStart);

        if (index < fields.size() - 1 && comma == std::string::npos) {
            return EquipmentStatusParseResult::Invalid;
        }

        if (index == fields.size() - 1) {
            if (comma != std::string::npos) {
                return EquipmentStatusParseResult::Invalid;
            }

            fields[index] = std::string_view(message).substr(fieldStart);
        } else {
            fields[index] = std::string_view(message).substr(
                fieldStart,
                comma - fieldStart
            );
            fieldStart = comma + 1;
        }

        if (fields[index].empty()) {
            return EquipmentStatusParseResult::Invalid;
        }
    }

    if (fields[0] != "STATUS") {
        return EquipmentStatusParseResult::Invalid;
    }

    EquipmentType station;
    EquipmentState state;

    if (!stationFromToken(fields[1], station) ||
        !stateFromToken(fields[2], state)) {
        return EquipmentStatusParseResult::Invalid;
    }

    equipmentManager.setReportedState(station, state);
    return EquipmentStatusParseResult::Applied;
}