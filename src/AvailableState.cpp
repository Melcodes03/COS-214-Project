#include "AvailableState.h"
#include "WorkItem.h"
#include "AssignedState.h"
#include "CancelledState.h"

WorkItemState* AvailableState::instance() {
    static AvailableState s;
    return &s;
}

bool AvailableState::assign(WorkItem& item) {
    item.setState(AssignedState::instance());
    return true;
}

bool AvailableState::cancel(WorkItem& item) {
    item.setState(CancelledState::instance());
    return true;
}

std::string AvailableState::getName() const { return "Available"; }