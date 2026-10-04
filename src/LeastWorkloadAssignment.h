#ifndef LEASTWORKLOADASSIGNMENT_H
#define LEASTWORKLOADASSIGNMENT_H

#include "AssignmentStrategy.h"

//Strategy: ConcreteStrategy. Give the task to whoever currently has the fewest active tasks.
class LeastWorkloadAssignment : public AssignmentStrategy {
    public:
        Participant* choose(const WorkItem& item,
                            const std::vector<Participant*>& candidates) const override;
};

#endif //LEASTWORKLOADASSIGNMENT_H
