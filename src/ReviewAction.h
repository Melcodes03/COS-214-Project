#ifndef REVIEWACTION_H
#define REVIEWACTION_H

#include <string>
#include <vector>

class Participant;
class WorkItem;

/// @brief Strategy pattern: Strategy. What the Reviewer (the end of the escalation chain)
/// does with work that nobody on the chain could take.
class ReviewAction {
    public:
        virtual ~ReviewAction() {}
        /// @return the person who gets @p item now, or nullptr to keep it waiting under review
        virtual Participant* review(WorkItem& item, const std::vector<Participant*>& candidates) = 0;
        virtual std::string describe() const = 0;
};

/// @brief ConcreteStrategy (the default): keep the work waiting, flag it, and re-offer it
/// from the bottom of the chain when the next review sweep runs (someone may be free by then).
class FlagForReview : public ReviewAction {
    public:
        Participant* review(WorkItem& item, const std::vector<Participant*>& candidates) override;
        std::string describe() const override;
};

/// @brief ConcreteStrategy: give it to the least busy person at the task's level or above,
/// even if they are at capacity (overtime), so nothing ever waits.
class OverflowToLeastBusy : public ReviewAction {
    private:
        std::vector<std::string> roles;   ///< escalation levels, lowest first
    public:
        explicit OverflowToLeastBusy(const std::vector<std::string>& roles);
        Participant* review(WorkItem& item, const std::vector<Participant*>& candidates) override;
        std::string describe() const override;
};

#endif //REVIEWACTION_H
