#include "notificationService.h"

notificationService::notificationService(WorkItem *subject, CommunicationAdapter *channel, std::string recipient) : Observer(subject), observerState(subject->getState()), channel(channel), recipient(recipient) {}

notificationService::~notificationService()
{
}

void notificationService::update(WorkItemState *newState, std::string oldState)
{
    observerState = newState;
    this->oldState = oldState;
    channel->send(recipient, "Work item " + subject->getId() + " (" + subject->getName() + "): " + oldState + " -> " + newState->getName());
}

WorkItemState *notificationService::getObservedState()
{
    return observerState;
}
