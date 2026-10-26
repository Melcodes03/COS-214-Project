#ifndef ASSIGNMENTHANDLER_H
#define ASSIGNMENTHANDLER_H

#include <vector>

class WorkItem;
class Participant;

/// Chain of Responsibility: Handler.
///
/// One link in the chain of worker levels (for example: first-line staff,
/// specialists, senior staff, manager). A task is offered to the first level;
/// each level either picks a worker for it or passes it on to the next level.
/// The sender (WorkAllocator) only knows the first link, never which level
/// will end up taking the work.
class AssignmentHandler {
    private:
        AssignmentHandler* next;   ///< owned; the next level up, or nullptr at the end

    public:
        AssignmentHandler();
        virtual ~AssignmentHandler();   ///< deletes the rest of the chain

        AssignmentHandler(const AssignmentHandler&) = delete;
        AssignmentHandler& operator=(const AssignmentHandler&) = delete;

        /// Links the next level up. Takes ownership; any previous next level is deleted.
        void setNext(AssignmentHandler* nextHandler);
        AssignmentHandler* getNext() const;

        /// Picks a worker for @p item from @p candidates, or passes the request on.
        /// @return the chosen worker, or nullptr if no level in the rest of the chain can take it.
        virtual Participant* handle(WorkItem& item, const std::vector<Participant*>& candidates) = 0;

        /// True if @p worker belongs to this level (used to remember which level holds a task).
        virtual bool covers(const Participant& worker) const = 0;

        /// Asks the chain which level @p worker belongs to, starting at this link.
        /// @return that level, or nullptr if no level covers the worker.
        AssignmentHandler* levelOf(const Participant& worker);

    protected:
        /// Sends the request to the next level, or returns nullptr if this is the last one.
        Participant* passOn(WorkItem& item, const std::vector<Participant*>& candidates);
};

#endif //ASSIGNMENTHANDLER_H
