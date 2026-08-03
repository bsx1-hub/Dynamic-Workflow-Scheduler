#include "EquipmentManager.h"
#include "EquipmentStatusParser.h"

#include <iostream>
#include <string>

namespace {

int testsRun = 0;
int testsFailed = 0;

void check(bool condition, const std::string& name) {
    ++testsRun;

    if (condition) {
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++testsFailed;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

bool applies(
    const std::string& line,
    EquipmentManager& equipmentManager
) {
    const EquipmentStatusParser parser;
    return parser.applyLine(line, equipmentManager) ==
           EquipmentStatusParseResult::Applied;
}

void testValidStatusUpdates() {
    EquipmentManager equipmentManager;

    check(
        applies("STATUS,ESPRESSO,BUSY", equipmentManager) &&
            !equipmentManager.isAvailable(EquipmentType::Espresso),
        "Applies an espresso BUSY update"
    );

    check(
        applies("STATUS,BREW,BUSY\r", equipmentManager) &&
            !equipmentManager.isAvailable(EquipmentType::Brew),
        "Accepts an optional carriage return before newline"
    );

    check(
        applies("STATUS,FROZEN,AVAILABLE", equipmentManager) &&
            equipmentManager.isAvailable(EquipmentType::Frozen),
        "Applies a frozen AVAILABLE update"
    );
}

void testInvalidMessagesDoNotChangeState() {
    const std::string invalidMessages[] {
        "",
        "STATUS,ESPRESSO",
        "STATUS,OTHER,BUSY",
        "STATUS,ESPRESSO,OFFLINE",
        "status,ESPRESSO,BUSY",
        "STATUS, ESPRESSO,BUSY",
        "STATUS,ESPRESSO,BUSY,EXTRA",
        "STATUS,,BUSY"
    };

    for (const std::string& line : invalidMessages) {
        EquipmentManager equipmentManager;
        const bool result = applies(line, equipmentManager);

        check(
            !result && equipmentManager.isAvailable(EquipmentType::Espresso),
            "Rejects invalid message without changing equipment state: " + line
        );
    }
}

void testDuplicateAndPersistentUpdates() {
    EquipmentManager equipmentManager;

    const bool firstBusy =
        applies("STATUS,ESPRESSO,BUSY", equipmentManager);
    const bool secondBusy =
        applies("STATUS,ESPRESSO,BUSY", equipmentManager);

    equipmentManager.update();

    check(
        firstBusy && secondBusy &&
            !equipmentManager.isAvailable(EquipmentType::Espresso),
        "Duplicate reported updates are idempotent and BUSY persists"
    );

    check(
        applies("STATUS,ESPRESSO,AVAILABLE", equipmentManager) &&
            equipmentManager.isAvailable(EquipmentType::Espresso),
        "AVAILABLE update re-enables a station"
    );
}

} // namespace

int main() {
    testValidStatusUpdates();
    testInvalidMessagesDoNotChangeState();
    testDuplicateAndPersistentUpdates();

    std::cout << '\n' << testsRun << " tests run, "
              << testsFailed << " tests failed\n";

    return testsFailed == 0 ? 0 : 1;
}