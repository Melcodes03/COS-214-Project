#ifndef MULTILEVELAPPROVAL_H
#define MULTILEVELAPPROVAL_H

#include "ApprovalStrategy.h"

//Strategy: ConcreteStrategy. Every level must approve, in order; one "no" rejects.
class MultiLevelApproval : public ApprovalStrategy {
    private:
        int levels;

    public:
        explicit MultiLevelApproval(int levels);
        Decision decide(const std::vector<bool>& votes) const override;
};

#endif //MULTILEVELAPPROVAL_H
