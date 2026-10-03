#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "WorkItemState.h"

//State: CompletedState.h
class CompletedState : public WorkItemState {
    public:
        static WorkItemState* instance();
        std::string getName() const override;
    private:
        CompletedState() {}
};

#endif //COMPLETEDSTATE_H