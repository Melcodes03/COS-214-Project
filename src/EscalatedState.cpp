#include "EscalatedState.h"
#include "WorkItem.h"
#include "AssignedState.h"
#include "CancelledState.h"

WorkItemState* EscalatedState::instance() {
    static EscalatedState s;
    return &s;
}

bool EscalatedState::assign(WorkItem& item) {
    item.setState(AssignedState::instance());
    return true;
}

bool EscalatedState::cancel(WorkItem& item) {
    item.setState(CancelledState::instance());
    return true;
}

std::string EscalatedState::getName() const { return "Escalated"; }