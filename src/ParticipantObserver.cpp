#include "ParticipantObserver.h"

ParticipantObserver::ParticipantObserver(WorkItem* subject, std::string participantName,  std::string interestedState = "") : Observer(subject), observerState(subject->getState()),
      participantName(participantName), interestedState(interestedState){
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
}

void ParticipantObserver::setSubject(WorkItem* sub){
    subject = sub;
}