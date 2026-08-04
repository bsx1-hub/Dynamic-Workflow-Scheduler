#ifndef EQUIPMENT_MANAGER_H
#define EQUIPMENT_MANAGER_H

#include <string>

enum class EquipmentType {
    Espresso,
    Brew,
    Frozen,
    Other
};

enum class EquipmentState {
    Available,
    Busy
};

struct Equipment {
    EquipmentState state = EquipmentState::Available;
    int busyTicksRemaining = 0;
    bool reportedStateOverride = false;
};

class EquipmentManager {
private:
    Equipment espressoStation;
    Equipment brewStation;
    Equipment frozenStation;
    Equipment otherStation;

    Equipment& getMutableEquipment(EquipmentType type);

public:
    const Equipment& getEquipment(EquipmentType type) const;
    bool isAvailable(EquipmentType type) const;
    bool setBusy(EquipmentType type, int busyTicks);
    void setReportedState(EquipmentType type, EquipmentState state);
    void update();
    void displayEquipment() const;
};

EquipmentType equipmentTypeFromBuildKey(const std::string& buildKey);
std::string equipmentTypeName(EquipmentType type);
std::string equipmentStateName(EquipmentState state);

#endif