#ifndef ASSIGNMENTSTRATEGY_H
#define ASSIGNMENTSTRATEGY_H

#include <vector>

class WorkItem;
class Participant;

//Strategy: abstract Strategy. The rule an organisation uses to decide who gets a task.
/** @brief Selects a participant to assign a work item. */
class AssignmentStrategy {
    public:
        virtual ~AssignmentStrategy() {}

        //returns nullptr if nobody suitable is available
        /**
         * @brief Chooses a suitable participant for a work item.
         * @param item Work item to assign.
         * @param candidates Participants available for assignment.
         * @return The selected participant, or nullptr when none is suitable.
         */
        virtual Participant* choose(const WorkItem& item,
                                    const std::vector<Participant*>& candidates) const = 0;
};

#endif //ASSIGNMENTSTRATEGY_H
