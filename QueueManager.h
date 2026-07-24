// owns/stores the orders

#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include "Order.h"

#include <vector>
#include <ctime>

class Scheduler;

class QueueManager {
private:
    std::vector<Order> orders;

    Order* findMutableOrderById(int id);

public:
    bool addOrder(const Order& order);
    const Order* findOrderById(int id) const;
    bool removeOrder(int id);

    bool startOrder(int id);
    bool completeOrder(int id);
    bool cancelOrder(int id);

    std::vector<Order> getWaitingOrders() const;
    std::vector<Order> getActiveOrders() const;

    void prioritize(const Scheduler& scheduler, time_t now);
    void displayQueue(time_t now) const;
};


#endif