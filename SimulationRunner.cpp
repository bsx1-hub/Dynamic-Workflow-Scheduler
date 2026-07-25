#include "SimulationRunner.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <unordered_map>

int SimulationRunner::preparationTicks(
    const Order& order
) const {
    const double rawTicks =
        static_cast<double>(
            order.estimatedPrepSeconds
        ) / SECONDS_PER_TICK;

    return std::max(
        1,
        static_cast<int>(
            std::ceil(rawTicks)
        )
    );
}

bool SimulationRunner::equipmentIsBusy(
    EquipmentType equipment,
    const std::vector<ActiveJob>& activeJobs
) const {
    for (const ActiveJob& job : activeJobs) {
        if (job.equipment == equipment) {
            return true;
        }
    }

    return false;
}

std::vector<std::size_t>
SimulationRunner::selectFIFOOrders(
    const std::vector<SimulatedOrder>& orders,
    const std::vector<ActiveJob>& activeJobs
) const {
    std::vector<std::size_t> selectedIndices;

    for (std::size_t i = 0; i < orders.size(); ++i) {
        const SimulatedOrder& simulatedOrder =
            orders[i];

        if (simulatedOrder.started ||
            simulatedOrder.completed) {
            continue;
        }

        const EquipmentType equipment =
            equipmentTypeFromBuildKey(
                simulatedOrder.order.buildKey
            );

        if (equipmentIsBusy(
                equipment,
                activeJobs
            )) {
            continue;
        }

        const bool equipmentAlreadySelected =
            std::any_of(
                selectedIndices.begin(),
                selectedIndices.end(),
                [&orders, equipment](
                    std::size_t selectedIndex
                ) {
                    return equipmentTypeFromBuildKey(
                        orders[selectedIndex]
                            .order
                            .buildKey
                    ) == equipment;
                }
            );

        if (!equipmentAlreadySelected) {
            selectedIndices.push_back(i);
        }
    }

    return selectedIndices;
}

std::vector<std::size_t>
SimulationRunner::selectDynamicBatch(
    const std::vector<SimulatedOrder>& orders,
    const std::vector<ActiveJob>& activeJobs,
    std::time_t simulationTime
) const {
    std::vector<Order> availableOrders;
    std::unordered_map<int, std::size_t> indexById;

    for (std::size_t i = 0; i < orders.size(); ++i) {
        const SimulatedOrder& simulatedOrder =
            orders[i];

        if (simulatedOrder.started ||
            simulatedOrder.completed) {
            continue;
        }

        const EquipmentType equipment =
            equipmentTypeFromBuildKey(
                simulatedOrder.order.buildKey
            );

        if (equipmentIsBusy(
                equipment,
                activeJobs
            )) {
            continue;
        }

        availableOrders.push_back(
            simulatedOrder.order
        );

        indexById[
            simulatedOrder.order.id
        ] = i;
    }

    if (availableOrders.empty()) {
        return {};
    }

    const ScheduleDecision decision =
        scheduler.makeDecision(
            availableOrders,
            simulationTime,
            3
        );

    if (decision.anchorOrderId == -1) {
        return {};
    }

    std::vector<std::size_t> selectedIndices;

    const auto anchorMatch =
        indexById.find(
            decision.anchorOrderId
        );

    if (anchorMatch == indexById.end()) {
        return {};
    }

    selectedIndices.push_back(
        anchorMatch->second
    );

    for (int orderId :
         decision.batchedOrderIds) {
        const auto match =
            indexById.find(orderId);

        if (match != indexById.end()) {
            selectedIndices.push_back(
                match->second
            );
        }
    }

    return selectedIndices;
}

