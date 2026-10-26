#ifndef WORKALLOCATOR_H
#define WORKALLOCATOR_H

#include <map>
#include <string>
#include <vector>

class WorkItem;
class Participant;
class AssignmentStrategy;
class AssignmentHandler;

/// Gives work items to participants.
///
/// Strategy pattern: Context. Without a chain, the allocator asks its
/// AssignmentStrategy to pick one worker from everyone it knows.
///
/// Chain of Responsibility: Client. When a chain of levels is set, the task is
/// offered to the first level and passed up until a level takes it. If the
/// worker who has it declines or does not respond, the task goes to the next
/// level up, ending with the manager.
class WorkAllocator {
    private:
        AssignmentStrategy* strategy;                         ///< owned
        AssignmentHandler* chain;                             ///< owned; first level, or nullptr
        std::vector<Participant*> participants;               ///< not owned
        std::map<std::string, Participant*> assignee;         ///< item id -> who has it
        std::map<std::string, AssignmentHandler*> holder;     ///< item id -> level that has it

    public:
        explicit WorkAllocator(AssignmentStrategy* strategy);
        ~WorkAllocator();

        WorkAllocator(const WorkAllocator&) = delete;
        WorkAllocator& operator=(const WorkAllocator&) = delete;

        void setStrategy(AssignmentStrategy* newStrategy);

        /// Sets the first level of the escalation chain (takes ownership of the whole chain).
        /// Passing nullptr goes back to using only the strategy.
        void setChain(AssignmentHandler* firstLevel);

        void addParticipant(Participant* p);

        /// Picks a worker and assigns @p item to them.
        /// @return the worker, or nullptr if nobody can take it or the item cannot be assigned now.
        Participant* allocate(WorkItem& item);

        /// The worker did not respond: the item is escalated (Assigned/InProgress -> Escalated),
        /// the worker is freed, and the item is given to the next level up.
        /// @return the new worker, or nullptr if there is no chain, no higher level, or the item
        ///         cannot be escalated or assigned.
        Participant* escalate(WorkItem& item);

        /// The worker declined (Participant::decline() already freed them): the item is given
        /// to the next level up.
        /// @return the new worker, or nullptr as for escalate().
        Participant* reassign(WorkItem& item);

        /// Forgets who holds @p itemId (for example once it is completed or cancelled).
        void release(const std::string& itemId);

        /// Puts back who holds @p itemId and at which level. Used only to undo an allocation
        /// (see AllocationCommand). A null @p who forgets the item. It does not touch the item
        /// or the participants; the caller restores those.
        void restoreAssignment(const std::string& itemId, Participant* who, AssignmentHandler* level);
        Participant* getAssignee(const std::string& itemId) const;

        /// The level of the chain that holds @p itemId, or nullptr if unknown or there is no chain.
        AssignmentHandler* getLevel(const std::string& itemId) const;

    private:
        /// Offers @p item to the level above the one that holds it now.
        Participant* passUp(WorkItem& item);

        /// Assigns @p item to @p chosen and records who and which level has it.
        Participant* give(WorkItem& item, Participant* chosen);
};

#endif //WORKALLOCATOR_H
