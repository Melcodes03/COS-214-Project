#ifndef WORKALLOCATOR_H
#define WORKALLOCATOR_H

#include <map>
#include <string>
#include <vector>

class WorkItem;
class Participant;
class AssignmentStrategy;

//Strategy: Context. Hands work to participants using whichever assignment rule
//it was configured with. The rule can be swapped at runtime with setStrategy().
class WorkAllocator {
    private:
        AssignmentStrategy* strategy;               //owned
        std::vector<Participant*> participants;     //not owned
        std::map<std::string, Participant*> assignee;   //work item id -> participant

    public:
        explicit WorkAllocator(AssignmentStrategy* strategy);   //takes ownership
        ~WorkAllocator();
        WorkAllocator(const WorkAllocator&) = delete;
        WorkAllocator& operator=(const WorkAllocator&) = delete;

        void setStrategy(AssignmentStrategy* newStrategy);      //takes ownership
        void addParticipant(Participant* p);

        //Picks a participant with the strategy, then asks the item to move to
        //Assigned (State decides if that is allowed). nullptr if either step fails.
        Participant* allocate(WorkItem& item);
        Participant* getAssignee(const std::string& itemId) const;
};

#endif //WORKALLOCATOR_H
