#ifndef REJECTEDSTATE_H
#define REJECTEDSTATE_H

#include "WorkItemState.h"

//State: RejectedState.h
/**
 * @brief State: ConcreteState. Refused after work. It can be reassigned for rework only while retries remain, or cancelled.
 */
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