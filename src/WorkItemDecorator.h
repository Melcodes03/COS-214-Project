#ifndef WORKITEMDECORATOR_H
#define WORKITEMDECORATOR_H

#include "WorkItem.h"

/*Decorator: abstract Decorator
It is a WorkItem that wraps another WorkItem
and forwards every operation to it. Concrete decorators override only what
they add to*/
class WorkItemDecorator : public WorkItem {
    protected:
        WorkItem* wrapped;   //owned: deleted with the decorator

    public:
        //takes ownership of component (must not be null)
        explicit WorkItemDecorator(WorkItem* component);
        ~WorkItemDecorator() override;

        //owns a raw pointer, so copying is disabled (use clone())
        WorkItemDecorator(const WorkItemDecorator&) = delete;
        WorkItemDecorator& operator=(const WorkItemDecorator&) = delete;

        std::string getId() const override;
        std::string getName() const override;

        //Composite: forwarded so a decorated Stage still behaves as a Stage
        bool add(WorkItem* child) override;
        WorkItem* getChild(int index) const override;
        int getChildCount() const override;

        //State: forwarded to the wrapped item
        bool makeAvailable() override;
        bool assign() override;
        bool start() override;
        bool complete() override;
        bool reject() override;
        bool escalate() override;
        bool cancel() override;
        std::string getStateName() const override;
        WorkItemState* getState() const override;
        void setState(WorkItemState* newState) override;

        int getRetryCount() const override;
        int getMaxRetries() const override;
        void setMaxRetries(int max) override;
        void setRetryCount(int count) override;
        int getPriority() const override;

        //Memento: the real state lives in the wrapped item, so snapshots go through it
        WorkItemMemento* createMemento() const override;
        bool restore(const WorkItemMemento& memento) override;
};

#endif //WORKITEMDECORATOR_H