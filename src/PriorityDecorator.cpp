#include "PriorityDecorator.h"

PriorityDecorator::PriorityDecorator(WorkItem* component, int priority)
    : WorkItemDecorator(component), priority(priority){}

int PriorityDecorator::getPriority() const { return priority; }

WorkItem* PriorityDecorator::clone() const {
    return new PriorityDecorator(wrapped->clone(), priority);
}