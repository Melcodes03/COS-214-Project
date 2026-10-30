#ifndef DEPTHFIRSTITERATOR_H
#define DEPTHFIRSTITERATOR_H

#include "WorkItemIterator.h"
#include <vector> 

//ConcreteIterator

class DepthFirstIterator : public WorkItemIterator{
    private: 
        WorkItem* root;
        std::vector<WorkItem*> stack;

    public:
        DepthFirstIterator(WorkItem* root);

        void first() override;
        void next() override;
        bool isDone() const override;
        WorkItem* currentItem() const override;

};
#endif //DEPTHFIRSTITERATOR_H