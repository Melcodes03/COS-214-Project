#ifndef PARTICIPANTOBSERVER_H
#define PARTICIPANTOBSERVER_H

#include "Observer.h"
#include "WorkItemState.h"
#include "WorkItem.h"
#include "CommunicationAdapter.h"

// CONCRETE OBSERVER

class ParticipantObserver : public Observer
{
private:
    WorkItemState *observerState; // last state seen
    std::string participantName;
    std::string interestedState;
    std::vector<std::string> inbox;
    CommunicationAdapter* channel;

public:
    ParticipantObserver(WorkItem *subject, std::string participantName, std::string interestedState);
    virtual ~ParticipantObserver();
    void addContactMethod(CommunicationAdapter* channel);
    std::vector<std::string> &getInbox();
    void setSubject(WorkItem *);
    virtual void update(WorkItemState *newState, std::string oldState) override;
};

#endif