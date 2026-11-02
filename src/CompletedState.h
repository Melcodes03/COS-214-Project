#ifndef COMPLETEDSTATE_H
#define COMPLETEDSTATE_H

#include "WorkItemState.h"

//State: CompletedState.h
/**
 * @brief State: ConcreteState. Final state: no operation is allowed any more.
 */
class CompletedState : public WorkItemState {
    public:
        static WorkItemState* instance();
        std::string getName() const override;
    private:
        CompletedState() {}
};

#endif //COMPLETEDSTATE_H