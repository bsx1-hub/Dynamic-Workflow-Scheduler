// Defines what an order is.

#ifndef ORDER_H
#define ORDER_H

#include <ctime>
#include <string>

enum class Source {
    DriveThru,
    Mobile,
    EatIn
};

enum class OrderStatus {
    Waiting,
    Scheduled,
    InProgress,
    Completed,
    Cancelled
};

struct Order {
    int id;
    std::string drink;
    std::string size;
    bool hot;
    Source source;
    time_t placedAt;
    std::string buildKey;
    OrderStatus status = OrderStatus::Waiting;
};

std::string sourceName(Source source);
std::string statusName(OrderStatus status);

#endif