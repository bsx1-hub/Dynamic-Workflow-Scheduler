// Converts enum values into user-facing labels.

#include "Order.h"

std::string sourceName(Source source) {
    switch (source) {
        case Source::DriveThru:
            return "Drive-Thru";

        case Source::Mobile:
            return "Mobile";

        case Source::EatIn:
            return "Eat-In";
    }

    return "Unknown";
}

std::string statusName(OrderStatus status) {
    switch (status) {
        case OrderStatus::Waiting:
            return "Waiting";

        case OrderStatus::Scheduled:
            return "Scheduled";

        case OrderStatus::InProgress:
            return "In Progress";

        case OrderStatus::Completed:
            return "Completed";

        case OrderStatus::Cancelled:
            return "Cancelled";
    }

    return "Unknown";
}