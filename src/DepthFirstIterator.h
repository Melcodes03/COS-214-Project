#ifndef DEPTHFIRSTITERATOR_H
#define DEPTHFIRSTITERATOR_H

#include "WorkItemIterator.h"
#include <vector> 

//ConcreteIterator

/** @brief Iterates through a work-item hierarchy in depth-first order. */
class DepthFirstIterator : public WorkItemIterator{
    private: 
        WorkItem* root;
        std::vector<WorkItem*> stack;

    public:
        /** @brief Constructs an iterator rooted at a work item. @param root Work item from which traversal begins. */
        DepthFirstIterator(WorkItem* root);

        void first() override;
        void next() override;
        bool isDone() const override;
        WorkItem* currentItem() const override;

};
#endif //DEPTHFIRSTITERATOR_H