SimulationMetrics SimulationRunner::run(
    const std::vector<Order>& orderStream,
    SchedulingStrategy strategy,
    std::time_t scenarioStart
) const {
    SimulationMetrics metrics;
    metrics.strategyName =
        strategyName(strategy);

    std::vector<SimulatedOrder> orders;
    orders.reserve(orderStream.size());

    for (const Order& order : orderStream) {
        const int arrivalTick =
            static_cast<int>(
                std::difftime(
                    order.placedAt,
                    scenarioStart
                ) / SECONDS_PER_TICK
            );

        orders.push_back({
            order,
            std::max(0, arrivalTick)
        });
    }

    std::sort(
        orders.begin(),
        orders.end(),
        [](const SimulatedOrder& left,
           const SimulatedOrder& right) {
            if (left.arrivalTick !=
                right.arrivalTick) {
                return left.arrivalTick <
                       right.arrivalTick;
            }

            return left.order.id <
                   right.order.id;
        }
    );

    std::vector<ActiveJob> activeJobs;

    int tick = 0;
    int completedCount = 0;

    while (
        completedCount <
        static_cast<int>(orders.size())
    ) {
        /*
         * Complete work performed during the
         * previous tick.
         */
        for (ActiveJob& job : activeJobs) {
            --job.ticksRemaining;
        }

        for (const ActiveJob& job : activeJobs) {
            if (job.ticksRemaining > 0) {
                continue;
            }

            for (int completedId :
                 job.orderIds) {
                for (SimulatedOrder& order :
                     orders) {
                    if (order.order.id ==
                        completedId) {
                        order.completed = true;
                        order.completionTick = tick;
                        ++completedCount;
                        break;
                    }
                }
            }
        }

        activeJobs.erase(
            std::remove_if(
                activeJobs.begin(),
                activeJobs.end(),
                [](const ActiveJob& job) {
                    return job.ticksRemaining <= 0;
                }
            ),
            activeJobs.end()
        );

        /*
         * Determine which orders have arrived.
         */
        std::vector<SimulatedOrder> eligibleOrders =
            orders;

        for (SimulatedOrder& order :
             eligibleOrders) {
            if (order.arrivalTick > tick) {
                order.started = true;
            }
        }

        std::vector<std::size_t> selections;

        if (strategy ==
            SchedulingStrategy::FIFO) {
            selections =
                selectFIFOOrders(
                    eligibleOrders,
                    activeJobs
                );
        } else {
            const std::time_t simulationTime =
                scenarioStart +
                tick * SECONDS_PER_TICK;

            selections =
                selectDynamicBatch(
                    eligibleOrders,
                    activeJobs,
                    simulationTime
                );
        }

        /*
         * Start selected orders.
         */
        if (strategy ==
            SchedulingStrategy::FIFO) {
            for (std::size_t index :
                 selections) {
                SimulatedOrder& order =
                    orders[index];

                if (order.arrivalTick > tick ||
                    order.started ||
                    order.completed) {
                    continue;
                }

                const EquipmentType equipment =
                    equipmentTypeFromBuildKey(
                        order.order.buildKey
                    );

                order.started = true;
                order.startTick = tick;

                activeJobs.push_back({
                    equipment,
                    {order.order.id},
                    preparationTicks(order.order)
                });

                ++metrics.equipmentOperations;
            }
        } else if (!selections.empty()) {
            std::vector<std::size_t>
                validSelections;

            for (std::size_t index :
                 selections) {
                if (index >= orders.size()) {
                    continue;
                }

                SimulatedOrder& order =
                    orders[index];

                if (order.arrivalTick <= tick &&
                    !order.started &&
                    !order.completed) {
                    validSelections.push_back(
                        index
                    );
                }
            }

            if (!validSelections.empty()) {
                const EquipmentType equipment =
                    equipmentTypeFromBuildKey(
                        orders[
                            validSelections.front()
                        ].order.buildKey
                    );

                int longestPreparation = 1;

                std::vector<int> batchIds;

                for (std::size_t index :
                     validSelections) {
                    SimulatedOrder& order =
                        orders[index];

                    order.started = true;
                    order.startTick = tick;

                    longestPreparation =
                        std::max(
                            longestPreparation,
                            preparationTicks(
                                order.order
                            )
                        );

                    batchIds.push_back(
                        order.order.id
                    );
                }

                /*
                 * Shared setup model:
                 *
                 * The longest drink determines the
                 * base batch time. Each additional
                 * order adds one tick.
                 */
                const int batchDuration =
                    longestPreparation +
                    static_cast<int>(
                        validSelections.size()
                    ) - 1;

                activeJobs.push_back({
                    equipment,
                    batchIds,
                    batchDuration
                });

                ++metrics.equipmentOperations;

                if (validSelections.size() > 1) {
                    metrics.ordersBatched +=
                        static_cast<int>(
                            validSelections.size()
                        ) - 1;
                }
            }
        }

        ++tick;

        /*
         * Safety limit prevents an accidental
         * infinite simulation.
         */
        if (tick > 100'000) {
            break;
        }
    }

    metrics.ordersCompleted =
        completedCount;

    metrics.totalTicks = tick;

    double totalWait = 0.0;
    int maximumWait = 0;

    for (const SimulatedOrder& order : orders) {
        if (order.startTick < 0) {
            continue;
        }

        const int waitTicks =
            order.startTick -
            order.arrivalTick;

        totalWait += waitTicks;
        maximumWait =
            std::max(
                maximumWait,
                waitTicks
            );
    }

    if (!orders.empty()) {
        metrics.averageWaitTicks =
            totalWait /
            static_cast<double>(
                orders.size()
            );
    }

    metrics.maximumWaitTicks =
        maximumWait;

    return metrics;
}

std::string strategyName(
    SchedulingStrategy strategy
) {
    switch (strategy) {
        case SchedulingStrategy::FIFO:
            return "FIFO";

        case SchedulingStrategy::DynamicBatching:
            return "Dynamic Scheduling with Batching";
    }

    return "Unknown";
}