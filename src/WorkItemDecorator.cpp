#include "WorkItemDecorator.h"

WorkItemDecorator::WorkItemDecorator(WorkItem* component)
    : WorkItem(component->getId(), component->getName()), wrapped(component){}

WorkItemDecorator::~WorkItemDecorator(){
    delete wrapped;
}

std::string WorkItemDecorator::getId() const { return wrapped->getId(); }
std::string WorkItemDecorator::getName() const { return wrapped->getName(); }

bool WorkItemDecorator::add(WorkItem* child) { return wrapped->add(child); }
WorkItem* WorkItemDecorator::getChild(int index) const { return wrapped->getChild(index); }
int WorkItemDecorator::getChildCount() const { return wrapped->getChildCount(); }

bool WorkItemDecorator::makeAvailable() { return wrapped->makeAvailable(); }
bool WorkItemDecorator::assign() { return wrapped->assign(); }
bool WorkItemDecorator::start() { return wrapped->start(); }
bool WorkItemDecorator::complete() { return wrapped->complete(); }
bool WorkItemDecorator::reject() { return wrapped->reject(); }
bool WorkItemDecorator::escalate() { return wrapped->escalate(); }
bool WorkItemDecorator::cancel() { return wrapped->cancel(); }
std::string WorkItemDecorator::getStateName() const { return wrapped->getStateName(); }
void WorkItemDecorator::setState(WorkItemState* newState) { wrapped->setState(newState); }

int WorkItemDecorator::getRetryCount() const { return wrapped->getRetryCount(); }
int WorkItemDecorator::getMaxRetries() const { return wrapped->getMaxRetries(); }
void WorkItemDecorator::setMaxRetries(int max) { wrapped->setMaxRetries(max); }
int WorkItemDecorator::getPriority() const { return wrapped->getPriority(); }