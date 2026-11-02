#include "generalObserver.h"

generalObserver::generalObserver(WorkItem *subject, CommunicationAdapter *channel, std::string recipient) : Observer(subject), observerState(subject->getState()), channel(channel), recipient(recipient) {}

generalObserver::~generalObserver()
{
}

void generalObserver::update(WorkItemState *newState, std::string oldState)
{
    observerState = newState;
    this->oldState = oldState;
    channel->send(recipient, "Work item " + subject->getId() + " (" + subject->getName() + "): " + oldState + " -> " + newState->getName());
}

WorkItemState *generalObserver::getObservedState()
{
    return observerState;
}

void generalObserver::setSubject(WorkItem *c)
{
    if (c)
    {
        subject = c;
    }
}
