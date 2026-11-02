#ifndef SINGLEAPPROVAL_H
#define SINGLEAPPROVAL_H

#include "ApprovalStrategy.h"

//Strategy: ConcreteStrategy. One approver decides.
/** @brief Resolves an approval request using a single approver vote. */
class SingleApproval : public ApprovalStrategy {
    public:
        /** @brief Resolves approval from the first approver vote. @param votes Approver votes. @return The resulting approval decision. */
        Decision decide(const std::vector<bool>& votes) const override;
};

#endif //SINGLEAPPROVAL_H
