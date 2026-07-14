// owns/stores the orders

#ifndef QUEUE_MANAGER_H
#define QUEUE_MANAGER_H

#include "Order.h"

#include <vector>
#include <ctime>

class QueueManager {
private:
    std::vector<Order> orders;

public:
    void addOrder(const Order& order);

    void completeOrder(int id);

    void displayQueue(time_t now) const;

    std::vector<Order>& getOrders();
};

#endif