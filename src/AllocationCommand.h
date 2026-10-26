#ifndef ALLOCATIONCOMMAND_H
#define ALLOCATIONCOMMAND_H

#include <string>
#include <vector>
#include "Command.h"
#include "WorkItem.h"

class WorkAllocator;
class AssignmentHandler;
class Participant;
class WorkItemMemento;

/// @brief Command pattern: ConcreteCommand. Gives a work item to someone (or moves it on), undoably.
///
/// Wraps one WorkAllocator operation: allocate(), escalate() or reassign(). Giving work out
/// changes three things at once, and undo must put all three back:
///  - the item's lifecycle state (restored from a WorkItemMemento),
///  - the workload and worklist of the participants involved (restored from a snapshot),
///  - the allocator's record of who holds the item and at which level.
/// Without this, undoing a decline after the item was re-assigned leaves a worker with a task
/// count for an item they no longer hold.
///
/// @code
/// history.execute(new AllocationCommand(&allocator, item, everyone, &WorkAllocator::allocate,
///                                       "Assign " + item->getName()));
/// @endcode
///
/// @see WorkAllocator, WorkerCommand
class AllocationCommand : public Command {
    public:
        /// A WorkAllocator operation: &WorkAllocator::allocate, ::escalate or ::reassign.
        typedef Participant* (WorkAllocator::*Operation)(WorkItem&);

        /**
         * @param allocator  not owned
         * @param item       the item to give out; not owned
         * @param everyone   every participant the allocator knows. Their workload and worklist are
         *                   snapshotted, because the operation may change any of them.
         * @param operation  which allocator operation to run
         * @param name       label for history listings
         */
        AllocationCommand(WorkAllocator* allocator, WorkItem* item,
                          const std::vector<Participant*>& everyone,
                          Operation operation, const std::string& name);
        ~AllocationCommand() override;

        AllocationCommand(const AllocationCommand&) = delete;
        AllocationCommand& operator=(const AllocationCommand&) = delete;

        /**
         * @brief Run the operation.
         * @return false, changing nothing, if the operation did nothing (nobody could take the
         *         item, or its state does not allow it). Note that escalate() can change the item
         *         even when nobody new takes it; that still counts as done and can be undone.
         */
        bool execute() override;

        /// @brief Put back the item, the participants and the allocator's record.
        void undo() override;

        std::string getName() const override;

    private:
        struct Workload {
            Participant* who;
            int active;
            std::vector<WorkItem*> worklist;
        };

        WorkAllocator* allocator;
        WorkItem* item;
        std::vector<Participant*> everyone;
        Operation operation;
        std::string name;

        WorkItemMemento* before;          ///< item snapshot taken by execute(); owned
        std::vector<Workload> saved;      ///< participant snapshots taken by execute()
        Participant* previousAssignee;    ///< allocator's record before execute()
        AssignmentHandler* previousLevel;

        bool participantsChanged() const;
};

#endif //ALLOCATIONCOMMAND_H
