#ifndef MULTILEVELAPPROVAL_H
#define MULTILEVELAPPROVAL_H

#include "ApprovalStrategy.h"

//Strategy: ConcreteStrategy. Every level must approve, in order; one "no" rejects.
/** @brief Requires approval at every configured level. */
class MultiLevelApproval : public ApprovalStrategy {
    private:
        int levels;

    public:
        /** @brief Constructs a multi-level approval strategy. @param levels Number of approval levels required. */
        explicit MultiLevelApproval(int levels);
        Decision decide(const std::vector<bool>& votes) const override;
};

#endif //MULTILEVELAPPROVAL_H
