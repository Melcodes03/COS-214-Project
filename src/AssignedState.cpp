#include "AssignedState.h"
#include "WorkItem.h"
#include "InProgressState.h"
#include "AvailableState.h"
#include "EscalatedState.h"
#include "CancelledState.h"

WorkItemState* AssignedState::instance() {
    static AssignedState s;
    return &s;
}

bool AssignedState::start(WorkItem& item) {
    item.setState(InProgressState::instance());
    return true;
}

bool AssignedState::reject(WorkItem& item) {
    item.setState(AvailableState::instance());
    return true;
}

bool AssignedState::escalate(WorkItem& item) {
    item.setState(EscalatedState::instance());
    return true;
}

bool AssignedState::cancel(WorkItem& item) {
    item.setState(CancelledState::instance());
    return true;
}

std::string AssignedState::getName() const { return "Assigned"; }