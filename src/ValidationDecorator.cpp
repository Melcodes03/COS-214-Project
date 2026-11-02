#include "ValidationDecorator.h"

ValidationDecorator::ValidationDecorator(WorkItem* component, Rule rule)
    : WorkItemDecorator(component), rule(rule){}

bool ValidationDecorator::complete() {
    if (!rule(*wrapped)) {
        return false;   //validation failed: the item stays where it is
    }
    return wrapped->complete();
}

WorkItem* ValidationDecorator::clone() const {
    return new ValidationDecorator(wrapped->clone(), rule);
}