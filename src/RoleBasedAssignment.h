#ifndef ROLEBASEDASSIGNMENT_H
#define ROLEBASEDASSIGNMENT_H

#include "AssignmentStrategy.h"
#include <string>

//Strategy: ConcreteStrategy. Give the task to the first participant with the required role.
/** @brief Assigns work items to participants having a required role. */
class RoleBasedAssignment : public AssignmentStrategy {
    private:
        std::string requiredRole;

    public:
        /** @brief Constructs a role-based assignment strategy. @param requiredRole Role a candidate must hold. */
        explicit RoleBasedAssignment(const std::string& requiredRole);
        Participant* choose(const WorkItem& item,
                            const std::vector<Participant*>& candidates) const override;
};

#endif //ROLEBASEDASSIGNMENT_H
