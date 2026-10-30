#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include "WorkItemState.h"

//State: CancelledState.h
/**
 * @brief State: ConcreteState. Final state: no operation is allowed any more.
 */
class CancelledState : public WorkItemState {
    public:
        static WorkItemState* instance();
        std::string getName() const override;
    private:
        CancelledState() {}
};

#endif //CANCELLEDSTATE_H