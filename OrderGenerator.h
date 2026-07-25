#ifndef ORDER_GENERATOR_H
#define ORDER_GENERATOR_H

#include "Order.h"

#include <cstddef>
#include <ctime>
#include <random>
#include <vector>

enum class DemandScenario {
    Normal,
    Rush
};

class OrderGenerator {
private:
    std::mt19937 randomEngine;
    int nextOrderId;

    int randomInteger(int minimum, int maximum);
    Source generateSource();
    std::string generateSize();

    Order generateOrder(
        std::time_t arrivalTime
    );

public:
    explicit OrderGenerator(
        unsigned int seed = std::random_device{}()
    );

    std::vector<Order> generateScenario(
        DemandScenario scenario,
        std::size_t orderCount,
        std::time_t startTime
    );
};

#endif