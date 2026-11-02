#ifndef ESCALATEDSTATE_H
#define ESCALATEDSTATE_H

#include "WorkItemState.h"

//State: EscalatedState.h
/**
 * @brief State: ConcreteState. Nobody responded in time. It can be reassigned to someone else, or cancelled.
 */
class EscalatedState : public WorkItemState {
    public:
        static WorkItemState* instance();
        bool assign(WorkItem& item) override;
        bool cancel(WorkItem& item) override;
        std::string getName() const override;
    private:
        EscalatedState() {}
};

#endif //ESCALATEDSTATE_H