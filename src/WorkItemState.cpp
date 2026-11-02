#include "WorkItemState.h"

bool WorkItemState::makeAvailable(WorkItem&) { return false; }
bool WorkItemState::assign(WorkItem&) { return false; }
bool WorkItemState::start(WorkItem&) { return false; }
bool WorkItemState::complete(WorkItem&)  { return false; }
bool WorkItemState::reject(WorkItem&) { return false; }
bool WorkItemState::escalate(WorkItem&) { return false; }
bool WorkItemState::cancel(WorkItem&) { return false; }