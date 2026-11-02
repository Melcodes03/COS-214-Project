#ifndef GENERALOBSERVER_H
#define GENERALOBSERVER_H

#include "Observer.h"
#include "WorkItemState.h"
#include "WorkItem.h"
#include "CommunicationAdapter.h"

// CONCRETE OBSERVER

/** @brief Observer that sends work-item state notifications through a communication adapter. */
class generalObserver : public Observer
{
private:
    WorkItemState *observerState;
    CommunicationAdapter *channel; // sms or email
    std::string recipient;

public:
    /**
     * @brief Constructs a general work-item observer.
     * @param subject Work item whose state changes are observed.
     * @param channel Communication adapter used for notifications.
     * @param recipient Notification destination.
     */
    generalObserver(WorkItem *subject, CommunicationAdapter *channel, std::string recipient);
    virtual ~generalObserver();
    void setSubject(WorkItem *);
    WorkItemState *getObservedState();
    virtual void update(WorkItemState *newState, std::string oldState) override;
};

#endif