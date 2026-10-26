#include "AllocationCommand.h"
#include "WorkAllocator.h"
#include "Participant.h"
#include "WorkItemMemento.h"

AllocationCommand::AllocationCommand(WorkAllocator* allocator, WorkItem* item,
                                     const std::vector<Participant*>& everyone,
                                     Operation operation, const std::string& name)
    : allocator(allocator), item(item), everyone(everyone), operation(operation), name(name),
      before(nullptr), previousAssignee(nullptr), previousLevel(nullptr){}

AllocationCommand::~AllocationCommand(){
    delete before;
}

bool AllocationCommand::participantsChanged() const {
    for (int i = 0; i < (int)saved.size(); i++){
        if (saved[i].who->getActiveTasks() != saved[i].active ||
            saved[i].who->getWorklist() != saved[i].worklist){
            return true;
        }
    }
    return false;
}

bool AllocationCommand::execute(){
    if (allocator == nullptr || item == nullptr || operation == nullptr){
        return false;
    }
    delete before;
    before = item->createMemento();
    saved.clear();
    for (int i = 0; i < (int)everyone.size(); i++){
        if (everyone[i] == nullptr) continue;
        Workload w;
        w.who = everyone[i];
        w.active = everyone[i]->getActiveTasks();
        w.worklist = everyone[i]->getWorklist();
        saved.push_back(w);
    }
    previousAssignee = allocator->getAssignee(item->getId());
    previousLevel = allocator->getLevel(item->getId());
    std::string stateBefore = item->getStateName();

    Participant* result = (allocator->*operation)(*item);

    bool changed = result != nullptr || item->getStateName() != stateBefore || participantsChanged();
    if (!changed){
        delete before;   //nothing happened, so there is nothing to undo
        before = nullptr;
        saved.clear();
        return false;
    }
    return true;
}

void AllocationCommand::undo(){
    if (before == nullptr){
        return;   //execute() did not succeed
    }
    item->restore(*before);   //also tells the item's observers about the state change
    for (int i = 0; i < (int)saved.size(); i++){
        Participant* p = saved[i].who;
        while (p->getActiveTasks() > saved[i].active) p->finishTask();
        while (p->getActiveTasks() < saved[i].active) p->addTask();
        p->getWorklist() = saved[i].worklist;
    }
    allocator->restoreAssignment(item->getId(), previousAssignee, previousLevel);
}

std::string AllocationCommand::getName() const {
    return name;
}
