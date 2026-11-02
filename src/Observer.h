#ifndef OBSERVER_H
#define OBSERVER_H

//ABSTRACT OBSERVER
#include "WorkItem.h"
#include "WorkItemState.h"

/** @brief Abstract observer for work-item state changes. */
class Observer{
    protected:
        WorkItem* subject;
        std::string oldState;

    public:
        Observer(WorkItem* subject);
        virtual ~Observer();
        /**
         * @brief Handles an observed work-item state change.
         * @param newState New work-item state.
         * @param oldState Name of the previous state.
         */
        virtual void update(WorkItemState* newState, std::string oldState) = 0;

};

#endif