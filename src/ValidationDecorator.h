#ifndef VALIDATIONDECORATOR_H
#define VALIDATIONDECORATOR_H

#include "WorkItemDecorator.h"
#include <functional>

//Decorator: ConcreteDecorator. A work item cannot be completed unless its
//validation rule passes (the [validation passes] guard on InProgress -> Completed).
class ValidationDecorator : public WorkItemDecorator {
    public:
        typedef std::function<bool(const WorkItem&)> Rule;

        ValidationDecorator(WorkItem* component, Rule rule);

        bool complete() override;
        WorkItem* clone() const override;

    private:
        Rule rule;
};

#endif //VALIDATIONDECORATOR_H