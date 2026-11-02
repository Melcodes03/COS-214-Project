#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H

#include <vector>

class WorkItem;
class Participant;

//Strategy: abstract Strategy. The rule an organisation uses to decide who gets a task.
class AssignmentStrategy {
    public:
        virtual ~AssignmentStrategy() {}

        //returns nullptr if nobody suitable is available
        virtual Participant* choose(const WorkItem& item,
                                    const std::vector<Participant*>& candidates) const = 0;
};

#endif //ASSIGNMENTSTRATEGY_H
