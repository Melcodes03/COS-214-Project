#include "InProgressState.h"
#include "WorkItem.h"
#include "CompletedState.h"
#include "RejectedState.h"
#include "EscalatedState.h"
#include "CancelledState.h"

WorkItemState* InProgressState::instance() {
    static InProgressState s;
    return &s;
}

bool InProgressState::complete(WorkItem& item) {
    item.setState(CompletedState::instance());
    return true;
}

bool InProgressState::reject(WorkItem& item) {
    item.setState(RejectedState::instance());
    return true;
}

bool InProgressState::escalate(WorkItem& item) {
    item.setState(EscalatedState::instance());
    return true;
}

bool InProgressState::cancel(WorkItem& item) {
    item.setState(CancelledState::instance());
    return true;
}

std::string InProgressState::getName() const { return "InProgress"; }