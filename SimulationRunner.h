#ifndef SIMULATION_RUNNER_H
#define SIMULATION_RUNNER_H

#include "EquipmentManager.h"
#include "Order.h"
#include "Scheduler.h"

#include <ctime>
#include <string>
#include <vector>

enum class SchedulingStrategy {
    FIFO,
    DynamicBatching
};

struct SimulationMetrics {
    std::string strategyName;

    int ordersCompleted = 0;
    int totalTicks = 0;

    double averageWaitTicks = 0.0;
    int maximumWaitTicks = 0;

    int equipmentOperations = 0;
    int ordersBatched = 0;
};

class SimulationRunner {
private:
    struct ActiveJob {
        EquipmentType equipment;
        std::vector<int> orderIds;
        int ticksRemaining;
    };

    struct SimulatedOrder {
        Order order;
        int arrivalTick;
        int startTick = -1;
        int completionTick = -1;
        bool started = false;
        bool completed = false;
    };

    Scheduler scheduler;

    static constexpr int SECONDS_PER_TICK = 10;

    int preparationTicks(
        const Order& order
    ) const;

    bool equipmentIsBusy(
        EquipmentType equipment,
        const std::vector<ActiveJob>& activeJobs
    ) const;

    std::vector<std::size_t> selectFIFOOrders(
        const std::vector<SimulatedOrder>& orders,
        const std::vector<ActiveJob>& activeJobs
    ) const;

    std::vector<std::size_t> selectDynamicBatch(
        const std::vector<SimulatedOrder>& orders,
        const std::vector<ActiveJob>& activeJobs,
        std::time_t simulationTime
    ) const;

public:
    SimulationMetrics run(
        const std::vector<Order>& orderStream,
        SchedulingStrategy strategy,
        std::time_t scenarioStart
    ) const;
};

std::string strategyName(
    SchedulingStrategy strategy
);

#endif