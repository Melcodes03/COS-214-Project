#include "WorkItemDecorator.h"

WorkItemDecorator::WorkItemDecorator(WorkItem *component)
    : WorkItem(component->getId(), component->getName()), wrapped(component) {}

WorkItemDecorator::~WorkItemDecorator()
{
    delete wrapped;
}

std::string WorkItemDecorator::getId() const { return wrapped->getId(); }
std::string WorkItemDecorator::getName() const { return wrapped->getName(); }

bool WorkItemDecorator::add(WorkItem *child) { return wrapped->add(child); }
WorkItem *WorkItemDecorator::getChild(int index) const { return wrapped->getChild(index); }
int WorkItemDecorator::getChildCount() const { return wrapped->getChildCount(); }

bool WorkItemDecorator::makeAvailable() { return wrapped->makeAvailable(); }
bool WorkItemDecorator::assign() { return wrapped->assign(); }
bool WorkItemDecorator::start() { return wrapped->start(); }
bool WorkItemDecorator::complete() { return wrapped->complete(); }
bool WorkItemDecorator::reject() { return wrapped->reject(); }
bool WorkItemDecorator::escalate() { return wrapped->escalate(); }
bool WorkItemDecorator::cancel() { return wrapped->cancel(); }
std::string WorkItemDecorator::getStateName() const { return wrapped->getStateName(); }
WorkItemState *WorkItemDecorator::getState() const { return wrapped->getState(); }
void WorkItemDecorator::setState(WorkItemState *newState) { wrapped->setState(newState); }

int WorkItemDecorator::getRetryCount() const { return wrapped->getRetryCount(); }
int WorkItemDecorator::getMaxRetries() const { return wrapped->getMaxRetries(); }
void WorkItemDecorator::setMaxRetries(int max) { wrapped->setMaxRetries(max); }
void WorkItemDecorator::setRetryCount(int count) { wrapped->setRetryCount(count); }
int WorkItemDecorator::getPriority() const { return wrapped->getPriority(); }

WorkItemMemento* WorkItemDecorator::createMemento() const { return wrapped->createMemento(); }
bool WorkItemDecorator::restore(const WorkItemMemento& memento) { return wrapped->restore(memento); }

void WorkItemDecorator::accept(WorkItemVisitor& visitor) { wrapped->accept(visitor); }

WorkItemMemento *WorkItemDecorator::createMemento() const { return wrapped->createMemento(); }
bool WorkItemDecorator::restore(const WorkItemMemento &memento) { return wrapped->restore(memento); }

// observer related functions (here they pass on to the wrapped workItem)
void WorkItemDecorator::attach(Observer *observer)
{
    wrapped->attach(observer);
}

void WorkItemDecorator::detach(Observer *observer)
{
    wrapped->detach(observer);
}

void WorkItemDecorator::notify(std::string oldState)
{
    wrapped->notify(oldState);
}
