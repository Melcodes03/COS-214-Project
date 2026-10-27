#ifndef SINGLEAPPROVAL_H
#define SINGLEAPPROVAL_H

#include "ApprovalStrategy.h"

//Strategy: ConcreteStrategy. One approver decides.
class SingleApproval : public ApprovalStrategy {
    public:
        Decision decide(const std::vector<bool>& votes) const override;
};

#endif //SINGLEAPPROVAL_H
