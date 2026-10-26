#include "LevelHandler.h"
#include "AssignmentStrategy.h"
#include "Participant.h"
#include "WorkItem.h"

LevelHandler::LevelHandler(const std::string& role, int maxPriority, AssignmentStrategy* strategy)
    : role(role), maxPriority(maxPriority), strategy(strategy) {}

LevelHandler::~LevelHandler() { delete strategy; }

bool LevelHandler::covers(const Participant& worker) const {
    return worker.getRole() == role;
}

//TODO(Stephen's Participant update): also require worker.canTakeWork() once
//Participant has availability and capacity.
bool LevelHandler::isFree(const Participant& worker) const {
    return covers(worker);
}

Participant* LevelHandler::handle(WorkItem& item, const std::vector<Participant*>& candidates) {
    if (item.getPriority() > maxPriority) {
        return passOn(item, candidates);   //not this level's kind of work
    }

    std::vector<Participant*> mine;
    for (int i = 0; i < (int)candidates.size(); i++) {
        if (candidates[i] != nullptr && isFree(*candidates[i])) {
            mine.push_back(candidates[i]);
        }
    }

    Participant* chosen = mine.empty() ? nullptr : strategy->choose(item, mine);
    return chosen != nullptr ? chosen : passOn(item, candidates);
}

std::string LevelHandler::getRole() const { return role; }
int LevelHandler::getMaxPriority() const { return maxPriority; }
