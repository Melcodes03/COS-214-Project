#include "ManagerHandler.h"
#include "Participant.h"

ManagerHandler::ManagerHandler(Participant* manager) : manager(manager) {}

Participant* ManagerHandler::handle(WorkItem& item, const std::vector<Participant*>& candidates) {
    //the manager takes it if they have room; when they are full and a Reviewer follows, the
    //Reviewer decides (so work is never silently piled onto a full manager)
    if (manager != nullptr && (manager->canTakeWork() || getNext() == nullptr)) {
        return manager;
    }
    return passOn(item, candidates);   //no manager, or the manager is full: let the next link try
}

bool ManagerHandler::covers(const Participant& worker) const {
    return &worker == manager;
}
