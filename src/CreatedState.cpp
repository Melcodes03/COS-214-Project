#include "CreatedState.h"
#include "WorkItem.h"
#include "AvailableState.h"
#include "CancelledState.h"

WorkItemState* CreatedState::instance() {
    static CreatedState s;
    return &s;
}

bool CreatedState::makeAvailable(WorkItem& item) {
    item.setState(AvailableState::instance());
    return true;
}

bool CreatedState::cancel(WorkItem& item) {
    item.setState(CancelledState::instance());
    return true;
}

std::string CreatedState::getName() const { return "Created"; }