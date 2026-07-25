#ifndef EQUIPMENT_MANAGER_H
#define EQUIPMENT_MANAGER_H

#include <string>

enum class EquipmentState {
    Available,
    Busy
};

enum class EquipmentType {
    Espresso,
    Brew,
    Frozen,
    Other
};

struct Equipment {
    EquipmentType type;
    EquipmentState state = EquipmentState::Available;
    int busyTicksRemaining = 0;
};

class EquipmentManager {
private:
    Equipment espressoStation {
        EquipmentType::Espresso
    };

    Equipment brewStation {
        EquipmentType::Brew
    };

    Equipment frozenStation {
        EquipmentType::Frozen
    };

    Equipment otherStation {
        EquipmentType::Other
    };

    Equipment& getMutableEquipment(
        EquipmentType type
    );

public:
    const Equipment& getEquipment(
        EquipmentType type
    ) const;

    bool isAvailable(
        EquipmentType type
    ) const;

    bool setBusy(
        EquipmentType type,
        int busyTicks
    );

    void update();

    void displayEquipment() const;
};

EquipmentType equipmentTypeFromBuildKey(
    const std::string& buildKey
);

std::string equipmentTypeName(
    EquipmentType type
);

std::string equipmentStateName(
    EquipmentState state
);

#endif