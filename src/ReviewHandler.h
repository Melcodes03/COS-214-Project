#ifndef REVIEWHANDLER_H
#define REVIEWHANDLER_H

#include <set>
#include <string>
#include "AssignmentHandler.h"

class ReviewAction;

/// @brief Chain of Responsibility: ConcreteHandler. The Reviewer, the very last link.
///
/// A plain chain guarantees nothing: a request can fall off the end, or sit with someone who
/// never acts. The Reviewer catches whatever reaches it (everybody at the right level and above,
/// the manager included, is full) and applies its ReviewAction: keep it flagged for the next
/// review sweep, or hand it to someone straight away. It never holds work itself.
class ReviewHandler : public AssignmentHandler {
    private:
        ReviewAction* action;                 ///< owned
        std::set<std::string> flagged;        ///< ids of work waiting under review
    public:
        explicit ReviewHandler(ReviewAction* action);   ///< takes ownership of action
        ~ReviewHandler() override;
        Participant* handle(WorkItem& item, const std::vector<Participant*>& candidates) override;
        bool covers(const Participant& worker) const override;   ///< always false: nobody "is" the reviewer
        bool isFlagged(const std::string& itemId) const;
        void clearFlag(const std::string& itemId);
        const ReviewAction& getAction() const;
};

#endif //REVIEWHANDLER_H
