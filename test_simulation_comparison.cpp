#include "OrderGenerator.h"
#include "SimulationRunner.h"

#include <cmath>
#include <ctime>
#include <iostream>

namespace {

int failures = 0;

void check(bool condition, const char* name) {
    if (condition) {
        std::cout << "[PASS] " << name << '\n';
    } else {
        ++failures;
        std::cerr << "[FAIL] " << name << '\n';
    }
}

} // namespace

int main() {
    constexpr std::time_t scenarioStart = 1'000'000;
    OrderGenerator generator(42);
    const std::vector<Order> orders = generator.generateScenario(
        DemandScenario::Rush,
        20,
        scenarioStart
    );

    const SimulationRunner runner;
    const SimulationMetrics fifo = runner.run(
        orders,
        SchedulingStrategy::FIFO,
        scenarioStart
    );
    const SimulationMetrics dynamic = runner.run(
        orders,
        SchedulingStrategy::DynamicBatching,
        scenarioStart
    );

    check(fifo.ordersCompleted == 20 && dynamic.ordersCompleted == 20,
          "Both strategies complete the same 20-order stream");
    check(dynamic.peakConcurrentOperations >= 2,
          "Dynamic scheduling starts work on multiple free stations in one tick");
    check(dynamic.equipmentOperations <= fifo.equipmentOperations,
          "Dynamic batching does not use more equipment operations than FIFO");

    std::cout << "\nFixed-seed rush comparison (seed 42, 20 orders)\n";
    std::cout << "FIFO:    wait=" << fifo.averageWaitTicks
              << ", max wait=" << fifo.maximumWaitTicks
              << ", ticks=" << fifo.totalTicks
              << ", operations=" << fifo.equipmentOperations
              << ", peak concurrent=" << fifo.peakConcurrentOperations << '\n';
    std::cout << "Dynamic: wait=" << dynamic.averageWaitTicks
              << ", max wait=" << dynamic.maximumWaitTicks
              << ", ticks=" << dynamic.totalTicks
              << ", operations=" << dynamic.equipmentOperations
              << ", peak concurrent=" << dynamic.peakConcurrentOperations << '\n';

    if (fifo.averageWaitTicks > 0.0) {
        const double waitReduction = 100.0 *
            (fifo.averageWaitTicks - dynamic.averageWaitTicks) /
            fifo.averageWaitTicks;
        std::cout << "Average-wait reduction: " << std::round(waitReduction * 10.0) / 10.0
                  << "%\n";
    }

    return failures == 0 ? 0 : 1;
}