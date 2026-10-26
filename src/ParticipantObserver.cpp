#include "ParticipantObserver.h"

ParticipantObserver::ParticipantObserver(WorkItem* subject, std::string interestedState, Participant* p)
    : Observer(subject), observerState(subject->getState()), interestedState(interestedState) {
        channel = nullptr;
        setParticipant(p);
}

ParticipantObserver::~ParticipantObserver(){
}

std::vector<std::string>& ParticipantObserver::getInbox() {
    return inbox;
}

void ParticipantObserver::update(WorkItemState* newState, std::string oldState){
    observerState = newState;
    if (!interestedState.empty() && newState->getName() != interestedState){    //not what this participant subscribed to
        return;                                     
    }

    inbox.push_back(participantName + " was told: " + subject->getId() + " " + oldState + " -> " + newState->getName());

    if (channel){
        channel->send(participantName, inbox[inbox.size()-1]);
    }
}

void ParticipantObserver::setSubject(WorkItem* sub){
    subject = sub;
}

void ParticipantObserver::addContactMethod(CommunicationAdapter* channel){
    this->channel = channel;
}

void ParticipantObserver::setParticipant(Participant* p){
    if (p){
        participant = p;
        participantName = p->getName();
    }
}