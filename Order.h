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

enum class Status {
    Waiting,
    InProgress,
    Complete,
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
    Status status = Status::Waiting;

    int estimatedPrepSeconds = 0;
};

std::string sourceName(Source source);
std::string statusName(Status status);

#endif