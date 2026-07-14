// handles order related helper functions. Order.cpp converts between computer and human reading the display.

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

    return "?";
}

std::string statusName(Status status) {
    switch (status) {
        case Status::Waiting:
            return "Waiting";

        case Status::InProgress:
            return "In Progress";

        case Status::Complete:
            return "Complete";
    }

    return "?";
}