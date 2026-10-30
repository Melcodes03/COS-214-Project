#include "CompletedState.h"
#include "WorkItem.h"

WorkItemState* CompletedState::instance() {
    static CompletedState s;
    return &s;
}


std::string CompletedState::getName() const { return "Completed"; }