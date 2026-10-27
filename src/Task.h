#ifndef TASK_H
#define TASK_H

#include "WorkItem.h"

//Composite: leaf
class Task : public WorkItem {
    public: 
        Task(const std::string& id, const std::string& name);
        WorkItem* clone() const override;
        void accept(WorkItemVisitor& visitor) override;   //Visitor: ConcreteElement
}; 

#endif //TASK_H