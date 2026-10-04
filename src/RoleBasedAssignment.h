#ifndef ROLEBASEDASSIGNMENT_H
#define ROLEBASEDASSIGNMENT_H

#include "AssignmentStrategy.h"
#include <string>

//Strategy: ConcreteStrategy. Give the task to the first participant with the required role.
class RoleBasedAssignment : public AssignmentStrategy {
    private:
        std::string requiredRole;

    public:
        explicit RoleBasedAssignment(const std::string& requiredRole);
        Participant* choose(const WorkItem& item,
                            const std::vector<Participant*>& candidates) const override;
};

#endif //ROLEBASEDASSIGNMENT_H
