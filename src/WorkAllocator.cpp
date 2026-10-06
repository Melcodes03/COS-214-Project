#include "WorkAllocator.h"
#include "WorkItem.h"
#include "Participant.h"
#include "AssignmentStrategy.h"

WorkAllocator::WorkAllocator(AssignmentStrategy* strategy) : strategy(strategy) {}

WorkAllocator::~WorkAllocator() { delete strategy; }

void WorkAllocator::setStrategy(AssignmentStrategy* newStrategy) {
    if (newStrategy == nullptr || newStrategy == strategy) {
        return;
    }
    delete strategy;
    strategy = newStrategy;
}

void WorkAllocator::addParticipant(Participant* p) {
    if (p != nullptr) {
        participants.push_back(p);
    }
}

Participant* WorkAllocator::allocate(WorkItem& item) {
    Participant* chosen = strategy->choose(item, participants);
    if (chosen == nullptr || !item.assign()) {
        return nullptr;   //nobody suitable, or the item is not in a state that can be assigned
    }
    chosen->addTask();
    assignee[item.getId()] = chosen;
    return chosen;
}

Participant* WorkAllocator::getAssignee(const std::string& itemId) const {
    std::map<std::string, Participant*>::const_iterator found = assignee.find(itemId);
    return found == assignee.end() ? nullptr : found->second;
}
