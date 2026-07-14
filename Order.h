// defines what an order IS

#ifndef ORDER_H
#define ORDER_H

#include <string>
#include <ctime>

enum class Source {
    DriveThru,
    Mobile,
    EatIn
};

enum class Status {
    Waiting,
    InProgress,
    Complete
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
};

std::string sourceName(Source source);
std::string statusName(Status status);

#endif