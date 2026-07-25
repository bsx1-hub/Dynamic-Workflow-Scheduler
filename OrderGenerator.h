#ifndef ORDER_GENERATOR_H
#define ORDER_GENERATOR_H

#include "Order.h"

#include <cstddef>
#include <ctime>
#include <random>
#include <string>
#include <vector>

enum class DemandScenario {
    Normal,
    Rush
};

class OrderGenerator {
public:
    explicit OrderGenerator(
        unsigned int seed = 42
    );

    std::vector<Order> generateScenario(
        DemandScenario scenario,
        std::size_t orderCount,
        std::time_t startTime
    );

private:
    std::mt19937 randomEngine;
    int nextOrderId;

    int randomInteger(
        int minimum,
        int maximum
    );

    Source generateSource();

    std::string generateSize();

    Order generateOrder(
        std::time_t placedAt
    );
};

#endif