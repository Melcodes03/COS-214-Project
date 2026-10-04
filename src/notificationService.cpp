#include "notificationService.h"

notificationService::notificationService(WorkItem* subject) : Observer(subject){
    this->update(subject->getState());
}

notificationService::~notificationService(){

}

void notificationService::update(WorkItemState* newState){
    observerState = newState;
}