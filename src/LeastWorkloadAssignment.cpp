#include "LeastWorkloadAssignment.h"
#include "Participant.h"

Participant* LeastWorkloadAssignment::choose(const WorkItem&, const std::vector<Participant*>& candidates) const {
    Participant* best = nullptr;
    for (int i = 0; i < (int)candidates.size(); i++) {
        if (best == nullptr || candidates[i]->getActiveTasks() < best->getActiveTasks()) {
            best = candidates[i];
        }
    }
    return best;
}
