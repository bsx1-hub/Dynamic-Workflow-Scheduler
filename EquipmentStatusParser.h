#ifndef EQUIPMENT_STATUS_PARSER_H
#define EQUIPMENT_STATUS_PARSER_H

#include "EquipmentManager.h"

#include <string>

enum class EquipmentStatusParseResult {
    Applied,
    Invalid
};

class EquipmentStatusParser {
public:
    // Parses one newline-delimited STM32 status message and updates the
    // matching physical station only when the message is valid.
    EquipmentStatusParseResult applyLine(
        const std::string& line,
        EquipmentManager& equipmentManager
    ) const;
};

#endif