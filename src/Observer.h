#ifndef OBSERVER_H
#define OBSERVER_H

//ABSTRACT OBSERVER
#include "WorkItem.h"
#include "WorkItemState.h"

class Observer{
    protected:
        WorkItem* subject;
        std::string oldState;

    public:
        Observer(WorkItem* subject);
        virtual ~Observer();
        virtual void update(WorkItemState* newState, std::string oldState) = 0;

};

#endif