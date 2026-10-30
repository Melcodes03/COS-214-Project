#ifndef AVAILABLESTATE_H
#define AVAILABLESTATE_H

#include "WorkItemState.h"

//State: AvailableState
/**
 * @brief State: ConcreteState. Ready to be picked up. It can be assigned or cancelled.
 */
class AvailableState : public WorkItemState {
    public:
        static WorkItemState* instance();
        bool assign(WorkItem& item) override;
        bool cancel(WorkItem& item) override;
        std::string getName() const override;
    private:
        AvailableState() {}
};

#endif //AVAILABLESTATE_H