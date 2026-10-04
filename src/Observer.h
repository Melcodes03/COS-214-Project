#ifndef OBSERVER_H
#define OBSERVER_H

//ABSTRACT OBSERVER
#include "WorkItem.h"
#include "WorkItemState.h"

class Observer{
    protected:
        WorkItem* subject;

    public:
        Observer(WorkItem* subject);
        virtual ~Observer();
        virtual void update(WorkItemState* newState) = 0;

};

#endif