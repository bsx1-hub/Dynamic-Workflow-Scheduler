#include "EquipmentManager.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

Equipment& EquipmentManager::getMutableEquipment(
    EquipmentType type
) {
    switch (type) {
        case EquipmentType::Espresso:
            return espressoStation;

        case EquipmentType::Brew:
            return brewStation;

        case EquipmentType::Frozen:
            return frozenStation;

        case EquipmentType::Other:
            return otherStation;
    }

    throw std::invalid_argument("Unknown equipment type.");
}

const Equipment& EquipmentManager::getEquipment(
    EquipmentType type
) const {
    switch (type) {
        case EquipmentType::Espresso:
            return espressoStation;

        case EquipmentType::Brew:
            return brewStation;

        case EquipmentType::Frozen:
            return frozenStation;

        case EquipmentType::Other:
            return otherStation;
    }

    throw std::invalid_argument("Unknown equipment type.");
}

bool EquipmentManager::isAvailable(
    EquipmentType type
) const {
    return getEquipment(type).state == EquipmentState::Available;
}

bool EquipmentManager::setBusy(
    EquipmentType type,
    int busyTicks
) {
    if (busyTicks <= 0) {
        return false;
    }

    Equipment& equipment = getMutableEquipment(type);

    if (equipment.state == EquipmentState::Busy) {
        return false;
    }

    equipment.state = EquipmentState::Busy;
    equipment.busyTicksRemaining = busyTicks;
    equipment.reportedStateOverride = false;

    return true;
}

void EquipmentManager::setReportedState(
    EquipmentType type,
    EquipmentState state
) {
    Equipment& equipment = getMutableEquipment(type);

    equipment.state = state;
    equipment.busyTicksRemaining = 0;

    // STM32-reported BUSY remains active until STM32 reports AVAILABLE.
    equipment.reportedStateOverride = (state == EquipmentState::Busy);
}

void EquipmentManager::update() {
    Equipment* stations[] {
        &espressoStation,
        &brewStation,
        &frozenStation,
        &otherStation
    };

    for (Equipment* equipment : stations) {
        // Do not decrement a BUSY state reported by the STM32.
        if (equipment->reportedStateOverride) {
            continue;
        }

        if (equipment->state != EquipmentState::Busy) {
            continue;
        }

        equipment->busyTicksRemaining =
            std::max(0, equipment->busyTicksRemaining - 1);

        if (equipment->busyTicksRemaining == 0) {
            equipment->state = EquipmentState::Available;
        }
    }
}

void EquipmentManager::displayEquipment() const {
    const EquipmentType types[] {
        EquipmentType::Espresso,
        EquipmentType::Brew,
        EquipmentType::Frozen,
        EquipmentType::Other
    };

    std::cout
        << "================ EQUIPMENT =================\n";

    for (EquipmentType type : types) {
        const Equipment& equipment = getEquipment(type);

        std::cout
            << equipmentTypeName(type)
            << ": "
            << equipmentStateName(equipment.state);

        if (equipment.state == EquipmentState::Busy) {
            if (equipment.reportedStateOverride) {
                std::cout << " (reported busy)";
            } else {
                std::cout
                    << " ("
                    << equipment.busyTicksRemaining
                    << " ticks remaining)";
            }
        }

        std::cout << '\n';
    }

    std::cout
        << "============================================\n";
}

EquipmentType equipmentTypeFromBuildKey(
    const std::string& buildKey
) {
    if (buildKey == "espresso") {
        return EquipmentType::Espresso;
    }

    if (buildKey == "brew") {
        return EquipmentType::Brew;
    }

    if (buildKey == "frozen") {
        return EquipmentType::Frozen;
    }

    return EquipmentType::Other;
}

std::string equipmentTypeName(
    EquipmentType type
) {
    switch (type) {
        case EquipmentType::Espresso:
            return "Espresso Station";

        case EquipmentType::Brew:
            return "Brew Station";

        case EquipmentType::Frozen:
            return "Frozen Station";

        case EquipmentType::Other:
            return "Other Station";
    }

    return "Unknown Station";
}

std::string equipmentStateName(
    EquipmentState state
) {
    switch (state) {
        case EquipmentState::Available:
            return "Available";

        case EquipmentState::Busy:
            return "Busy";
    }

    return "Unknown";
}