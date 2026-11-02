#ifndef APPROVERPOLICY_H
#define APPROVERPOLICY_H

#include <string>

/// @brief Strategy pattern: Strategy. Decides WHO may sign off a task.
///
/// ApprovalStrategy (SingleApproval, MultiLevelApproval) counts the votes; this rule picks
/// the people allowed to cast them. Levels are positions in the escalation chain
/// (1 = lowest, topLevel = the manager level). The rule is swappable, so "self-approval" or
/// "anyone else" is a one-line change in TividyService.
class ApproverPolicy {
    public:
        virtual ~ApproverPolicy() {}
        /// @param approverLevel level of the person who wants to sign off
        /// @param holderLevel   level of the person holding (doing) the task
        /// @param samePerson    true if the approver is the holder
        /// @param topLevel      the highest level (the manager level)
        virtual bool mayApprove(int approverLevel, int holderLevel, bool samePerson, int topLevel) const = 0;
        /// Plain-English rule, shown on the task ("Engineer or above").
        virtual std::string describe() const = 0;
};

/// @brief ConcreteStrategy (the default): a different person at a HIGHER level signs off.
/// Work done at the manager level has nobody above, so the manager may sign it off.
class HigherLevelApprover : public ApproverPolicy {
    public:
        bool mayApprove(int approverLevel, int holderLevel, bool samePerson, int topLevel) const override;
        std::string describe() const override;
};

/// @brief ConcreteStrategy: anybody except the person who did the work.
class AnyOtherPersonApprover : public ApproverPolicy {
    public:
        bool mayApprove(int approverLevel, int holderLevel, bool samePerson, int topLevel) const override;
        std::string describe() const override;
};

/// @brief ConcreteStrategy: the holder may approve their own work (the old behaviour).
class HolderMayApprove : public ApproverPolicy {
    public:
        bool mayApprove(int approverLevel, int holderLevel, bool samePerson, int topLevel) const override;
        std::string describe() const override;
};

#endif //APPROVERPOLICY_H
