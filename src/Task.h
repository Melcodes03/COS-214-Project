#ifndef TASK_H
#define TASK_H

#include "WorkItem.h"

//Composite: Leaf. Prototype: ConcretePrototype (clone() copies the task fresh).
/** @brief Leaf work item representing a single task. */
class Task : public WorkItem {
    public: 
        /** @brief Constructs a task. @param id Unique task identifier. @param name Display name of the task. */
        Task(const std::string& id, const std::string& name);
        WorkItem* clone() const override;
        void accept(WorkItemVisitor& visitor) override;   //Visitor: ConcreteElement
}; 

#endif //TASK_H