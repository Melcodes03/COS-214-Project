#ifndef PRIORITYDECORATOR_H
#define PRIORITYDECORATOR_H

#include "WorkItemDecorator.h"

//Decorator: ConcreteDecorator. Gives a work item a priority
class PriorityDecorator : public WorkItemDecorator {
    private:
        int priority;

    public:
        PriorityDecorator(WorkItem* component, int priority);

        int getPriority() const override;
        WorkItem* clone() const override;
};

#endif //PRIORITYDECORATOR_H