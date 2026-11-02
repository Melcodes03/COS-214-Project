#ifndef PARTICIPANTOBSERVER_H
#define PARTICIPANTOBSERVER_H

#include "Observer.h"
#include "WorkItemState.h"
#include "WorkItem.h"
#include "CommunicationAdapter.h"
#include "Participant.h"

// CONCRETE OBSERVER

class Participant;

/** @brief Observer that records state changes relevant to a participant. */
class ParticipantObserver : public Observer
{
private:
    WorkItemState *observerState; // last state seen
    std::string participantName;
    std::string interestedState;
    std::vector<std::string> inbox;
    CommunicationAdapter* channel;
    Participant* participant;

public:
    /**
     * @brief Constructs a participant observer.
     * @param subject Work item to observe.
     * @param interestedState State for which notifications are collected.
     * @param p Participant receiving notifications.
     */
    ParticipantObserver(WorkItem *subject, std::string interestedState, Participant* p);
    virtual ~ParticipantObserver();
    void addContactMethod(CommunicationAdapter* channel);
    std::vector<std::string> &getInbox();
    void setSubject(WorkItem *);
    virtual void update(WorkItemState *newState, std::string oldState) override;
    void setParticipant(Participant* p);
};

#endif