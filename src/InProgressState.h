#ifndef INPROGRESSSTATE_H
#define INPROGRESSSTATE_H

#include "WorkItemState.h"

//State: AssignedState.h
class InProgressState : public WorkItemState {
    public:
        static WorkItemState* instance();
        bool complete(WorkItem& item) override;
        bool cancel(WorkItem& item) override;
        bool reject(WorkItem& item) override;
        bool escalate(WorkItem& item) override;
        std::string getName() const override;
    private:
        InProgressState() {}
};

#endif //INPROGRESSSTATE_H