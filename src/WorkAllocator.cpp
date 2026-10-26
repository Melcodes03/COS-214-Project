#include "WorkAllocator.h"
#include "WorkItem.h"
#include "Participant.h"
#include "AssignmentStrategy.h"
#include "AssignmentHandler.h"

WorkAllocator::WorkAllocator(AssignmentStrategy* strategy) : strategy(strategy), chain(nullptr) {}

WorkAllocator::~WorkAllocator() {
    delete strategy;
    delete chain;
}

void WorkAllocator::setStrategy(AssignmentStrategy* newStrategy) {
    if (newStrategy == nullptr || newStrategy == strategy) {
        return;
    }
    delete strategy;
    strategy = newStrategy;
}

void WorkAllocator::setChain(AssignmentHandler* firstLevel) {
    if (firstLevel == chain) {
        return;
    }
    delete chain;
    chain = firstLevel;
    holder.clear();   //old levels are gone
}

void WorkAllocator::addParticipant(Participant* p) {
    if (p != nullptr) {
        participants.push_back(p);
    }
}

Participant* WorkAllocator::allocate(WorkItem& item) {
    Participant* chosen = nullptr;
    if (chain != nullptr) {
        chosen = chain->handle(item, participants);
    } else if (strategy != nullptr) {
        chosen = strategy->choose(item, participants);
    }
    return give(item, chosen);
}

Participant* WorkAllocator::escalate(WorkItem& item) {
    if (chain == nullptr) {
        return nullptr;
    }
    Participant* current = getAssignee(item.getId());
    if (!item.escalate()) {
        return nullptr;   //not Assigned or InProgress, so there is nothing to escalate
    }
    if (current != nullptr) {
        current->finishTask();   //they no longer hold it
    }
    return passUp(item);
}

Participant* WorkAllocator::reassign(WorkItem& item) {
    if (chain == nullptr) {
        return nullptr;
    }
    return passUp(item);
}

Participant* WorkAllocator::passUp(WorkItem& item) {
    AssignmentHandler* level = getLevel(item.getId());
    AssignmentHandler* above = level == nullptr ? chain : level->getNext();
    if (above == nullptr) {
        return nullptr;   //already at the top of the chain
    }
    return give(item, above->handle(item, participants));
}

Participant* WorkAllocator::give(WorkItem& item, Participant* chosen) {
    if (chosen == nullptr || !item.assign()) {
        return nullptr;   //nobody suitable, or the item is not in a state that can be assigned
    }
    chosen->receive(&item);
    assignee[item.getId()] = chosen;
    if (chain != nullptr) {
        holder[item.getId()] = chain->levelOf(*chosen);
    }
    return chosen;
}

void WorkAllocator::release(const std::string& itemId) {
    assignee.erase(itemId);
    holder.erase(itemId);
}

Participant* WorkAllocator::getAssignee(const std::string& itemId) const {
    std::map<std::string, Participant*>::const_iterator found = assignee.find(itemId);
    return found == assignee.end() ? nullptr : found->second;
}

AssignmentHandler* WorkAllocator::getLevel(const std::string& itemId) const {
    std::map<std::string, AssignmentHandler*>::const_iterator found = holder.find(itemId);
    return found == holder.end() ? nullptr : found->second;
}
