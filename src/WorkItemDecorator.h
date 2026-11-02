#ifndef WORKITEMDECORATOR_H
#define WORKITEMDECORATOR_H

#include "WorkItem.h"

class Observer;
/**
 * @brief Decorator: adds behaviour to a work item without changing what it is.
 *
 * A decorator IS a WorkItem that owns another WorkItem and forwards every operation
 * to it, so the rest of the system cannot tell a plain item from a decorated one.
 * Concrete decorators override only what they add, and decorators can be stacked,
 * for example Auditing(Validation(Priority(Task))). The outer layer sees each call first.
 */
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

        //Routing and engine support: forwarded, because the real item is the wrapped one
        std::string getRequiredRole() const override;
        void setRequiredRole(const std::string& role) override;
        bool isAvailable() const override;
        bool isCompleted() const override;
        bool isCancelled() const override;
        bool advance(const DataMap& data) override;

        //Memento: the real state lives in the wrapped item, so snapshots go through it
        WorkItemMemento* createMemento() const override;
        bool restore(const WorkItemMemento& memento) override;

        //Visitor: a decorated item is still a Task or a Stage underneath
        void accept(WorkItemVisitor& visitor) override;

        //Observer Pattern
        void attach(Observer*) override;
        void detach(Observer*) override;
        void notify(std::string oldState) override;
};

#endif //WORKITEMDECORATOR_H