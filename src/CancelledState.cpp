#include "CancelledState.h"
#include "WorkItem.h"

WorkItemState* CancelledState::instance() {
    static CancelledState s;
    return &s;
}


std::string CancelledState::getName() const { return "Cancelled"; }