#include "RejectedState.h"
#include "WorkItem.h"
#include "AssignedState.h"
#include "CancelledState.h"

WorkItemState* RejectedState::instance() {
    static RejectedState s;
    return &s;
}

bool RejectedState::assign(WorkItem& item) {
    if (!item.retriesRemaining()) {
        return false;  //guard [retries remaining] failed; item can only be cancelled
    }
    item.useRetry();
    item.setState(AssignedState::instance());
    return true;
}

bool RejectedState::cancel(WorkItem& item) {
    item.setState(CancelledState::instance());
    return true;
}

std::string RejectedState::getName() const { return "Rejected"; }