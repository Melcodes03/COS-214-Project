#include "RoleBasedAssignment.h"
#include "Participant.h"

RoleBasedAssignment::RoleBasedAssignment(const std::string& requiredRole) : requiredRole(requiredRole) {}

Participant* RoleBasedAssignment::choose(const WorkItem&, const std::vector<Participant*>& candidates) const {
    for (int i = 0; i < (int)candidates.size(); i++) {
        if (candidates[i]->getRole() == requiredRole) {
            return candidates[i];
        }
    }
    return nullptr;
}
