#ifndef CREATEDSTATE_H
#define CREATEDSTATE_H

#include "WorkItemState.h"

//State: ConcreteState
/**
 * @brief State: ConcreteState. A new item. It can be made available or cancelled.
 */
class CreatedState : public WorkItemState {
    public:
        static WorkItemState* instance();
        bool makeAvailable(WorkItem& item) override;
        bool cancel(WorkItem& item) override;
        std::string getName() const override;
    private:
        CreatedState() {}
};

#endif //CREATEDSTATE_H