#include "OrderGenerator.h"

#include <array>
#include <stdexcept>
#include <string>

namespace {

struct DrinkProfile {
    const char* name;
    const char* buildKey;
    bool mayBeHot;
    bool mayBeIced;
    int minimumPrepSeconds;
    int maximumPrepSeconds;
};

constexpr std::array<DrinkProfile, 8> DRINK_PROFILES {{
    {
        "Latte",
        "espresso",
        true,
        true,
        75,
        110
    },
    {
        "Cappuccino",
        "espresso",
        true,
        false,
        70,
        100
    },
    {
        "Espresso",
        "espresso",
        true,
        false,
        30,
        50
    },
    {
        "Coffee",
        "brew",
        true,
        true,
        25,
        45
    },
    {
        "Cold Brew",
        "brew",
        false,
        true,
        20,
        40
    },
    {
        "Tea",
        "brew",
        true,
        true,
        35,
        60
    },
    {
        "Frozen Coffee",
        "frozen",
        false,
        true,
        80,
        120
    },
    {
        "Frozen Matcha",
        "frozen",
        false,
        true,
        85,
        125
    }
}};

} // namespace

OrderGenerator::OrderGenerator(unsigned int seed)
    : randomEngine(seed),
      nextOrderId(1) {
}

int OrderGenerator::randomInteger(
    int minimum,
    int maximum
) {
    std::uniform_int_distribution<int> distribution(
        minimum,
        maximum
    );

    return distribution(randomEngine);
}

Source OrderGenerator::generateSource() {
    // Random value from 1 through 100:
    // 1-45   = Drive-Thru, 45%
    // 46-80  = Mobile, 35%
    // 81-100 = Eat-In, 20%
    const int sourceRoll = randomInteger(1, 100);

    if (sourceRoll <= 45) {
        return Source::DriveThru;
    }

    if (sourceRoll <= 80) {
        return Source::Mobile;
    }

    return Source::EatIn;
}

std::string OrderGenerator::generateSize() {
    constexpr std::array<const char*, 3> SIZES {
        "S",
        "M",
        "L"
    };

    const int index = randomInteger(
        0,
        static_cast<int>(SIZES.size()) - 1
    );

    return SIZES[index];
}

Order OrderGenerator::generateOrder(
    std::time_t arrivalTime
) {
    const int drinkIndex = randomInteger(
        0,
        static_cast<int>(DRINK_PROFILES.size()) - 1
    );

    const DrinkProfile& profile =
        DRINK_PROFILES[drinkIndex];

    bool isHot = false;

    if (profile.mayBeHot && profile.mayBeIced) {
        isHot = randomInteger(0, 1) == 1;
    } else if (profile.mayBeHot) {
        isHot = true;
    }

    const int preparationTime = randomInteger(
        profile.minimumPrepSeconds,
        profile.maximumPrepSeconds
    );

    Order order {
        nextOrderId++,
        profile.name,
        generateSize(),
        isHot,
        generateSource(),
        arrivalTime,
        profile.buildKey,
        Status::Waiting,
        preparationTime
    };

    return order;
}

std::vector<Order> OrderGenerator::generateScenario(
    DemandScenario scenario,
    std::size_t orderCount,
    std::time_t startTime
) {
    std::vector<Order> generatedOrders;
    generatedOrders.reserve(orderCount);

    std::time_t arrivalTime = startTime;

    for (std::size_t i = 0; i < orderCount; ++i) {
        int minimumInterval = 0;
        int maximumInterval = 0;

        switch (scenario) {
            case DemandScenario::Normal:
                minimumInterval = 15;
                maximumInterval = 45;
                break;

            case DemandScenario::Rush:
                minimumInterval = 2;
                maximumInterval = 10;
                break;

            default:
                throw std::invalid_argument(
                    "Unknown demand scenario."
                );
        }

        if (i > 0) {
            arrivalTime += randomInteger(
                minimumInterval,
                maximumInterval
            );
        }

        generatedOrders.push_back(
            generateOrder(arrivalTime)
        );
    }

    return generatedOrders;
}