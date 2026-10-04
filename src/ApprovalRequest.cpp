#include "ApprovalRequest.h"
#include "WorkItem.h"

ApprovalRequest::ApprovalRequest(WorkItem* item, ApprovalStrategy* strategy)
    : item(item), strategy(strategy), decision(ApprovalStrategy::PENDING) {}

ApprovalRequest::~ApprovalRequest() { delete strategy; }

ApprovalStrategy::Decision ApprovalRequest::vote(bool approve) {
    if (decision != ApprovalStrategy::PENDING) {
        return decision;
    }
    votes.push_back(approve);
    decision = strategy->decide(votes);
    if (decision == ApprovalStrategy::APPROVED) {
        item->complete();   //goes through State (and Validation, if decorated)
    } else if (decision == ApprovalStrategy::REJECTED) {
        item->reject();
    }
    return decision;
}

bool ApprovalRequest::deadlinePassed() {
    if (decision != ApprovalStrategy::PENDING) {
        return false;
    }
    return item->escalate();
}

ApprovalStrategy::Decision ApprovalRequest::getDecision() const { return decision; }
