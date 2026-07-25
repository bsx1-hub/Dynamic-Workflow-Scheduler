#include "Order.h"
#include "OrderGenerator.h"
#include "EquipmentManager.h"
#include "SimulationRunner.h"

#include <ctime>
#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

void displayGeneratedOrders(
    const std::vector<Order>& orders,
    std::time_t scenarioStart
) {
    for (const Order& order : orders) {
        const long arrivalOffset =
            static_cast<long>(
                std::difftime(
                    order.placedAt,
                    scenarioStart
                )
            );

        std::cout
            << "#"
            << order.id
            << " | +"
            << arrivalOffset
            << "s | "
            << sourceName(order.source)
            << " | "
            << order.size
            << " "
            << (order.hot ? "Hot" : "Iced")
            << " "
            << order.drink
            << " | buildKey: "
            << order.buildKey
            << " | estimated preparation: "
            << order.estimatedPrepSeconds
            << "s\n";
    }
}

int fixedBusyTicks(EquipmentType type) {
    switch (type) {
        case EquipmentType::Espresso:
            return 3;

        case EquipmentType::Brew:
            return 2;

        case EquipmentType::Frozen:
            return 4;

        case EquipmentType::Other:
            return 1;
    }

    return 1;
}

void runEquipmentSimulation(
    const std::vector<Order>& orders
) {
    EquipmentManager equipmentManager;

    std::cout
        << "\n================ EQUIPMENT SIMULATION ================\n";

    equipmentManager.displayEquipment();

    int tick = 0;

    for (const Order& order : orders) {
        std::cout
            << "\nTICK "
            << tick
            << '\n';

        const EquipmentType requiredEquipment =
            equipmentTypeFromBuildKey(
                order.buildKey
            );

        std::cout
            << "Order #"
            << order.id
            << " requires "
            << equipmentTypeName(
                requiredEquipment
            )
            << ".\n";

        if (equipmentManager.isAvailable(
                requiredEquipment
            )) {
            const int busyTicks =
                fixedBusyTicks(
                    requiredEquipment
                );

            equipmentManager.setBusy(
                requiredEquipment,
                busyTicks
            );

            std::cout
                << "Started Order #"
                << order.id
                << ". Station will be busy for "
                << busyTicks
                << " ticks.\n";
        } else {
            const Equipment& equipment =
                equipmentManager.getEquipment(
                    requiredEquipment
                );

            std::cout
                << "Order #"
                << order.id
                << " must wait. "
                << equipmentTypeName(
                    requiredEquipment
                )
                << " is busy for "
                << equipment.busyTicksRemaining
                << " more tick";

            if (equipment.busyTicksRemaining != 1) {
                std::cout << 's';
            }

            std::cout << ".\n";
        }

        equipmentManager.displayEquipment();

        equipmentManager.update();
        ++tick;
    }

    while (
        !equipmentManager.isAvailable(
            EquipmentType::Espresso
        ) ||
        !equipmentManager.isAvailable(
            EquipmentType::Brew
        ) ||
        !equipmentManager.isAvailable(
            EquipmentType::Frozen
        ) ||
        !equipmentManager.isAvailable(
            EquipmentType::Other
        )
    ) {
        std::cout
            << "\nTICK "
            << tick
            << " - no new order\n";

        equipmentManager.update();
        equipmentManager.displayEquipment();

        ++tick;
    }

    std::cout
        << "\nAll equipment is available.\n";
}

void displayMetrics(
    const SimulationMetrics& metrics
) {
    std::cout
        << "\n============================================\n"
        << metrics.strategyName
        << "\n============================================\n";

    std::cout
        << "Orders completed: "
        << metrics.ordersCompleted
        << '\n';

    std::cout
        << "Total simulation ticks: "
        << metrics.totalTicks
        << '\n';

    std::cout
        << "Average wait: "
        << std::fixed
        << std::setprecision(2)
        << metrics.averageWaitTicks
        << " ticks\n";

    std::cout
        << "Maximum wait: "
        << metrics.maximumWaitTicks
        << " ticks\n";

    std::cout
        << "Equipment operations: "
        << metrics.equipmentOperations
        << '\n';

    std::cout
        << "Orders pulled into batches: "
        << metrics.ordersBatched
        << '\n';
}

