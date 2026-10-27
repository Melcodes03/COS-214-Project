#ifndef APPROVALSTRATEGY_H
#define APPROVALSTRATEGY_H

#include <vector>

//Strategy: abstract Strategy. The rule that turns approvers' votes into a decision.
class ApprovalStrategy {
    public:
        enum Decision { PENDING, APPROVED, REJECTED };

        virtual ~ApprovalStrategy() {}
        virtual Decision decide(const std::vector<bool>& votes) const = 0;   //true = approve
};

#endif //APPROVALSTRATEGY_H
