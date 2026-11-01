#include "ReviewHandler.h"
#include "ReviewAction.h"
#include "WorkItem.h"

ReviewHandler::ReviewHandler(ReviewAction* action) : action(action) {}

ReviewHandler::~ReviewHandler() { delete action; }

Participant* ReviewHandler::handle(WorkItem& item, const std::vector<Participant*>& candidates) {
    Participant* chosen = action->review(item, candidates);
    if (chosen == nullptr) {
        flagged.insert(item.getId());
        return passOn(item, candidates);   //normally nullptr: the work waits under review
    }
    flagged.erase(item.getId());
    return chosen;
}

bool ReviewHandler::covers(const Participant&) const { return false; }

bool ReviewHandler::isFlagged(const std::string& itemId) const { return flagged.count(itemId) > 0; }

void ReviewHandler::clearFlag(const std::string& itemId) { flagged.erase(itemId); }

const ReviewAction& ReviewHandler::getAction() const { return *action; }
