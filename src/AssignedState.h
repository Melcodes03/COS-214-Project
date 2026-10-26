#ifndef ASSIGNEDSTATE_H
#define ASSIGNEDSTATE_H

#include "WorkItemState.h"

//State: AssignedState.h
/**
 * @brief State: ConcreteState. Given to a participant. It can be started, declined (back to Available), escalated or cancelled.
 */
class AssignedState : public WorkItemState {
    public:
        static WorkItemState* instance();
        bool start(WorkItem& item) override;
        bool cancel(WorkItem& item) override;
        bool reject(WorkItem& item) override;
        bool escalate(WorkItem& item) override;
        std::string getName() const override;
    private:
        AssignedState() {}
};

#endif //ASSIGNEDSTATE_H