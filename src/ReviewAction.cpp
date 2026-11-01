#include "ReviewAction.h"
#include "Participant.h"
#include "WorkItem.h"

Participant* FlagForReview::review(WorkItem&, const std::vector<Participant*>&) {
    return nullptr;
}

std::string FlagForReview::describe() const { return "flag it and re-offer it at the next review"; }

OverflowToLeastBusy::OverflowToLeastBusy(const std::vector<std::string>& roles) : roles(roles) {}

Participant* OverflowToLeastBusy::review(WorkItem& item, const std::vector<Participant*>& candidates) {
    Participant* best = nullptr;
    for (int i = 0; i < (int)candidates.size(); i++) {
        Participant* p = candidates[i];
        if (p == nullptr || !p->isAvailable()) continue;
        int level = 0;
        for (int r = 0; r < (int)roles.size(); r++) {
            if (roles[r] == p->getRole()) level = r + 1;
        }
        if (level < item.getPriority()) continue;   //below the level the work needs
        if (best == nullptr || p->getActiveTasks() < best->getActiveTasks()) best = p;
    }
    return best;
}

std::string OverflowToLeastBusy::describe() const { return "give it to the least busy person at its level or above"; }
