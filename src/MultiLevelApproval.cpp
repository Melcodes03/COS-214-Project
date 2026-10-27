#include "MultiLevelApproval.h"

MultiLevelApproval::MultiLevelApproval(int levels) : levels(levels < 1 ? 1 : levels) {}

ApprovalStrategy::Decision MultiLevelApproval::decide(const std::vector<bool>& votes) const {
    for (int i = 0; i < (int)votes.size(); i++) {
        if (!votes[i]) {
            return REJECTED;
        }
    }
    return (int)votes.size() >= levels ? APPROVED : PENDING;
}
