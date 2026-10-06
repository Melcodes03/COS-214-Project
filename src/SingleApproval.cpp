#include "SingleApproval.h"

ApprovalStrategy::Decision SingleApproval::decide(const std::vector<bool>& votes) const {
    if (votes.empty()) {
        return PENDING;
    }
    return votes[0] ? APPROVED : REJECTED;
}
