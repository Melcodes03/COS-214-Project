#include "ApproverPolicy.h"

bool HigherLevelApprover::mayApprove(int approverLevel, int holderLevel, bool samePerson, int topLevel) const {
    if (holderLevel >= topLevel) {
        return approverLevel >= topLevel;   //nobody is above the manager level
    }
    return !samePerson && approverLevel > holderLevel;
}

std::string HigherLevelApprover::describe() const { return "a higher level"; }

bool AnyOtherPersonApprover::mayApprove(int, int, bool samePerson, int) const { return !samePerson; }

std::string AnyOtherPersonApprover::describe() const { return "anyone except the holder"; }

bool HolderMayApprove::mayApprove(int, int, bool, int) const { return true; }

std::string HolderMayApprove::describe() const { return "anyone, the holder included"; }
