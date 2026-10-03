#ifndef CANCELLEDSTATE_H
#define CANCELLEDSTATE_H

#include "WorkItemState.h"

//State: CancelledState.h
class CancelledState : public WorkItemState {
    public:
        static WorkItemState* instance();
        std::string getName() const override;
    private:
        CancelledState() {}
};

#endif //CANCELLEDSTATE_H