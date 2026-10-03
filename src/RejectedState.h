#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include "WorkItemState.h"

//State: RejectedState.h
class RejectedState : public WorkItemState {
    public:
        static WorkItemState* instance();
        bool assign(WorkItem& item) override;
        bool cancel(WorkItem& item) override;
        std::string getName() const override;
    private:
        RejectedState() {}
};

#endif //REJECTEDSTATE_H