#ifndef APPROVALREQUEST_H
#define APPROVALREQUEST_H

#include <vector>
#include "ApprovalStrategy.h"

class WorkItem;

//Strategy: Context. Collects approvers' votes on a work item that is InProgress
//and, once the approval rule reaches a decision, completes or rejects the item.
//If nobody responds before the deadline, the item is escalated.
class ApprovalRequest {
    private:
        WorkItem* item;               //not owned
        ApprovalStrategy* strategy;   //owned
        std::vector<bool> votes;
        ApprovalStrategy::Decision decision;

    public:
        ApprovalRequest(WorkItem* item, ApprovalStrategy* strategy);   //takes ownership of strategy
        ~ApprovalRequest();
        ApprovalRequest(const ApprovalRequest&) = delete;
        ApprovalRequest& operator=(const ApprovalRequest&) = delete;

        //returns the decision after this vote; votes after a decision are ignored
        ApprovalStrategy::Decision vote(bool approve);
        bool deadlinePassed();   //escalates if still pending; false if nothing to escalate
        ApprovalStrategy::Decision getDecision() const;
};

#endif //APPROVALREQUEST_H