void displayComparison(
    const SimulationMetrics& fifo,
    const SimulationMetrics& dynamic
) {
    std::cout
        << "\n================ IMPROVEMENT SUMMARY ================\n";

    const double waitReduction =
        fifo.averageWaitTicks -
        dynamic.averageWaitTicks;

    const int tickReduction =
        fifo.totalTicks -
        dynamic.totalTicks;

    const int operationReduction =
        fifo.equipmentOperations -
        dynamic.equipmentOperations;

    std::cout
        << "Average wait reduction: "
        << std::fixed
        << std::setprecision(2)
        << waitReduction
        << " ticks\n";

    std::cout
        << "Simulation tick reduction: "
        << tickReduction
        << '\n';

    std::cout
        << "Equipment operation reduction: "
        << operationReduction
        << '\n';

    if (
        dynamic.averageWaitTicks <
        fifo.averageWaitTicks
    ) {
        std::cout
            << "Result: Dynamic scheduling reduced "
            << "average waiting time.\n";
    } else if (
        dynamic.averageWaitTicks >
        fifo.averageWaitTicks
    ) {
        std::cout
            << "Result: FIFO produced a lower "
            << "average waiting time in this run.\n";
    } else {
        std::cout
            << "Result: Both strategies produced "
            << "the same average waiting time.\n";
    }
}

bool parseSeed(
    int argc,
    char* argv[],
    unsigned int& seed
) {
    seed = 42;

    for (int index = 1; index < argc; ++index) {
        const std::string argument =
            argv[index];

        if (argument == "--seed") {
            if (index + 1 >= argc) {
                std::cerr
                    << "Error: --seed requires "
                    << "a non-negative integer.\n";

                return false;
            }

            const std::string seedText =
                argv[++index];

            try {
                std::size_t charactersRead = 0;

                const unsigned long parsedSeed =
                    std::stoul(
                        seedText,
                        &charactersRead
                    );

                if (
                    charactersRead !=
                    seedText.size()
                ) {
                    throw std::invalid_argument(
                        "Seed contains invalid characters"
                    );
                }

                seed =
                    static_cast<unsigned int>(
                        parsedSeed
                    );
            } catch (const std::exception&) {
                std::cerr
                    << "Error: invalid seed value: "
                    << seedText
                    << '\n';

                return false;
            }
        } else {
            std::cerr
                << "Error: unknown argument: "
                << argument
                << '\n';

            std::cerr
                << "Usage: .\\scheduler.exe "
                << "[--seed NUMBER]\n";

            return false;
        }
    }

    return true;
}

int main(
    int argc,
    char* argv[]
) {
    unsigned int seed = 42;

    if (!parseSeed(
            argc,
            argv,
            seed
        )) {
        return 1;
    }

    std::cout
        << "Random seed: "
        << seed
        << "\n\n";

    const std::time_t scenarioStart =
        std::time(nullptr);

    OrderGenerator generator(seed);

    const std::vector<Order> normalOrders =
        generator.generateScenario(
            DemandScenario::Normal,
            10,
            scenarioStart
        );

    std::cout
        << "================ NORMAL DEMAND ================\n";

    displayGeneratedOrders(
        normalOrders,
        scenarioStart
    );

    const std::time_t rushStart =
        scenarioStart + 600;

    const std::vector<Order> rushOrders =
        generator.generateScenario(
            DemandScenario::Rush,
            20,
            rushStart
        );

    std::cout
        << "\n================ RUSH DEMAND ==================\n";

    displayGeneratedOrders(
        rushOrders,
        rushStart
    );

    SimulationRunner simulationRunner;

    const SimulationMetrics fifoMetrics =
        simulationRunner.run(
            rushOrders,
            SchedulingStrategy::FIFO,
            rushStart
        );

    const SimulationMetrics dynamicMetrics =
        simulationRunner.run(
            rushOrders,
            SchedulingStrategy::DynamicBatching,
            rushStart
        );

    std::cout
        << "\n================ STRATEGY COMPARISON ================\n";

    displayMetrics(fifoMetrics);
    displayMetrics(dynamicMetrics);

    displayComparison(
        fifoMetrics,
        dynamicMetrics
    );

    runEquipmentSimulation(rushOrders);

    return 0;
}