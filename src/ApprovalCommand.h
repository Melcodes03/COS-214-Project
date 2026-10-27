#ifndef APPROVALCOMMAND_H
#define APPROVALCOMMAND_H

#include <string>
#include "Command.h"

class ApprovalRequest;
class WorkItem;
class WorkItemMemento;

/// @brief Command pattern: ConcreteCommand. One approver's vote on an ApprovalRequest, undoably.
///
/// A vote can finish the approval: once the rule reaches a decision, the request completes or
/// rejects the work item. Undo therefore has to take back two things: the vote itself, and
/// whatever the vote did to the item (restored from a WorkItemMemento, so the item's
/// observers and audit log hear about it like any other undo).
///
/// @see ApprovalRequest, TransitionCommand
class ApprovalCommand : public Command {
    public:
        /**
         * @param request  the approval being voted on; not owned
         * @param item     the item the request is about (the same one the request was made for); not owned
         * @param approve  true to approve, false to reject
         * @param name     label for history listings
         */
        ApprovalCommand(ApprovalRequest* request, WorkItem* item, bool approve, const std::string& name);
        ~ApprovalCommand() override;

        ApprovalCommand(const ApprovalCommand&) = delete;
        ApprovalCommand& operator=(const ApprovalCommand&) = delete;

        /// @return false, changing nothing, if the request had already reached a decision (the vote would be ignored).
        bool execute() override;

        /// @brief Take back the vote and restore the item.
        void undo() override;

        std::string getName() const override;

    private:
        ApprovalRequest* request;
        WorkItem* item;
        bool approve;
        std::string name;
        WorkItemMemento* before;   ///< item snapshot taken by execute(); owned
        bool voted;                ///< did execute() record a vote?
};

#endif //APPROVALCOMMAND_H
