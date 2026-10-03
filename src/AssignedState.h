#ifndef ASSIGNEDSTATE_H
#define ASSIGNEDSTATE_H

#include "WorkItemState.h"

//State: AssignedState.h
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