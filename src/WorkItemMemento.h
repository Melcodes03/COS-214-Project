#ifndef WORKITEMMEMENTO_H
#define WORKITEMMEMENTO_H

#include <vector>

class WorkItem;
class WorkItemState;

//Memento: opaque snapshot of a WorkItem and everything under it.
//Only WorkItem (the Originator) can create it or read it. The caretaker
//(TransitionCommand, CommandHistory) can only hold it and hand it back.
//It owns its child snapshots, so a snapshot of a Stage is a full deep copy
//of the stage's lifecycle data, not a shallow one.
class WorkItemMemento {
    friend class WorkItem;

    private:
        WorkItemState* state;   //states are shared singletons, so a pointer is a complete copy
        int retryCount;
        std::vector<WorkItemMemento*> children;

        WorkItemMemento(WorkItemState* state, int retryCount);
        void addChild(WorkItemMemento* child);   //takes ownership

    public:
        ~WorkItemMemento();
        WorkItemMemento(const WorkItemMemento&) = delete;
        WorkItemMemento& operator=(const WorkItemMemento&) = delete;
};

#endif //WORKITEMMEMENTO_H
