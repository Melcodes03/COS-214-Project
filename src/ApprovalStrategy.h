#ifndef APPROVALSTRATEGY_H
#define APPROVALSTRATEGY_H

#include <vector>

//Strategy: abstract Strategy. The rule that turns approvers' votes into a decision.
/** @brief Determines an approval decision from approvers' votes. */
class ApprovalStrategy {
    public:
        enum Decision { PENDING, APPROVED, REJECTED };

        virtual ~ApprovalStrategy() {}
        /**
         * @brief Converts approver votes into a decision.
         * @param votes Approval votes, where true represents approval and false represents rejection.
         * @return The resulting approval decision.
         */
        virtual Decision decide(const std::vector<bool>& votes) const = 0;   //true = approve
};

#endif //APPROVALSTRATEGY_H
